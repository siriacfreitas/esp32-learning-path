/**
 * @file main.c
 * @author Siria
 * @brief Implementação de exclusão mútua (Mutex) para proteção de recurso compartilhado.
 * @version 0.1
 * @date 2026-05-08
 * 
 * @copyright Copyright (c) 2026
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_err.h"

/** @brief Handle do Mutex para sincronização. */
SemaphoreHandle_t xSemaphore = NULL;

/** @brief Tag de log para identificação no monitor serial. */
#define TAG "DESAFIO_3"

/** @brief Variável global protegida pelo Mutex. */
int counter = 0;

/**
 * @brief Protótipo da Task 1.
 */
void vTaskCode1( void * pvParameters );

/**
 * @brief Protótipo da Task 2.
 */
void vTaskCode2( void * pvParameters );

/**
 * @brief Inicialização do sistema.
 * 
 * Cria o Mutex e as tarefas que acessam o contador global de forma segura.
 */
void app_main(void)
{
    // Criação do Mutex antes de iniciar as tarefas
    xSemaphore = xSemaphoreCreateMutex();

    if (xSemaphore != NULL) {
        xTaskCreate( vTaskCode1, "COUNT1", 2048, NULL, 1, NULL );
        xTaskCreate( vTaskCode2, "COUNT2", 2048, NULL, 1, NULL );
    } else {
        ESP_LOGE(TAG, "Erro ao criar o Mutex!");
    }
}

/**
 * @brief Tarefa 1 com proteção de Mutex.
 * 
 * Solicita a posse do Mutex e, se obtido, incrementa @ref counter com segurança.
 */
void vTaskCode1( void * pvParameters )
{
    for( ;; )
    {
        // Tenta tomar o Mutex indefinidamente
        if( xSemaphoreTake( xSemaphore, portMAX_DELAY) == pdTRUE )
        {
            counter++;
            ESP_LOGI(TAG, "COUNTER1: %d", counter);
            
            // Libera o Mutex para que outras tarefas possam usá-lo
            xSemaphoreGive( xSemaphore );
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/**
 * @brief Tarefa 2 com proteção de Mutex.
 * 
 * Solicita a posse do Mutex e, se obtido, incrementa @ref counter com segurança.
 */
void vTaskCode2( void * pvParameters )
{
    for( ;; )
    {
        if( xSemaphoreTake( xSemaphore, portMAX_DELAY ) == pdTRUE )
        {
            counter++;
            ESP_LOGI(TAG, "COUNTER2: %d", counter);
            
            xSemaphoreGive( xSemaphore );
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}