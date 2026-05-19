/**
 * @file spiffs_manager.c
 * @author Siria
 * @brief Implementação do driver de Abstração SPIFFS com segurança RTOS.
 * * Este arquivo implementa as funções de inicialização, montagem, formatação e 
 * manipulação de arquivos (E/S) na memória Flash interna via partição SPIFFS,
 * contando com controle de concorrência por Mutex.
 * * @version 1.1
 * @date 2026-05-18
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_spiffs.h"
#include "spiffs_manager.h"

static char *TAG = "SPIFFS_MANAGER";

/**
 * @brief Inicializa o sistema de arquivos SPIFFS e cria o Mutex de proteção.
 */
esp_err_t spiffs_manager_init(spiffs_manager_handle_t *handle, spiffs_manager_config_t *config) {
    if (handle == NULL || config == NULL || config->path == NULL || config->part_label == NULL) {
        ESP_LOGE(TAG, "Argumentos invalidos na inicializacao");
        return ESP_ERR_INVALID_ARG;
    }

    handle->spiffs_config = *config;

    /**
     * @note Alocação de Recursos de RTOS:
     * Aloca o semáforo do tipo Mutex apenas se o ponteiro estiver limpo,
     * impedindo vazamento de memória (vazamento de heap) em reinicializações quentes.
     */
    if (handle->mutex == NULL) {
        handle->mutex = xSemaphoreCreateMutex();
        if (handle->mutex == NULL) {
            ESP_LOGE(TAG, "Falha ao criar Mutex");
            return ESP_ERR_NO_MEM;
        } 
    }

    esp_vfs_spiffs_conf_t config_spiffs_init = {
        .base_path = handle->spiffs_config.path,
        .partition_label = handle->spiffs_config.part_label,
        .max_files = handle->spiffs_config.mxfiles,
        .format_if_mount_failed = true 
    };

    /**
     * @note Registro no Virtual File System (VFS):
     * Associa a partição física da memória Flash mapeada no partitions.csv
     * ao ponto de montagem lógico do sistema operacional da ESP-IDF.
     */
    esp_err_t ret = esp_vfs_spiffs_register(&config_spiffs_init);
    if (ret != ESP_OK) {
        /**
         * @note Tratamento de Estado Residual:
         * Se a partição já estiver registrada devido a um ciclo anterior que 
         * não foi desalocado, o driver não destrói o Mutex operacional.
         */
        if (ret != ESP_ERR_INVALID_STATE) {
            vSemaphoreDelete(handle->mutex);
            handle->mutex = NULL;
            return ret;
        }
    }
    
    ESP_LOGI(TAG, "Spiffs inicializada com sucesso em %s", config->path);
    return ESP_OK;
}

/**
 * @brief Formata de maneira forçada a partição interna atrelada ao driver SPIFFS.
 */
esp_err_t spiffs_manager_format(spiffs_manager_handle_t *handle) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;

    if (xSemaphoreTake(handle->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    ESP_LOGW(TAG, "Formatando particao SPIFFS (%s)... Todos os dados salvos localmente serao perdidos!", handle->spiffs_config.part_label);
    
    esp_err_t ret = esp_spiffs_format(handle->spiffs_config.part_label);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro critico durante o processo de formatacao: %s", esp_err_to_name(ret));
        xSemaphoreGive(handle->mutex);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Particao formatada com sucesso.");
    xSemaphoreGive(handle->mutex);
    return ESP_OK;
}

/**
 * @brief Grava uma string no arquivo. 
 */
esp_err_t spiffs_manager_write(spiffs_manager_handle_t *handle, char *modo){
    if (!handle || !handle->spiffs_config.data || !handle->spiffs_config.namesp){
        return ESP_ERR_INVALID_ARG;
    }

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
esp_err_t spiffs_manager_read(spiffs_manager_handle_t *handle){
    if (!handle || !handle->spiffs_config.namesp) return ESP_ERR_INVALID_ARG;

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
        printf("[LOG SPIFFS] %s\n", linha); 
        
        /**
         * @note Controle de Escalonamento (Scheduling):
         * Cede voluntariamente o controle do núcleo por 10ms, evitando que
         * tarefas de leitura extensiva em logs causem starve no RTOS.
         */
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    fclose(f);
    xSemaphoreGive(handle->mutex);
    return ESP_OK;
}

/**
 * @brief Verifica de forma atômica se um determinado arquivo existe no diretório.
 */
esp_err_t spiffs_manager_exists(spiffs_manager_handle_t *handle, const char *namesp) {
    if (!handle || !namesp) {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(handle->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    struct stat st;
    if (stat(namesp, &st) == 0) {
        xSemaphoreGive(handle->mutex);
        return ESP_OK;
    }

    xSemaphoreGive(handle->mutex);
    return ESP_ERR_NOT_FOUND;
}

/**
 * @brief Exclui permanentemente um arquivo da partição SPIFFS.
 */
esp_err_t spiffs_manager_delete_file(spiffs_manager_handle_t *handle, const char *file_name) {
    if (!handle || !file_name) return ESP_ERR_INVALID_ARG;

    char caminho_completo[64];
    snprintf(caminho_completo, sizeof(caminho_completo), "%s", file_name);

    if (xSemaphoreTake(handle->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    struct stat st;
    if (stat(caminho_completo, &st) != 0) {
        xSemaphoreGive(handle->mutex);
        return ESP_ERR_NOT_FOUND;
    }

    if (remove(caminho_completo) != 0) {
        ESP_LOGE(TAG, "Erro interno de exclusao no arquivo: %s", caminho_completo);
        xSemaphoreGive(handle->mutex);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Arquivo removido do armazenamento: %s", caminho_completo);
    xSemaphoreGive(handle->mutex);
    return ESP_OK;
}

/**
 * @brief Desregistra o VFS e libera o Mutex.
 */
esp_err_t spiffs_manager_deinit(spiffs_manager_handle_t *handle) {
    if (handle && handle->mutex) {
        
        /**
         * @note Desalocação Controlada de Recursos:
         * Desfaz o mapeamento no VFS da ESP-IDF e limpa o Mutex do heap
         * antes de anular o ponteiro de runtime do handler.
         */
        esp_vfs_spiffs_unregister(handle->spiffs_config.part_label);
        vSemaphoreDelete(handle->mutex);
        handle->mutex = NULL;
        ESP_LOGI(TAG, "Recursos SPIFFS liberados.");
        return ESP_OK;
    }
    return ESP_ERR_INVALID_ARG;
}