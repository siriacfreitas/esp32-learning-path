/**
 * @file arq_nvs.c
 * @author Siria
 * @brief Implementação do driver para abstração da memória não volátil (NVS) com suporte a Mutex.
 * 
 * Este driver facilita o armazenamento de dados persistentes no ESP32, lidando com 
 * a abertura, fechamento, commit e proteção de concorrência automaticamente.
 * 
 * @version 1.1
 * @date 2026-05-12
 */

#include "arq_nvs.h"
#include "esp_log.h"

#define TAG "ARQ_NVS" 

/**
 * @brief Macro para liberar o mutex e reportar erros de forma padronizada.
 * 
 * @param err Erro retornado pela função NVS.
 * @param mutex O semáforo a ser liberado.
 * @param msg Mensagem de erro customizada para o log.
 */
#define CHECK_AND_RELEASE(err, mutex, msg) if (err != ESP_OK) { \
    ESP_LOGE(TAG, "%s: %s", msg, esp_err_to_name(err)); \
    xSemaphoreGive(mutex); \
    return err; \
}

/**
 * @brief Inicializa a memória flash e prepara o handle do driver.
 * 
 * @param handle Ponteiro para a estrutura de controle do driver.
 * @param config Configurações de namespace, chave e tipo de dado.
 * @return esp_err_t ESP_OK em caso de sucesso.
 */
esp_err_t arq_nvs_init(arq_nvs_handle_t *handle, arq_nvs_config_t *config) {
    if (handle == NULL || config == NULL || config->namesp == NULL || config->key == NULL) {
        ESP_LOGE(TAG, "Argumentos invalidos na inicializacao");
        return ESP_ERR_INVALID_ARG;
    }

    /** @step 1: Inicializa o subsistema de hardware da NVS Flash. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS corrompida, apagando...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) return err;

    /** @step 2: Configuração do Handle e criação do Mutex para segurança RTOS. */
    handle->nvs_config = *config;
    handle->mutex = xSemaphoreCreateMutex();

    if (handle->mutex == NULL) {
        ESP_LOGE(TAG, "Falha ao criar Mutex");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "NVS inicializado. Namespace: %s, Key: %s", config->namesp, config->key);
    return ESP_OK;
}

/**
 * @brief Grava o valor atual da variável vinculada na memória Flash.
 * 
 * @param handle Handle inicializado.
 * @return esp_err_t ESP_OK se gravado e comitado com sucesso.
 */
esp_err_t arq_nvs_write(arq_nvs_handle_t *handle) {
    if (!handle || !handle->nvs_config.data){
         return ESP_ERR_INVALID_ARG;
    }

    /** @note Tenta obter o Mutex para evitar acesso simultâneo de outras tasks. */
    if (xSemaphoreTake(handle->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t err = nvs_open(handle->nvs_config.namesp, NVS_READWRITE, &handle->nvs_handle);
    CHECK_AND_RELEASE(err, handle->mutex, "Erro ao abrir NVS para escrita");

    /** @section Escrita baseada no tipo configurado (Switch Case). */
    switch (handle->nvs_config.type) {
        case NVS_TYPE_I8:  err = nvs_set_i8(handle->nvs_handle,  handle->nvs_config.key, *(int8_t*)handle->nvs_config.data); break;
        case NVS_TYPE_U8:  err = nvs_set_u8(handle->nvs_handle,  handle->nvs_config.key, *(uint8_t*)handle->nvs_config.data); break;
        case NVS_TYPE_I16: err = nvs_set_i16(handle->nvs_handle, handle->nvs_config.key, *(int16_t*)handle->nvs_config.data); break;
        case NVS_TYPE_U16: err = nvs_set_u16(handle->nvs_handle, handle->nvs_config.key, *(uint16_t*)handle->nvs_config.data); break;
        case NVS_TYPE_I32: err = nvs_set_i32(handle->nvs_handle, handle->nvs_config.key, *(int32_t*)handle->nvs_config.data); break;
        case NVS_TYPE_U32: err = nvs_set_u32(handle->nvs_handle, handle->nvs_config.key, *(uint32_t*)handle->nvs_config.data); break;
        default: err = ESP_ERR_INVALID_ARG; break;
    }

    if (err == ESP_OK) {
        nvs_commit(handle->nvs_handle); ///< Garante a persistência física dos dados.
        ESP_LOGD(TAG, "Dado gravado com sucesso");
    }

    nvs_close(handle->nvs_handle);
    xSemaphoreGive(handle->mutex); ///< Libera o driver para outras tarefas.
    return err;
}

/**
 * @brief Lê o valor da Flash e armazena na variável vinculada ao handle.
 * 
 * @param handle Handle inicializado.
 * @return esp_err_t ESP_OK ou ESP_ERR_NVS_NOT_FOUND se a chave for nova.
 */
esp_err_t arq_nvs_read(arq_nvs_handle_t *handle) {
    if (!handle || !handle->nvs_config.data) return ESP_ERR_INVALID_ARG;

    if (xSemaphoreTake(handle->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t err = nvs_open(handle->nvs_config.namesp, NVS_READONLY, &handle->nvs_handle);
    CHECK_AND_RELEASE(err, handle->mutex, "Erro ao abrir NVS para leitura");

    /** @section Leitura e Type Casting automático conforme configuração. */
    switch (handle->nvs_config.type) {
        case NVS_TYPE_I8:  err = nvs_get_i8(handle->nvs_handle,  handle->nvs_config.key, (int8_t*)handle->nvs_config.data); break;
        case NVS_TYPE_U8:  err = nvs_get_u8(handle->nvs_handle,  handle->nvs_config.key, (uint8_t*)handle->nvs_config.data); break;
        case NVS_TYPE_I16: err = nvs_get_i16(handle->nvs_handle, handle->nvs_config.key, (int16_t*)handle->nvs_config.data); break;
        case NVS_TYPE_U16: err = nvs_get_u16(handle->nvs_handle, handle->nvs_config.key, (uint16_t*)handle->nvs_config.data); break;
        case NVS_TYPE_I32: err = nvs_get_i32(handle->nvs_handle, handle->nvs_config.key, (int32_t*)handle->nvs_config.data); break;
        case NVS_TYPE_U32: err = nvs_get_u32(handle->nvs_handle, handle->nvs_config.key, (uint32_t*)handle->nvs_config.data); break;
        default: err = ESP_ERR_INVALID_ARG; break;
    }

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Chave '%s' nao encontrada", handle->nvs_config.key);
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro na leitura: %s", esp_err_to_name(err));
    }

    nvs_close(handle->nvs_handle);
    xSemaphoreGive(handle->mutex);
    return err;
}

/**
 * @brief Finaliza o driver e libera os recursos de memória (Mutex).
 * 
 * @param handle Handle a ser desativado.
 * @return esp_err_t ESP_OK em sucesso.
 */
esp_err_t arq_nvs_deinit(arq_nvs_handle_t *handle) {
    if (handle && handle->mutex) {
        vSemaphoreDelete(handle->mutex);
        handle->mutex = NULL;
        ESP_LOGI(TAG, "Recursos liberados.");
        return ESP_OK;
    }
    return ESP_ERR_INVALID_ARG;
}