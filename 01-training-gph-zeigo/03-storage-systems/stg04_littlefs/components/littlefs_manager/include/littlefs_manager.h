/**
 * @file littlefs_manager.h
 * @brief Gerenciador de sistema de arquivos LittleFS para ESP-IDF com suporte a concorrência (Thread-Safe).
 * @version 1.0
 * @date 2026-06-02
 * * @copyright Copyright (c) 2026
 */

#ifndef LITTLERFS_MANAGER_H
#define LITTLERFS_MANAGER_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Estrutura de configuração para o LittleFS Manager.
 * * Contém os parâmetros necessários para mapeamento do VFS e manipulação de arquivos.
 */
typedef struct {
    const char *path;        /**< Ponto de montagem base no VFS (ex: "/littlefs"). */
    const char *part_label;  /**< Label da partição definido no partitions.csv (ex: "storage"). */
    const char *namesp;      /**< Caminho absoluto do arquivo alvo para operações (ex: "/littlefs/log.txt"). */
    const char *data;        /**< Ponteiro para buffer de dados (utilizado em operações de escrita). */
} littlefs_manager_config_t;

/**
 * @brief Estrutura de controle (Handle) do LittleFS Manager.
 * * Mantém o estado de execução, cópia da configuração e o semáforo de exclusão mútua.
 */
typedef struct {
    littlefs_manager_config_t littlefs_config; /**< Cópia local da configuração ativa. */
    SemaphoreHandle_t mutex;                   /**< Mutex do FreeRTOS para garantir operações Thread-Safe. */
} littlefs_manager_handle_t;

/**
 * @brief Inicializa o sistema de arquivos LittleFS e aloca recursos do componente.
 * * Registra o VFS na ESP-IDF baseado nas configurações fornecidas e cria o Mutex interno.
 * * @param[out] handle Ponteiro para a estrutura de controle que será inicializada.
 * @param[in] config Ponteiro para as diretrizes de configuração do sistema de arquivos.
 * * @return 
 * - ESP_OK: Inicialização realizada com sucesso.
 * - ESP_ERR_INVALID_ARG: Argumentos nulos ou strings de configuração inválidas.
 * - ESP_ERR_NO_MEM: Falha ao alocar o Mutex no Heap.
 * - Outros códigos de erro retornados por esp_vfs_littlefs_register().
 */
esp_err_t littlefs_manager_init(littlefs_manager_handle_t *handle, littlefs_manager_config_t *config);

/**
 * @brief Formata de maneira forçada a partição física do LittleFS.
 * * @note Atenção! Todos os arquivos salvos localmente nessa partição serão perdidos permanentemente.
 * * @param[in] handle Ponteiro para a estrutura de controle do componente.
 * * @return 
 * - ESP_OK: Partição formatada com sucesso.
 * - ESP_ERR_INVALID_ARG: Handle ou Mutex inválido (componente desinicializado).
 * - ESP_ERR_TIMEOUT: Estouro de tempo ao tentar obter o Mutex.
 * - ESP_FAIL: Erro interno reportado pelo driver físico de formatação.
 */
esp_err_t littlefs_manager_format(littlefs_manager_handle_t *handle);

/**
 * @brief Grava o conteúdo presente em `config.data` no arquivo definido em `config.namesp`.
 * * @param[in] handle Ponteiro para a estrutura de controle do componente.
 * @param[in] modo String definindo o modo de abertura do arquivo (ex: "w" para sobrescrever, "a" para append).
 * * @return 
 * - ESP_OK: Dados escritos com sucesso.
 * - ESP_ERR_INVALID_ARG: Parâmetros nulos ou dados/caminhos ausentes.
 * - ESP_ERR_TIMEOUT: Estouro de tempo ao tentar obter o Mutex.
 * - ESP_FAIL: Falha ao abrir ou fechar o arquivo físico.
 */
esp_err_t littlefs_manager_write(littlefs_manager_handle_t *handle, char *modo);

/**
 * @brief Lê o conteúdo do arquivo definido em `config.namesp` linha por linha, jogando no console.
 * * @note Esta função implementa controle de escalonamento (`vTaskDelay`) interno para evitar
 * bloqueio excessivo da CPU durante leituras massivas de logs.
 * * @param[in] handle Ponteiro para a estrutura de controle do componente.
 * * @return 
 * - ESP_OK: Leitura processada até o fim do arquivo.
 * - ESP_ERR_INVALID_ARG: Handle ou caminho de arquivo nulo.
 * - ESP_ERR_TIMEOUT: Estouro de tempo ao tentar obter o Mutex.
 * - ESP_ERR_NOT_FOUND: Arquivo inexistente ou vazio.
 */
esp_err_t littlefs_manager_read(littlefs_manager_handle_t *handle);

/**
 * @brief Verifica de forma atômica se um arquivo específico existe na partição.
 * * @param[in] handle Ponteiro para a estrutura de controle do componente.
 * @param[in] namesp Caminho absoluto do arquivo a ser verificado (ex: "/littlefs/meu_arquivo.txt").
 * * @return 
 * - ESP_OK: O arquivo existe no diretório atual.
 * - ESP_ERR_INVALID_ARG: Parâmetros de entrada inválidos ou nulos.
 * - ESP_ERR_TIMEOUT: Estouro de tempo ao tentar obter o Mutex.
 * - ESP_ERR_NOT_FOUND: O arquivo informado não foi localizado.
 */
esp_err_t littlefs_manager_exists(littlefs_manager_handle_t *handle, const char *namesp);

/**
 * @brief Exclui permanentemente um arquivo mapeado no sistema.
 * * @param[in] handle Ponteiro para a estrutura de controle do componente.
 * @param[in] file_name Caminho absoluto do arquivo que será removido.
 * * @return 
 * - ESP_OK: Arquivo desvinculado e deletado com sucesso.
 * - ESP_ERR_INVALID_ARG: Parâmetros nulos ou inválidos.
 * - ESP_ERR_TIMEOUT: Estouro de tempo ao tentar obter o Mutex.
 * - ESP_ERR_NOT_FOUND: Arquivo não localizado para deleção.
 * - ESP_FAIL: Falha interna no driver ao tentar remover o arquivo do VFS.
 */
esp_err_t littlefs_manager_delete_file(littlefs_manager_handle_t *handle, const char *file_name);

/**
 * @brief Desregistra a partição no VFS da ESP-IDF e limpa os recursos de concorrência.
 * * Libera o Mutex alocado do heap e anula os ponteiros internos, prevenindo Memory Leak.
 * * @param[in,out] handle Ponteiro para a estrutura de controle a ser desativada.
 * * @return 
 * - ESP_OK: Recursos liberados e driver desativado com sucesso.
 * - ESP_ERR_INVALID_ARG: Handle inválido ou já desinicializado previamente.
 */
esp_err_t littlefs_manager_deinit(littlefs_manager_handle_t *handle);

#ifdef __cplusplus
}
#endif

#endif /* LITTLERFS_MANAGER_H */