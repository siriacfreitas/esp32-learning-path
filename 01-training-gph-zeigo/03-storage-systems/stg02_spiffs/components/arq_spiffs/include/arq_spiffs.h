/**
 * @file arq_spiffs.h
 * @author Siria
 * @brief Driver de abstração para o sistema de arquivos SPIFFS no ESP32.
 * 
 * Este componente facilita a montagem de partições SPIFFS, leitura e escrita
 * de arquivos de texto (logs), utilizando Mutexes para garantir a integridade
 * dos dados em ambientes multitarefa (RTOS).
 * 
 * @version 1.0
 * @date 2026-05-14
 */

#ifndef ARQ_SPIFFS
#define ARQ_SPIFFS

#include "esp_spiffs.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Estrutura de configuração para a partição SPIFFS.
 */
typedef struct {
    const char *path;       ///< Ponto de montagem no VFS (ex: "/spiffs").
    const char *part_label; ///< Label da partição definida no partitions.csv (ex: "storage").
    size_t mxfiles;         ///< Número máximo de arquivos abertos simultaneamente.
    const char *namesp;     ///< Caminho completo do arquivo alvo (ex: "/spiffs/log.txt").
    char *data;             ///< Ponteiro para o buffer de dados (escrita ou leitura).
} arq_spiffs_config_t;

/**
 * @brief Estrutura de controle (Handler) do driver SPIFFS.
 */
typedef struct {
    arq_spiffs_config_t spiffs_config; ///< Configurações ativas do driver.
    SemaphoreHandle_t mutex;           ///< Mutex para proteção de acesso ao sistema de arquivos.
} arq_spiffs_handle_t;

/**
 * @brief Inicializa e monta a partição SPIFFS.
 * 
 * @param handle Ponteiro para o handler que será inicializado.
 * @param config Configurações de montagem e caminhos.
 * @return 
 *      - ESP_OK: Partição montada com sucesso.
 *      - ESP_FAIL: Falha na montagem ou formatação.
 *      - ESP_ERR_NO_MEM: Falha ao criar o Mutex.
 */
esp_err_t arq_spiffs_init(arq_spiffs_handle_t *handle, arq_spiffs_config_t *config);

/**
 * @brief Escreve dados no arquivo especificado no handler.
 * 
 * @param handle Handler do driver.
 * @param modo Modo de abertura do arquivo ("w" para sobrescrever, "a" para append).
 * @return esp_err_t ESP_OK em caso de sucesso.
 */
esp_err_t arq_spiffs_write(arq_spiffs_handle_t *handle, char *modo);

/**
 * @brief Lê o conteúdo do arquivo e o imprime no log ou armazena no buffer.
 * 
 * @param handle Handler do driver.
 * @return esp_err_t ESP_OK se o arquivo foi lido corretamente.
 */
esp_err_t arq_spiffs_read(arq_spiffs_handle_t *handle);

/**
 * @brief Desmonta a partição SPIFFS e limpa os recursos de memória.
 * 
 * @param handle Handler a ser finalizado.
 * @return esp_err_t ESP_OK em sucesso.
 */
esp_err_t arq_spiffs_deinit(arq_spiffs_handle_t *handle);

#ifdef __cplusplus
}
#endif

#endif /* ARQ_SPIFFS */