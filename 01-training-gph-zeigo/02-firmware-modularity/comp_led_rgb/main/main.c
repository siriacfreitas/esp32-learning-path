/**
 * @file main.c
 * @author Siria
 * @brief Aplicação principal para controle de um LED RGB via PWM (LEDC).
 * @version 1.0
 * @date 2026-05-08
 * 
 * @copyright Copyright (c) 2026
 */

#include <stdio.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "led_rgb.h"
#include "esp_log.h"

/** @name Definições de Hardware */
/**@{*/
#define RED_PIN_GPIO   GPIO_NUM_23   /*!< GPIO para o canal vermelho */
#define GREEN_PIN_GPIO GPIO_NUM_22   /*!< GPIO para o canal verde */
#define BLUE_PIN_GPIO  GPIO_NUM_21   /*!< GPIO para o canal azul */

#define RED_LEDC_CHANNEL   LEDC_CHANNEL_0
#define GREEN_LEDC_CHANNEL LEDC_CHANNEL_1
#define BLUE_LEDC_CHANNEL  LEDC_CHANNEL_2

#define LEDC_DUTY_RESOLUTION LEDC_TIMER_8_BIT /*!< Resolução de 8 bits (0-255) */
#define LEDC_TIMER           LEDC_TIMER_0     /*!< Timer utilizado pelo LEDC */
/**@}*/

/** 
 * @brief Estrutura de configuração do LED RGB preenchida com as macros de hardware.
 */
led_rgb_config_t led_config = {
    .red_pin = RED_PIN_GPIO,
    .blue_pin = BLUE_PIN_GPIO,
    .green_pin = GREEN_PIN_GPIO,

    .red_channel = RED_LEDC_CHANNEL,
    .green_channel = GREEN_LEDC_CHANNEL,
    .blue_channel = BLUE_LEDC_CHANNEL,

    .timer_num = LEDC_TIMER,
    .duty_resolution = LEDC_DUTY_RESOLUTION,
};

/** @brief Handle para manipulação do driver LED RGB. */
led_rgb_handle_t led_rgb_handle;

/**
 * @brief Ponto de entrada principal da aplicação.
 * 
 * Inicializa o driver LED RGB e executa um loop de piscar (blink) alternando
 * entre ligado (branco) e desligado.
 */
void app_main(void)
{
    // Inicialização do driver com a configuração definida
    esp_err_t err = led_rgb_init(&led_rgb_handle, &led_config);

    if (err != ESP_OK)
    {
        ESP_LOGE("[LED_RGB]", "Falha na inicialização do driver!");
    }

    /**
     * @brief Loop de controle de cor.
     * Alterna a cor do LED a cada 500ms.
     */
    while (1)
    {
        // Define a cor como Branco (máxima intensidade em R, G e B)
        err = led_rgb_set_color(&led_rgb_handle, 255, 255, 255);
        if (err != ESP_OK)
        {
            ESP_LOGE("[LED_RGB]", "Erro ao definir cor: Branco");
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
        
        // Desliga o LED (intensidade zero em todos os canais)
        err = led_rgb_set_color(&led_rgb_handle, 0, 0, 0);
        if (err != ESP_OK)
        {
            ESP_LOGE("[LED_RGB]", "Erro ao definir cor: Desligado");
            break;
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    // Código de limpeza (caso o loop seja interrompido)
    vTaskDelay(pdMS_TO_TICKS(2000));
    led_rgb_deinit(&led_rgb_handle);
    
    // Deleta a tarefa main para liberar recursos do sistema
    vTaskDelete(NULL);
}