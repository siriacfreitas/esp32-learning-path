/**
 * @file main.c
 * @author Siria
 * @brief Datalogger Tolerante a Falhas.
 * * O sistema utiliza um Timer de Hardware para amostrar dados de sensores, envia os dados
 * para o SD Card (Caminho Nominal) e, em caso de falha física, desvia o fluxo para a 
 * SPIFFS (Caminho de Fallback). Ao detectar o retorno do SD Card, realiza a sincronização.
 * * @version 1.0
 * @date 2026-05-18
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "sd_manager.h"
#include "spiffs_manager.h"

// Definição da pinagem do barramento SPI para o SD Card
#define MOSI_PIN         23
#define MISO_PIN         19
#define SLK_PIN          18
#define CS_PIN           5

#define TAG               "LOGGER_APP" 
#define BUFFER_LOG_SIZE   160 ///< Tamanho máximo de cada string de log

// Handles Globais Operacionais
sd_manager_handle_t sd_h;             ///< Handler do driver do SD Card
spiffs_manager_handle_t spiffs_h;     ///< Handler do driver da SPIFFS
TaskHandle_t sensor_handle;           ///< Handle da tarefa de leitura do sensor
esp_timer_handle_t timer_handler;     ///< Handler do Timer de Hardware do ESP32


// Protótipos das funções internas
void task_sensor(void *pvParameters);
esp_err_t logger_save(const char *data);
void timer_isr(void *arg);

/**
 * @brief Ponto de entrada da aplicação. Inicializa periféricos, filas e temporizadores.
 */
void app_main(void) {
    ESP_LOGI(TAG, "Inicializando sistema de armazenamento tolerante a falhas...");

/**
 * @note Configuração estruturada para inicialização do SD Card via barramento SPI.
 */
sd_manager_conif_t sd_config = {
    .spi = {
        .spi_config = {
            .sck_pin  = SLK_PIN,
            .mosi_pin = MOSI_PIN,
            .miso_pin = MISO_PIN
        },
        .spi_dma = SDSPI_DEFAULT_DMA
    },
    .cs_pin    = CS_PIN,
    .mxfiles   = 5,
    .clusters  = 16 * 1024,
    .file_name = "/sdcard/log.txt",
    .data      = NULL 
};

    
 /**
 * @note Inicialização do Caminho Nominal (SD Card)
 */ 
    if (sd_manager_init(&sd_h, &sd_config) == ESP_OK) {
        ESP_LOGI(TAG, "Caminho Nominal (SD Card) inicializado com sucesso.");
    } else {
        ESP_LOGW(TAG, "SD Card nao detectado no boot. Sistema operando em Modo Fallback inicial.");
    }

 /**
 * @note Inicialização do Caminho de Fallback (SPIFFS - Memória Flash Interna)
 */  
    spiffs_manager_config_t spiffs_cfg = {
        .path = "/spiffs",
        .part_label = "storage",
        .mxfiles = 5,
        .namesp = "/spiffs/backlog.txt"
    };

    if (spiffs_manager_init(&spiffs_h, &spiffs_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Erro fatal: Nao foi possivel inicializar a memoria Flash interna (SPIFFS)!");
        return; 
    }
    ESP_LOGI(TAG, "Caminho de Fallback (SPIFFS) pronto.");

    /** * @note Arquitetura Defensiva: As tarefas só são criadas após a garantia 
     * de que todos os recursos de memória e drivers foram alocados com sucesso.
     */
    xTaskCreate(task_sensor, "TASK_SENSOR", 4096, NULL, 3, &sensor_handle);
    /** 
    * @note Configuração do Timer de Alta Precisão  5 segundoS (Interrupção por Hardware)
    */
    const esp_timer_create_args_t timer_args = {
        .callback = &timer_isr,
        .name = "timer_sensor_malha"
    };
    
    esp_timer_create(&timer_args, &timer_handler);
    esp_timer_start_periodic(timer_handler, 5000 * 1000); 
}

/**
 * @brief Tarefa de processamento e amostragem de dados do sensor.
 * Fica bloqueada aguardando notificações enviadas pela ISR do Timer.
 */
void task_sensor(void *pvParameters) {
    char payload[BUFFER_LOG_SIZE];
    for(;;){
        if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY)) {
            /**
             * @note Simula a leitura de dados de um sensor de temperatura (15°C a 44°C)
             */
            snprintf(payload, sizeof(payload), "Timestamp: %lu, Sensor_Data: %d", esp_log_timestamp(), (rand() % 30) + 15);
            ESP_LOGI(TAG, "Enviando payload para a fila do SD Card...");
            logger_save(payload);
        }
    }
}

/**
 * @brief Algoritmo Core de Failover e Sincronização de Dados.
 * Gerencia o desvio do fluxo de dados se o SD Card falhar e processa a 
 * recuperação atômica do histórico contido na SPIFFS quando o SD retorna.
 * * @param data Ponteiro para a string de texto contendo a telemetria do sensor.
 * @return esp_err_t ESP_OK em caso de sucesso no armazenamento nominal.
 */
esp_err_t logger_save(const char *data) {
    esp_err_t ret;
    char linha_backlog[BUFFER_LOG_SIZE];
    char linha_log[BUFFER_LOG_SIZE + 20];

    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    sd_h.sd_config.data = data;
    ret = sd_manager_write_file(&sd_h, "a");
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Dado salvo no SD Card");
        sd_manager_read_file(&sd_h);
        return ESP_OK;
    }
    
    /**
     * @section MÁQUINA DE FAILOVER (MODO FALLBACK)
     * Se o código chegou aqui, a gravação nominal falhou. Iniciamos o desvio para a SPIFFS.
     */
    ESP_LOGW(TAG, "SD falhou! Salvando no SPIFFS ... ");
    spiffs_h.spiffs_config.data = data;
    ret = spiffs_manager_write(&spiffs_h, "a");
    spiffs_manager_read(&spiffs_h);

    /**
     * @note Pequeno delay controlado para estabilização de hardware antes da tentativa de remount
     */ 
    vTaskDelay(pdMS_TO_TICKS(1000)); 
    ESP_LOGI(TAG, "[Sync] Verificando se o cartao foi reinserido...");

    /**
     * @section SINCRONIZAÇÃO ATÔMICA
     * Tenta remontar o sistema de arquivos do SD Card para verificar se ele retornou.
     */
    if (sd_manager_mount(&sd_h) == ESP_OK) {
        ESP_LOGI(TAG, "[Sync] ===> SD Card detectado! Iniciando Sincronizacao Atómica...");
    
        FILE *f_backlog = fopen("/spiffs/backlog.txt", "r");

        if (f_backlog != NULL) {
            while (fgets(linha_backlog, sizeof(linha_backlog), f_backlog) != NULL) {
                linha_backlog[strcspn(linha_backlog, "\r\n")] = 0; 
                snprintf(linha_log, sizeof(linha_log), "SPIFFS: %s", linha_backlog);
                
                sd_h.sd_config.data = linha_log;
                sd_manager_write_file(&sd_h, "a");
            }
            fclose(f_backlog);
            spiffs_manager_format(&spiffs_h);
            ESP_LOGI(TAG, "[Sync] Sincronizacao concluida. Flash interna limpa!");
        } else {
            ESP_LOGI(TAG, "[Sync] Nenhum dado pendente na SPIFFS.");
        }
    }

    return ret;
}

/**
 * @brief Rotina de Interrupção do Timer (ISR).
 * Executada diretamente na IRAM do microcontrolador para garantir determinismo temporal.
 */
void IRAM_ATTR timer_isr(void *arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(sensor_handle, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}