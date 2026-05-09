/**
 * @file led_rgb.c
 * @author Siria
 * @brief Implementação do driver LED RGB com validações de hardware e proteção por Mutex.
 * @version 1.0
 * @date 2026-05-08
 */

#include <stdio.h>
#include "led_rgb.h"
#include "esp_log.h"

/** @name Tags de Log para Depuração */
/**@{*/
static const char *TAG_INIT   = "LED_RGB_INIT";
static const char *TAG_SET    = "LED_RGB_SET";
static const char *TAG_DEINIT = "LED_RGB_DEINIT";
/**@}*/

/**
 * @brief Inicializa o driver LED RGB.
 * 
 * Realiza a validação de GPIOs, Canais LEDC, Resolução e Timers antes de 
 * configurar o periférico de hardware.
 */
esp_err_t led_rgb_init(led_rgb_handle_t *ledRgb, led_rgb_config_t *config) {
    
    // 1. Validação de ponteiros nulos
    if(!ledRgb || !config){
        return ESP_ERR_INVALID_ARG;
    }

    // 2. Validação física dos pinos GPIO
    if (!(GPIO_IS_VALID_GPIO(config->red_pin) && 
          GPIO_IS_VALID_GPIO(config->green_pin) && 
          GPIO_IS_VALID_GPIO(config->blue_pin))){
        ESP_LOGE(TAG_INIT, "Pinos GPIO inválidos para a arquitetura");
        return ESP_ERR_INVALID_ARG;
    }

    // 3. Validação dos canais LEDC
    if (config->red_channel   >= LEDC_CHANNEL_MAX ||  
        config->green_channel >= LEDC_CHANNEL_MAX || 
        config->blue_channel  >= LEDC_CHANNEL_MAX){
        ESP_LOGE(TAG_INIT, "Número do canal LEDC fora do limite permitido");
        return ESP_ERR_INVALID_ARG;
    }

    // 4. Verificação de duplicidade de pinos ou canais
    if (config->red_pin == config->green_pin || config->blue_pin == config->red_pin || config->blue_pin == config->green_pin){
        ESP_LOGE(TAG_INIT, "Erro: Existe duplicidade nos pinos GPIO atribuídos");
        return ESP_ERR_INVALID_ARG;
    }

    if (config->red_channel == config->green_channel || config->blue_channel == config->red_channel || config->blue_channel == config->green_channel){
        ESP_LOGE(TAG_INIT, "Erro: Existe duplicidade nos canais LEDC atribuídos");
        return ESP_ERR_INVALID_ARG;
    }

    // 5. Validação de Timer e Resolução
    if (config->duty_resolution <= 0 || config->duty_resolution >= LEDC_TIMER_BIT_MAX ) {
        ESP_LOGE(TAG_INIT, "Resolução de Duty Cycle inválida");
        return ESP_ERR_INVALID_ARG;
    }

    if (config->timer_num >= LEDC_TIMER_MAX) {
        ESP_LOGE(TAG_INIT, "Número do timer LEDC inválido");
        return ESP_ERR_INVALID_ARG;
    }

    // Inicialização do Handle e Mutex
    ledRgb->config_led = *config;
    ledRgb->mutex = xSemaphoreCreateMutex();

    if(ledRgb->mutex == NULL){
        ESP_LOGE(TAG_INIT, "Falha ao criar o Mutex de proteção");
        return ESP_FAIL;
    }

    // Configuração do Timer PWM
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = config->duty_resolution,
        .timer_num        = ledRgb->config_led.timer_num,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    
    esp_err_t err = ledc_timer_config(&ledc_timer);
    if(err != ESP_OK){
        vSemaphoreDelete(ledRgb->mutex);
        return err;
    }

    // Configuração base dos canais
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .timer_sel      = ledRgb->config_led.timer_num,
        .intr_type      = LEDC_INTR_DISABLE,
        .duty           = 0, 
        .hpoint         = 0       
    };

    // Aplicação para os 3 canais (R, G, B)
    const ledc_channel_t channels[] = {config->red_channel, config->green_channel, config->blue_channel};
    const gpio_num_t pins[] = {config->red_pin, config->green_pin, config->blue_pin};

    for (int i = 0; i < 3; i++) {
        ledc_channel.channel = channels[i];
        ledc_channel.gpio_num = pins[i];
        err = ledc_channel_config(&ledc_channel);
        if (err != ESP_OK) {
            vSemaphoreDelete(ledRgb->mutex);
            return err;
        }
    }

    ESP_LOGI(TAG_INIT, "Driver inicializado com sucesso");
    return ESP_OK;
}

/**
 * @brief Atualiza as cores do LED.
 * 
 * @param red Valor de intensidade vermelha.
 * @param green Valor de intensidade verde.
 * @param blue Valor de intensidade azul.
 * 
 * @note Possui timeout de 1000ms para obtenção do Mutex.
 */
esp_err_t led_rgb_set_color(led_rgb_handle_t *ledRgb, uint32_t red, uint32_t green, uint32_t blue) {

    if (!ledRgb) return ESP_ERR_INVALID_ARG;

    // Validação de limite de resolução
    uint32_t max_duty = (1 << ledRgb->config_led.duty_resolution) - 1;

    if (red > max_duty || green > max_duty || blue > max_duty) {
        ESP_LOGE(TAG_SET, "Valores RGB excedem a resolução máxima (%u)", (unsigned int)max_duty);
        return ESP_ERR_INVALID_ARG;
    }

    // Tentativa de obter o Mutex (Thread-Safety)
    if (xSemaphoreTake(ledRgb->mutex, pdMS_TO_TICKS(1000)) != pdTRUE){     
        ESP_LOGE(TAG_SET, "Recurso ocupado: Não foi possível acessar o driver");
        return ESP_ERR_TIMEOUT;
    }

    // Array para facilitar a iteração de escrita
    const uint32_t values[] = {red, green, blue};
    const ledc_channel_t channels[] = {ledRgb->config_led.red_channel, 
                                       ledRgb->config_led.green_channel, 
                                       ledRgb->config_led.blue_channel};

    for (int i = 0; i < 3; i++) {
        ledc_set_duty(LEDC_MODE, channels[i], values[i]);
        ledc_update_duty(LEDC_MODE, channels[i]);
    }

    xSemaphoreGive(ledRgb->mutex);
    return ESP_OK;
}

/**
 * @brief Finaliza o driver e libera recursos.
 */
esp_err_t led_rgb_deinit(led_rgb_handle_t *ledRgb){
    if(!ledRgb) return ESP_ERR_INVALID_ARG;

    // Para o sinal PWM em todos os canais
    ledc_stop(LEDC_MODE, ledRgb->config_led.red_channel, 0);
    ledc_stop(LEDC_MODE, ledRgb->config_led.green_channel, 0);
    ledc_stop(LEDC_MODE, ledRgb->config_led.blue_channel, 0);

    // Deleta o Mutex
    if (ledRgb->mutex != NULL) {
        vSemaphoreDelete(ledRgb->mutex);
        ledRgb->mutex = NULL;
    }

    ESP_LOGI(TAG_DEINIT, "Driver finalizado.");
    return ESP_OK;
}