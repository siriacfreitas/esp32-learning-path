/**
 * @file led_rgb.h
 * @author Siria
 * @brief Header do driver para controle de LED RGB utilizando o periférico LEDC (PWM) do ESP32.
 * @version 1.0
 * @date 2026-05-08
 */

#ifndef LED_RGB_H
#define LED_RGB_H

#include <stdio.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @name Configurações Padrão do LEDC */
/**@{*/
#define LEDC_MODE               LEDC_LOW_SPEED_MODE     /*!< Modo de velocidade do LEDC */
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT        /*!< Resolução padrão de 8 bits (0-255) */
#define LEDC_FREQUENCY          (4000)                  /*!< Frequência de 4 kHz */
/**@}*/

/**
 * @brief Estrutura de configuração inicial do LED RGB.
 */
typedef struct 
{
    gpio_num_t red_pin;         /*!< Pino GPIO para o componente Vermelho */
    gpio_num_t green_pin;       /*!< Pino GPIO para o componente Verde */
    gpio_num_t blue_pin;        /*!< Pino GPIO para o componente Azul */

    ledc_channel_t red_channel;   /*!< Canal LEDC para o Vermelho */
    ledc_channel_t green_channel; /*!< Canal LEDC para o Verde */
    ledc_channel_t blue_channel;  /*!< Canal LEDC para o Azul */

    ledc_timer_bit_t duty_resolution; /*!< Resolução do Duty Cycle (bits) */
    ledc_timer_t timer_num;           /*!< Número do Timer LEDC utilizado */
} led_rgb_config_t;

/**
 * @brief Handle de controle do LED RGB. Contém a configuração e o Mutex de proteção.
 */
typedef struct
{  
    led_rgb_config_t config_led;    /*!< Cópia da configuração do LED */
    SemaphoreHandle_t mutex;        /*!< Mutex para proteção de acesso concorrente */
} led_rgb_handle_t;

/**
 * @brief Inicializa os pinos e canais PWM para o LED RGB.
 * * @param ledRgb Ponteiro para o handle que será inicializado.
 * @param config Ponteiro para a estrutura de configuração.
 * @return esp_err_t ESP_OK em caso de sucesso, ESP_ERR_INVALID_ARG em caso de erro nos parâmetros.
 */
esp_err_t led_rgb_init(led_rgb_handle_t *ledRgb, led_rgb_config_t *config);

/**
 * @brief Define a cor do LED RGB.
 * * @param ledRgb Ponteiro para o handle do LED.
 * @param red Valor de intensidade vermelha (depende da resolução).
 * @param green Valor de intensidade verde.
 * @param blue Valor de intensidade azul.
 * @return esp_err_t ESP_OK, ESP_ERR_TIMEOUT se o mutex estiver ocupado.
 */
esp_err_t led_rgb_set_color(led_rgb_handle_t *ledRgb, uint32_t red, uint32_t green, uint32_t blue);

/**
 * @brief Desliga o PWM e libera os recursos associados ao LED RGB.
 * * @param ledRgb Ponteiro para o handle do LED.
 * @return esp_err_t ESP_OK em caso de sucesso.
 */
esp_err_t led_rgb_deinit(led_rgb_handle_t *ledRgb);

#ifdef __cplusplus
}   
#endif

#endif /* LED_RGB_H */