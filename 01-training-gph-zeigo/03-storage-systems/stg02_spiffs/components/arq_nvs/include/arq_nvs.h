/**
 * @file arq_nvs.h
 * @author Siria
 * @brief Definições de estruturas e protótipos para o driver de abstração da NVS.
 * 
 * Este cabeçalho define a interface para manipulação simplificada da Non-Volatile Storage (NVS)
 * no ESP32, garantindo segurança em sistemas multitarefa através de Mutexes.
 * 
 * @version 1.1
 * @date 2026-05-12
 */

#ifndef ARQ_NVS_H
#define ARQ_NVS_H

#include <stdio.h>
#include <stdint.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Estrutura de configuração para o par chave-valor da NVS.
 * 
 * Contém as informações necessárias para localizar e tipar o dado na memória flash.
 */
typedef struct {
    const char* namesp; ///< Nome do compartimento (namespace) na NVS.
    const char* key;    ///< Chave identificadora do dado (máx 15 caracteres).
    void* data;         ///< Ponteiro para a variável que receberá ou enviará o dado.
    nvs_type_t type;    ///< Tipo do dado conforme o enumerador oficial da ESP-IDF (ex: NVS_TYPE_I8).
} arq_nvs_config_t;

/**
 * @brief Estrutura de controle (Handle) do driver.
 * 
 * Armazena o estado interno do driver, incluindo a configuração ativa e o 
 * semáforo de controle de acesso.
 */
typedef struct {
    arq_nvs_config_t nvs_config; ///< Cópia da configuração de usuário.
    nvs_handle_t nvs_handle;     ///< Handle interno de conexão com a API da ESP-IDF.
    SemaphoreHandle_t mutex;     ///< Mutex para garantir exclusão mútua entre tasks.
} arq_nvs_handle_t;

/**
 * @brief Inicializa o subsistema NVS e prepara o handle.
 * 
 * @param handle Ponteiro para a estrutura de controle que será inicializada.
 * @param config Ponteiro para as configurações de namespace, chave e tipo.
 * @return 
 *      - ESP_OK: Inicializado com sucesso.
 *      - ESP_ERR_INVALID_ARG: Se algum ponteiro for nulo.
 *      - ESP_ERR_NO_MEM: Se não houver memória para o Mutex.
 */
esp_err_t arq_nvs_init(arq_nvs_handle_t *handle, arq_nvs_config_t *config);

/**
 * @brief Persiste o valor da variável apontada na memória Flash.
 * 
 * @param handle Handle previamente inicializado.
 * @return 
 *      - ESP_OK: Dado gravado e comitado.
 *      - ESP_ERR_TIMEOUT: Falha ao obter o Mutex no tempo estipulado.
 */
esp_err_t arq_nvs_write(arq_nvs_handle_t *handle);

/**
 * @brief Recupera o valor da memória Flash para a variável apontada.
 * 
 * @param handle Handle previamente inicializado.
 * @return 
 *      - ESP_OK: Valor lido com sucesso.
 *      - ESP_ERR_NVS_NOT_FOUND: A chave ainda não existe na Flash.
 */
esp_err_t arq_nvs_read(arq_nvs_handle_t *handle);

/**
 * @brief Desaloca os recursos utilizados pelo driver (ex: Mutex).
 * 
 * @param handle Handle a ser finalizado.
 * @return esp_err_t ESP_OK em sucesso.
 */
esp_err_t arq_nvs_deinit(arq_nvs_handle_t *handle);

#ifdef __cplusplus
}
#endif

#endif /* ARQ_NVS_H */