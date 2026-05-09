/**
 * @file main.c
 * @author Siria
 * @brief Demonstração de condição de corrida (race condition) entre duas tarefas.
 * @version 0.1
 * @date 2026-05-08
 * 
 * @copyright Copyright (c) 2026
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

/** @brief Tag utilizada para os logs do sistema. */
#define TAG "DESAFIO_2"

/** 
 * @brief Variável global compartilhada entre as tarefas. 
 * @note Esta variável está sujeita a condições de corrida.
 */
int counter = 0;

/**
 * @brief Protótipo da Task 1 para incremento do contador.
 * @param pvParameters Parâmetros passados na criação da tarefa (não utilizados).
 */
void vTaskCode1( void * pvParameters );

/**
 * @brief Protótipo da Task 2 para incremento do contador.
 * @param pvParameters Parâmetros passados na criação da tarefa (não utilizados).
 */
void vTaskCode2( void * pvParameters );

/**
 * @brief Ponto de entrada da aplicação.
 * 
 * Cria as duas tarefas que competirão pelo acesso à variável global.
 */
void app_main(void)
{
    xTaskCreate( vTaskCode1, "COUNT1", 2048, NULL, 1, NULL );
    xTaskCreate( vTaskCode2, "COUNT2", 2048, NULL, 1, NULL );
}

/**
 * @brief Tarefa 1: Incrementa e imprime o contador.
 * 
 * Incrementa a variável global @ref counter e exibe o valor via ESP_LOGI a cada 500ms.
 */
void vTaskCode1( void * pvParameters )
{
    for( ;; )
    {
        counter++;
        ESP_LOGI(TAG, "COUNTER1: %d", counter);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/**
 * @brief Tarefa 2: Incrementa e imprime o contador.
 * 
 * Incrementa a variável global @ref counter e exibe o valor via ESP_LOGI a cada 500ms.
 */
void vTaskCode2( void * pvParameters )
{
    for( ;; )
    {
        counter++;
        ESP_LOGI(TAG, "COUNTER2: %d", counter);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}