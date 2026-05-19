/**
 * @file spiffs_manager.h
 * @author Siria
 * @brief Driver de abstração para o sistema de arquivos SPIFFS no ESP32.
 * * Este componente facilita a montagem de partições SPIFFS, leitura e escrita
 * de arquivos de texto (logs), utilizando Mutexes para garantir a integridade
 * dos dados em ambientes multitarefa (RTOS). Atua como o armazenamento local
 * secundário ou de failover.
 * * @version 1.0
 * @date 2026-05-14
 */

#ifndef SPIFFS_MANAGER_H
#define SPIFFS_MANAGER_H

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
    const char *data;       ///< Ponteiro para o buffer de dados constante (escrita ou leitura).
} spiffs_manager_config_t;

/**
 * @brief Estrutura de controle (Handler) do driver SPIFFS.
 */
typedef struct {
    spiffs_manager_config_t spiffs_config; ///< Configurações ativas do driver.
    SemaphoreHandle_t mutex;               ///< Mutex para proteção de acesso ao sistema de arquivos.
} spiffs_manager_handle_t;

/**
 * @brief Inicializa e monta a partição SPIFFS.
 * * @param handle Ponteiro para o handler que será inicializado.
 * @param config Configurações de montagem e caminhos.
 * @return 
 * - ESP_OK: Partição montada com sucesso.
 * - ESP_FAIL: Falha na montagem ou formatação.
 * - ESP_ERR_NO_MEM: Falha ao criar o Mutex.
 */
esp_err_t spiffs_manager_init(spiffs_manager_handle_t *handle, spiffs_manager_config_t *config);

/**
 * @brief Formata a partição SPIFFS associada ao handler.
 * * @param handle Handler do driver.
 * @return esp_err_t ESP_OK em caso de sucesso na formatação.
 */
esp_err_t spiffs_manager_format(spiffs_manager_handle_t *handle);

/**
 * @brief Escreve dados no arquivo especificado no handler.
 * * @param handle Handler do driver.
 * @param modo Modo de abertura do arquivo ("w" para sobrescrever, "a" para append).
 * @return esp_err_t ESP_OK em caso de sucesso.
 */
esp_err_t spiffs_manager_write(spiffs_manager_handle_t *handle, char *modo);

/**
 * @brief Lê o conteúdo do arquivo e o imprime no monitor serial linha por linha.
 * * @param handle Handler do driver.
 * @return esp_err_t ESP_OK se o arquivo foi lido corretamente.
 */
esp_err_t spiffs_manager_read(spiffs_manager_handle_t *handle);

/**
 * @brief Verifica se um determinado arquivo existe na partição montada.
 * * @param handle Handler do driver.
 * @param namesp Caminho completo do arquivo a ser verificado.
 * @return 
 * - ESP_OK: O arquivo existe.
 * - ESP_ERR_NOT_FOUND: O arquivo não foi localizado.
 */
esp_err_t spiffs_manager_exists(spiffs_manager_handle_t *handle, const char *namesp);

/**
 * @brief Remove fisicamente um arquivo da partição SPIFFS.
 * * @param handle Handler do driver.
 * @param file_name Caminho completo do arquivo a ser deletado.
 * @return esp_err_t ESP_OK em caso de sucesso na remoção.
 */
esp_err_t spiffs_manager_delete_file(spiffs_manager_handle_t *handle, const char *file_name);

/**
 * @brief Desmonta a partição SPIFFS e limpa os recursos de memória (Mutex).
 * * @param handle Handler a ser finalizado.
 * @return esp_err_t ESP_OK em sucesso.
 */
esp_err_t spiffs_manager_deinit(spiffs_manager_handle_t *handle);

#ifdef __cplusplus
}
#endif

#endif /* SPIFFS_MANAGER_H */