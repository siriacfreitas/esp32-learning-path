/**
 * @file desafio4_sincronizacao.c
 * @author Siria
 * @brief Sincronização entre tarefas usando Semáforo Binário no FreeRTOS.
 * @version 1.0
 * @date 2026-05-09
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

/** @brief Handle do semáforo para sincronização. */
SemaphoreHandle_t xSemaphEvent = NULL;

/** @brief Variável simulando o dado de um sensor. */
int random_val = 0;

#define TAG "DESAFIO_4"

void vTaskSensor(void * pvParameters);
void vTaskProcessor(void * pvParameters);

void app_main(void) {
    srand(time(NULL));

    /** 
     * @brief Criação de um Semáforo Binário. 
     * Diferente do Mutex, ele inicia "vazio" (0), forçando a tarefa que tenta 
     * dar o 'Take' a esperar até que ocorra um 'Give'.
     */
    xSemaphEvent = xSemaphoreCreateBinary();
    
    if (xSemaphEvent != NULL) {
        xTaskCreate(vTaskSensor, "TaskSensor", 2048, NULL, 1, NULL);
        xTaskCreate(vTaskProcessor, "TaskProcessor", 2048, NULL, 1, NULL);
    } else {
        ESP_LOGE(TAG, "Erro ao criar o Semáforo!");
    }
}

/**
 * @brief Tarefa Produtora (Evento).
 * Simula a leitura de um sensor. Quando um dado válido chega (n > 50), 
 * sinaliza a tarefa de processamento.
 */
void vTaskSensor(void * pvParameters){
    for(;;){
        random_val = rand() % 2; 
        ESP_LOGI(TAG, "[Sensor] Evento detectado! Valor: %d. Notificando...", random_val);
        if (random_val ) {
            /** @brief Sinaliza que o evento ocorreu. */
            xSemaphoreGive(xSemaphEvent);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Tarefa Consumidora (Sincronizada).
 * Fica em estado 'Blocked' sem consumir CPU até que o semáforo seja liberado.
 */
void vTaskProcessor(void * pvParameters){
    for(;;){
        /** @brief Aguarda o evento indefinidamente (portMAX_DELAY). */
        if(xSemaphoreTake(xSemaphEvent, portMAX_DELAY) == pdTRUE){
            ESP_LOGW(TAG, "[Processor] Evento recebido! Processando dado: %d", random_val);
        }
    }
}