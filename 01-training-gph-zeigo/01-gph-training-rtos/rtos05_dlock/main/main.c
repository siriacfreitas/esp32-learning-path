/**
 * @file desafio5_deadlock.c
 * @author Siria
 * @brief Implementação de um cenário de Deadlock (Impasse) para fins educacionais.
 * @version 1.0
 * @date 2026-05-11
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"

#define TAG "DESAFIO_5"

/** @name Recursos Compartilhados */
/**@{*/
SemaphoreHandle_t mutexA;
SemaphoreHandle_t mutexB;
/**@}*/

void vTaskCode1 (void *pvParameters);
void vTaskCode2 (void *pvParameters);

void app_main(void)
{
    // Inicialização dos Mutexes
    mutexA = xSemaphoreCreateMutex();
    mutexB = xSemaphoreCreateMutex();

    if (mutexA == NULL || mutexB == NULL) {
        ESP_LOGE(TAG, "Erro ao criar os mutexes");
        return;
    }

    // Criação das tarefas com a mesma prioridade
    xTaskCreate(vTaskCode1, "TASK1", 2048, NULL, 1, NULL);
    xTaskCreate(vTaskCode2, "TASK2", 2048, NULL, 1, NULL);
}

/**
 * @brief Task 1: Tenta adquirir Mutex A e depois Mutex B.
 */
void vTaskCode1 (void *pvParameters) {
    for(;;) {
        if(xSemaphoreTake(mutexA, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Task 1: Pegou Mutex A, aguardando Mutex B...");
            
            if(xSemaphoreTake(mutexB, portMAX_DELAY) == pdTRUE) {
                printf("Task 1: Conseguiu ambos os recursos!\n");
                xSemaphoreGive(mutexB);
            }
            xSemaphoreGive(mutexA);
        }
        //vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/**
 * @brief Task 2: Tenta adquirir Mutex B e depois Mutex A.
 */
void vTaskCode2 (void *pvParameters) {
    for(;;) {
        if(xSemaphoreTake(mutexB, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Task 2: Pegou Mutex B, aguardando Mutex A...");

            if(xSemaphoreTake(mutexA, portMAX_DELAY) == pdTRUE) {
                printf("Task 2: Conseguiu ambos os recursos!\n");
                xSemaphoreGive(mutexA);
            }
            xSemaphoreGive(mutexB);
        }
        //vTaskDelay(pdMS_TO_TICKS(500));
    }
}