/**
 * @file main.c
 * @author Síria Cabral
 * @brief Exemplo de comunicação entre tarefas usando Filas (Queues) no FreeRTOS.
 * @version 0.1
 * @date 2026-05-08
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/** 
 * @brief Handle da fila utilizada para comunicação entre as tarefas de leitura e escrita.
 */
QueueHandle_t xqueue = NULL;

/**
 * @brief Protótipo da tarefa responsável por simular a leitura de um sensor.
 */
void vTaskLeitura();

/**
 * @brief Protótipo da tarefa responsável por processar/escrever os dados recebidos da fila.
 */
void vTaskEscrita();

/**
 * @brief Ponto de entrada principal da aplicação.
 * 
 * Inicializa a fila e cria as tarefas de processamento.
 */
void app_main() {
    // Cria uma fila para armazenar 10 itens do tamanho de um inteiro
    xqueue = xQueueCreate( 10, sizeof( int ));

    if (xqueue != NULL) {
        xTaskCreate( vTaskLeitura, "leitura", 2048, NULL, 1, NULL );
        xTaskCreate( vTaskEscrita, "escrita", 2048, NULL, 1, NULL );
    } else {
        ESP_LOGE("MAIN", "Falha ao criar a fila!");
    }
}

/**
 * @brief Tarefa de Simulação de Leitura.
 * 
 * Incrementa um contador e tenta enviar o valor para a fila a cada 500ms.
 * Se a fila estiver cheia por mais de 100ms, emite um aviso no log.
 */
void vTaskLeitura(){
    int contador = 0;
    for(;;){
        // Tenta enviar o dado para a fila
        if(xQueueSend(xqueue, &contador, pdMS_TO_TICKS(100)) != pdPASS) {
            ESP_LOGW("FILA", "Fila cheia, não foi possível enviar o dado.");
        }
        
        contador++;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/**
 * @brief Tarefa de Escrita/Log.
 * 
 * Aguarda indefinidamente (portMAX_DELAY) por um dado na fila e,
 * ao receber, exibe o valor no console. Possui um delay interno de 1s.
 */
void vTaskEscrita(){
    int dado_recebido = 0;
    for(;;){
        // Aguarda o dado chegar na fila
        if(xQueueReceive( xqueue, &dado_recebido , portMAX_DELAY ) == pdPASS) {
            ESP_LOGI("SENSOR", "Valor recebido: %d", dado_recebido);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}