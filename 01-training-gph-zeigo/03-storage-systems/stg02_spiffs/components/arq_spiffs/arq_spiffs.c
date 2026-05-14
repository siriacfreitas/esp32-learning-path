/**
 * @file arq_spiffs.c
 * @author Siria
 * @brief Implementação do driver de abstração SPIFFS com segurança RTOS.
 */

#include <stdio.h>
#include <string.h>
#include "esp_spiffs.h"
#include "esp_log.h"
#include "arq_spiffs.h"

#define TAG "ARQ_SPIFFS"

/**
 * @brief Inicializa o sistema de arquivos SPIFFS e cria o Mutex de proteção.
 */
esp_err_t arq_spiffs_init(arq_spiffs_handle_t *handle, arq_spiffs_config_t *config){
    if (handle == NULL || config == NULL || config->path == NULL || config->part_label == NULL) {
        ESP_LOGE(TAG, "Argumentos invalidos na inicializacao");
        return ESP_ERR_INVALID_ARG;
    }

    handle->spiffs_config = *config;
    handle->mutex = xSemaphoreCreateMutex();

    if (handle->mutex == NULL) {
        ESP_LOGE(TAG, "Falha ao criar Mutex");
        return ESP_ERR_NO_MEM;
    } 

    esp_vfs_spiffs_conf_t config_spiffs_init = {
        .base_path = handle->spiffs_config.path,
        .partition_label = handle->spiffs_config.part_label,
        .max_files = handle->spiffs_config.mxfiles,
        .format_if_mount_failed = true // Formata a partição se a montagem falhar
    };

    esp_err_t ret = esp_vfs_spiffs_register(&config_spiffs_init);
    if (ret != ESP_OK) {
        vSemaphoreDelete(handle->mutex);
        handle->mutex = NULL;
        return ret;
    }
    
    ESP_LOGI(TAG, "Spiffs inicializada com sucesso em %s", config->path);
    return ESP_OK;
}

/**
 * @brief Grava uma string no arquivo. 
 */
esp_err_t arq_spiffs_write(arq_spiffs_handle_t *handle, char *modo){
    if (!handle || !handle->spiffs_config.data){
        return ESP_ERR_INVALID_ARG;
    }

    // Tenta obter o Mutex por até 1 segundo antes de falhar
    if (xSemaphoreTake(handle->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    FILE *f = fopen(handle->spiffs_config.namesp, modo);
    if (f == NULL){
        ESP_LOGE(TAG, "Falha ao abrir o arquivo para escrita: %s", handle->spiffs_config.namesp);
        xSemaphoreGive(handle->mutex);
        return ESP_FAIL;
    }

    fprintf(f, "%s\n", handle->spiffs_config.data);
    fclose(f);
    
    xSemaphoreGive(handle->mutex);
    return ESP_OK;
}

/**
 * @brief Lê o arquivo linha por linha e imprime no terminal.
 */
esp_err_t arq_spiffs_read(arq_spiffs_handle_t *handle){
    if (!handle) return ESP_ERR_INVALID_ARG;

    if (xSemaphoreTake(handle->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    FILE *f = fopen(handle->spiffs_config.namesp, "r");
    if (f == NULL){
        ESP_LOGW(TAG, "Arquivo nao encontrado ou vazio.");
        xSemaphoreGive(handle->mutex);
        return ESP_ERR_NOT_FOUND;
    }

    char linha[128];
    while (fgets(linha, sizeof(linha), f) != NULL) {
        linha[strcspn(linha, "\n")] = 0;
        printf("[LOG] %s\n", linha); 
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    fclose(f);
    xSemaphoreGive(handle->mutex);
    return ESP_OK;
}

/**
 * @brief Desregistra o VFS e libera o Mutex.
 */
esp_err_t arq_spiffs_deinit(arq_spiffs_handle_t *handle) {
    if (handle && handle->mutex) {
        esp_vfs_spiffs_unregister(handle->spiffs_config.part_label);
        vSemaphoreDelete(handle->mutex);
        handle->mutex = NULL;
        ESP_LOGI(TAG, "Recursos SPIFFS liberados.");
        return ESP_OK;
    }
    return ESP_ERR_INVALID_ARG;
}