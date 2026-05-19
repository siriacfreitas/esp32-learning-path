/**
 * @file sd_manager.h
 * @author Siria
 * @brief Definições de estruturas e protótipos para o driver de gerenciamento de Cartão SD (FATFS).
 * * Este cabeçalho provê a interface para inicialização, montagem e manipulação de arquivos
 * em cartões SD usando a API de Virtual File System (VFS) FAT da ESP-IDF sobre o barramento SPI.
 * Inclui proteção por Mutex para garantir exclusão mútua em sistemas RTOS.
 * * @version 1.0
 * @date 2026-05-18
 */

#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include "spi_dev.h" 

#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif 

/**
 * @brief Estrutura de configuração para o Driver do Cartão SD.
 * * Concentra os parâmetros necessários para inicializar o barramento SPI periférico
 * e mapear o arquivo alvo no sistema de arquivos FAT.
 */
typedef struct {
    spi_dev_handle_t spi;      ///< Handle de configuração do barramento SPI (MISO/MOSI/CLK)
    gpio_num_t cs_pin;         ///< Pino GPIO de Chip Select (CS) para o escravo SPI
    int mxfiles;               ///< Número máximo de arquivos abertos de forma concorrente
    size_t clusters;           ///< Tamanho do cluster de alocação para formatação FAT
    const char *file_name;     ///< Caminho completo do arquivo alvo (ex: "/sdcard/log.txt")
    const char *data;          ///< Ponteiro para a string ou buffer de dados de leitura/escrita
} sd_manager_conif_t;

/**
 * @brief Handler de controle operacional do Cartão SD.
 * * Armazena as referências de runtime do driver, garantindo a integridade do barramento
 * e do hardware conectado.
 */
typedef struct {
    sd_manager_conif_t sd_config; ///< Cópia local das configurações passadas pelo usuário
    SemaphoreHandle_t mutex;       ///< Mutex para segurança de acesso concorrente entre tarefas
    sdmmc_card_t *card;            ///< Ponteiro para a estrutura interna do cartão alocada pela ESP-IDF
} sd_manager_handle_t;


/**
 * @brief Inicializa a periferia SPI e aloca os recursos de concorrência do driver.
 * * @param handle Ponteiro para o handler que será populado.
 * @param config Parâmetros de configuração de hardware e software.
 * @return 
 * - ESP_OK: Recursos e Mutex criados com sucesso.
 * - ESP_ERR_NO_MEM: Memória insuficiente para criar o Mutex.
 */
esp_err_t sd_manager_init(sd_manager_handle_t *handle, sd_manager_conif_t *config);

/**
 * @brief Realiza a montagem física do sistema de arquivos FAT do Cartão SD.
 * * É utilizado no boot e pelo algoritmo de failover para checar se o cartão foi reinserido.
 * * @param handle Handler operacional.
 * @return esp_err_t ESP_OK em caso de sucesso na montagem do VFS.
 */
esp_err_t sd_manager_mount(sd_manager_handle_t *handle);

/**
 * @brief Formata a partição do Cartão SD usando o sistema de arquivos FAT.
 * * @param handle Handler operacional.
 * @return esp_err_t ESP_OK se a formatação foi concluída com sucesso.
 */
esp_err_t sd_manager_format(sd_manager_handle_t *handle);

/**
 * @brief Escreve ou adiciona dados ao arquivo configurado no handler.
 * * @param handle Handler operacional.
 * @param modo Modo de abertura padrão do C (ex: "w" para escrita limpa, "a" para append).
 * @return 
 * - ESP_OK: Dados escritos fisicamente no cartão.
 * - ESP_ERR_TIMEOUT: O Mutex estava ocupado por outra tarefa.
 */
esp_err_t sd_manager_write_file(sd_manager_handle_t *handle, char *modo);

/**
 * @brief Lê o arquivo alvo de forma linear e despeja o conteúdo no monitor serial.
 * * @param handle Handler operacional.
 * @return esp_err_t ESP_OK se o arquivo foi aberto e lido por completo.
 */
esp_err_t sd_manager_read_file(sd_manager_handle_t *handle);

/**
 * @brief Remove fisicamente o arquivo configurado do Cartão SD.
 * * @param handle Handler operacional.
 * @return esp_err_t ESP_OK se o arquivo foi deletado com sucesso.
 */
esp_err_t sd_manager_delete_file(sd_manager_handle_t *handle);

/**
 * @brief Recupera informações de hardware do cartão (Tamanho, Fabricante, Capacidade).
 * * @param handle Handler operacional.
 * @return esp_err_t ESP_OK se as informações foram impressas com sucesso.
 */
esp_err_t sd_manager_get_info(sd_manager_handle_t *handle);

/**
 * @brief Verifica se um arquivo específico existe no diretório montado.
 * * @param handle Handler operacional.
 * @param file_name Nome ou caminho do arquivo a ser verificado.
 * @return 
 * - ESP_OK: O arquivo existe.
 * - ESP_ERR_NOT_FOUND: O arquivo não foi localizado.
 */
esp_err_t sd_manager_exists(sd_manager_handle_t *handle, const char *file_name);


#ifdef __cplusplus
}
#endif

#endif /* SD_MANAGER_H */