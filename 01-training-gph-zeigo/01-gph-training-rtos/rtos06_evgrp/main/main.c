/**
 * @file desafio6_event_groups.c
 * @author Siria
 * @brief Sincronização multi-tarefa usando Event Groups no FreeRTOS.
 * @version 1.0
 * @date 2026-05-11
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

#define TAG "DESAFIO_6"

/** @name Definição dos Bits de Evento */
/**@{*/
#define BIT_1 (1 << 0) 
#define BIT_2 (1 << 1) 
#define BIT_3 (1 << 2) 
/**@}*/

EventGroupHandle_t event_group;

/* Variáveis globais para simular dados de sensores */
int sensor1 = 0;
int sensor2 = 0;
int sensor3 = 0;

void TaskSensor1 (void *pvParameters);
void TaskSensor2 (void *pvParameters);
void TaskSensor3 (void *pvParameters);
void TaskProcess (void *pvParameters);

void app_main(void)
{
    srand(time(NULL));
    
    /** @brief Criação do grupo de eventos */
    event_group = xEventGroupCreate();

    if (event_group != NULL) {
        xTaskCreate(TaskSensor1, "SENSOR1", 2048, NULL, 1, NULL);
        xTaskCreate(TaskSensor2, "SENSOR2", 2048, NULL, 1, NULL);
        xTaskCreate(TaskSensor3, "SENSOR3", 2048, NULL, 1, NULL);
        
        // Task de processamento com prioridade maior para garantir resposta rápida
        xTaskCreate(TaskProcess, "PROCESS", 2048, NULL, 2, NULL);
    }
}

void TaskSensor1 (void *pvParameters) {
    for(;;) {
        sensor1 = rand() % 100;
        printf("[Sensor 1] Valor: %d\n", sensor1);
        
        /** @brief Sinaliza que o dado do sensor 1 está pronto */
        xEventGroupSetBits(event_group, BIT_1);
        
        vTaskDelay(pdMS_TO_TICKS(1000 + (rand() % 500))); // Delay variável para testar sincronia
    }
}

void TaskSensor2 (void *pvParameters) {
    for(;;) {
        sensor2 = rand() % 100;
        printf("[Sensor 2] Valor: %d\n", sensor2);
        
        xEventGroupSetBits(event_group, BIT_2);
        
        vTaskDelay(pdMS_TO_TICKS(1200));
    }
}

void TaskSensor3 (void *pvParameters) {
    for(;;) {
        sensor3 = rand() % 100;
        printf("[Sensor 3] Valor: %d\n", sensor3);
        
        xEventGroupSetBits(event_group, BIT_3);
        
        vTaskDelay(pdMS_TO_TICKS(800));
    }
}

void TaskProcess (void *pvParameters) {
    for(;;) {
        /** * @brief Aguarda todos os bits (BIT_1 | BIT_2 | BIT_3)
         * - pdTRUE: Limpa os bits ao sair (prepara para o próximo ciclo)
         * - pdTRUE: Aguarda TODOS os bits (lógica AND)
         * - portMAX_DELAY: Aguarda indefinidamente
         */
        xEventGroupWaitBits(event_group, BIT_1 | BIT_2 | BIT_3, pdTRUE, pdTRUE, portMAX_DELAY);
        
        int soma = sensor1 + sensor2 + sensor3;
        
        printf("\n>>> CICLO DE LEITURA COMPLETO <<<\n");
        printf("Soma total dos sensores: %d\n", soma);
        printf("----------------------------------\n\n");
    }
}