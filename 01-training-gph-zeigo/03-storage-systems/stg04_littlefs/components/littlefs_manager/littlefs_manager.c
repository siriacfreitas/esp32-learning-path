/**
 * @file littlefs_manager.c
 * @brief Implementação do gerenciador Thread-Safe para o sistema de arquivos LittleFS.
 * @version 1.0
 * @date 2026-06-02
 * @copyright Copyright (c) 2026
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

#include "esp_littlefs.h"
#include "littlefs_manager.h"

/**
 * @brief Tag constante para o identificador de logs do componente.
 * Mapeia as mensagens de depuração emitidas no monitor serial.
 */
static const char *TAG = "LITTLEFS_MANAGER";

/**
 * @brief Inicializa o sistema de arquivos LittleFS e aloca recursos do componente.
 * * @param[out] handle Ponteiro para a estrutura de controle que será alocada/configurada.
 * @param[in] config Ponteiro para as diretrizes de configuração do sistema de arquivos.
 * @return 
 * - ESP_OK: Inicialização e registro no VFS realizados com sucesso.
 * - ESP_ERR_INVALID_ARG: Configurações ausentes ou ponteiros nulos.
 * - ESP_ERR_NO_MEM: Falha ao alocar o Mutex no Heap.
 */
esp_err_t littlefs_manager_init(littlefs_manager_handle_t *handle, littlefs_manager_config_t *config) {
    if (handle == NULL || config == NULL || config->path == NULL || config->part_label == NULL) {
        ESP_LOGE(TAG, "Argumentos invalidos na inicializacao");
        return ESP_ERR_INVALID_ARG;
    }

    handle->littlefs_config = *config;

    /* Criação do Mutex se não houver um alocado */
    if (handle->mutex == NULL) {
        handle->mutex = xSemaphoreCreateMutex();
        if (handle->mutex == NULL) {
            ESP_LOGE(TAG, "Falha ao criar Mutex");
            return ESP_ERR_NO_MEM;
        } 
    }

    /* Estrutura interna exigida pelo driver esp_littlefs */
    esp_vfs_littlefs_conf_t config_littlefs_init = {
        .base_path = handle->littlefs_config.path,
        .partition_label = handle->littlefs_config.part_label,
        .format_if_mount_failed = true 
    };

    /* Registro do VFS no ecossistema ESP-IDF */
    esp_err_t ret = esp_vfs_littlefs_register(&config_littlefs_init);
    if (ret != ESP_OK) {
        if (ret != ESP_ERR_INVALID_STATE) {
            vSemaphoreDelete(handle->mutex);
            handle->mutex = NULL;
            return ret;
        }
    }
    
    ESP_LOGI(TAG, "LittleFS inicializada com sucesso em %s", config->path);
    return ESP_OK;
}

/**
 * @brief Formata de maneira forçada a partição interna atrelada ao driver littlefs.
 * * @param[in] handle Ponteiro para a estrutura de controle do componente.
 * @return 
 * - ESP_OK: Operação de formatação bem-sucedida.
 * - ESP_ERR_INVALID_ARG: Handle inválido ou não inicializado.
 * - ESP_ERR_TIMEOUT: Falha ao obter a tranca do Mutex.
 * - ESP_FAIL: Falha interna no hardware/partição.
 */
esp_err_t littlefs_manager_format(littlefs_manager_handle_t *handle) {
    if (handle == NULL) return ESP_ERR_INVALID_ARG;

    if (xSemaphoreTake(handle->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    ESP_LOGW(TAG, "Formatando particao littlefs (%s)... Todos os dados salvos localmente serao perdidos!", handle->littlefs_config.part_label);
    
    esp_err_t ret = esp_littlefs_format(handle->littlefs_config.part_label);
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
 * @brief Grava o conteúdo presente em `config.data` no arquivo definido em `config.namesp`.
 * * @param[in] handle Ponteiro para a estrutura de controle do componente.
 * @param[in] modo String definindo o modo de IO do arquivo (ex: "w" para escrita pura, "a" para append).
 * @return 
 * - ESP_OK: Escrita realizada com sucesso.
 * - ESP_ERR_INVALID_ARG: Parâmetros de buffer ou caminhos inválidos.
 * - ESP_ERR_TIMEOUT: Estouro de tempo ao tentar obter o Mutex.
 * - ESP_FAIL: Falha ao abrir o arquivo físico.
 */
esp_err_t littlefs_manager_write(littlefs_manager_handle_t *handle, char *modo){
    if (!handle || !handle->littlefs_config.data || !handle->littlefs_config.namesp){
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(handle->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    FILE *f = fopen(handle->littlefs_config.namesp, modo);
    if (f == NULL){
        ESP_LOGE(TAG, "Falha ao abrir o arquivo para escrita: %s", handle->littlefs_config.namesp);
        xSemaphoreGive(handle->mutex);
        return ESP_FAIL;
    }

    fprintf(f, "%s\n", handle->littlefs_config.data);
    fclose(f);
    
    xSemaphoreGive(handle->mutex);
    return ESP_OK;
}

/**
 * @brief Lê o conteúdo do arquivo definido em `config.namesp` linha por linha, jogando no console.
 * * @param[in] handle Ponteiro para a estrutura de controle do componente.
 * @return 
 * - ESP_OK: Leitura completa realizada sem interrupções.
 * - ESP_ERR_INVALID_ARG: Componente inválido ou sem referências de arquivo.
 * - ESP_ERR_TIMEOUT: Falha de sincronismo no Mutex.
 * - ESP_ERR_NOT_FOUND: Arquivo inexistente no VFS.
 */
esp_err_t littlefs_manager_read(littlefs_manager_handle_t *handle){
    if (!handle || !handle->mutex || !handle->littlefs_config.namesp) {
        return ESP_ERR_INVALID_ARG; 
    }

    if (xSemaphoreTake(handle->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    FILE *f = fopen(handle->littlefs_config.namesp, "r");
    if (f == NULL){
        ESP_LOGW(TAG, "Arquivo nao encontrado ou vazio.");
        xSemaphoreGive(handle->mutex);
        return ESP_ERR_NOT_FOUND;
    }

    char linha[128];
    while (fgets(linha, sizeof(linha), f) != NULL) {
        linha[strcspn(linha, "\n")] = 0;
        printf("[LOG littlefs] %s\n", linha); 
        
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
 * * @param[in] handle Ponteiro para a estrutura de controle do componente.
 * @param[in] namesp Caminho absoluto do arquivo para checagem estrutural.
 * @return 
 * - ESP_OK: Arquivo encontrado.
 * - ESP_ERR_INVALID_ARG: Entradas inválidas passadas à função.
 * - ESP_ERR_TIMEOUT: Estouro de tempo no Mutex.
 * - ESP_ERR_NOT_FOUND: Arquivo não localizado na partição.
 */
esp_err_t littlefs_manager_exists(littlefs_manager_handle_t *handle, const char *namesp) {
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
 * @brief Exclui permanentemente um arquivo da partição littlefs.
 * * @param[in] handle Ponteiro para a estrutura de controle do componente.
 * @param[in] file_name Caminho absoluto do arquivo a ser deletado da flash.
 * @return 
 * - ESP_OK: Arquivo apagado e removido do mapa de alocação.
 * - ESP_ERR_INVALID_ARG: Erro de parâmetros nulos.
 * - ESP_ERR_TIMEOUT: Falha ao requisitar o Mutex.
 * - ESP_ERR_NOT_FOUND: Arquivo solicitado não existe.
 * - ESP_FAIL: Falha interna do sistema operacional de arquivos.
 */
esp_err_t littlefs_manager_delete_file(littlefs_manager_handle_t *handle, const char *file_name) {
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
 * @brief Desregistra o VFS e libera o Mutex de sincronismo.
 * * @param[in,out] handle Ponteiro para a estrutura de controle que será destruída.
 * @return 
 * - ESP_OK: Recursos liberados de forma limpa.
 * - ESP_ERR_INVALID_ARG: O handle informado já se encontra desalocado ou nulo.
 */
esp_err_t littlefs_manager_deinit(littlefs_manager_handle_t *handle) {
    if (handle && handle->mutex) {
        
        /**
         * @note Desalocação Controlada de Recursos:
         * Desfaz o mapeamento no VFS da ESP-IDF e limpa o Mutex do heap
         * antes de anular o ponteiro de runtime do handler.
         */
        esp_vfs_littlefs_unregister(handle->littlefs_config.part_label);
        vSemaphoreDelete(handle->mutex);
        handle->mutex = NULL;
        ESP_LOGI(TAG, "Recursos littlefs liberados.");
        return ESP_OK;
    }
    return ESP_ERR_INVALID_ARG;
}