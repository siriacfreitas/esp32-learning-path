/**
 * @file spi_dev.h
 * @author Siria
 * @brief Definições de estruturas e protótipos para a camada de abstração do barramento SPI (Low-Level).
 * * Este cabeçalho provê a interface de inicialização genérica do periférico SPI
 * do ESP32. Funciona como a camada base de hardware sobre a qual drivers de alto
 * nível (como o sd_manager) realizam suas transações de dados.
 * * @version 1.0
 * @date 2026-05-18
 */

#ifndef SPI_DEV_H
#define SPI_DEV_H

#include "driver/gpio.h"
#include "driver/spi_common.h"

#ifdef __cplusplus
extern "C" {
#endif 

/**
 * @brief Estrutura de hardware pura para mapeamento dos pinos lógicos do SPI.
 */
typedef struct {
    gpio_num_t mosi_pin; ///< Master Out Slave In - Linha de transmissão de dados
    gpio_num_t miso_pin; ///< Master In Slave Out - Linha de recepção de dados
    gpio_num_t sck_pin;  ///< Serial Clock - Linha de sincronismo temporal
} spi_dev_conif_t;

/**
 * @brief Handler operacional de controle do barramento SPI.
 * * Agrupa as configurações de pinagem, alocação do canal de DMA e identificação
 * do periférico de hardware interno do chip do ESP32 (ex: SPI2_HOST/SPI3_HOST).
 */
typedef struct {
    spi_dev_conif_t spi_config;   ///< Cópia local das configurações de pinagem informadas pelo usuário
    spi_dma_chan_t spi_dma;       ///< Configuração do canal de Direct Memory Access (DMA) associado
    spi_host_device_t spi_device; ///< Identificador do host físico do periférico no SoC (Slot)
} spi_dev_handle_t;


/**
 * @brief Inicializa o barramento Host SPI barramento de baixo nível da ESP-IDF.
 * * Configura os pinos físicos e aloca os recursos de DMA do hardware interno do ESP32.
 * * @param handle Ponteiro para o handler operacional que gerencia o ciclo de vida do periférico.
 * @param config Parâmetros específicos de pinagem física das linhas de dados e clock.
 * @return 
 * - ESP_OK: Barramento inicializado e alocado com sucesso.
 * - ESP_ERR_INVALID_ARG: Ponteiros nulos passados como argumento.
 * - ESP_ERR_INVALID_STATE: O host SPI selecionado já está em uso por outro periférico.
 */
esp_err_t spi_dev_init(spi_dev_handle_t *handle, spi_dev_conif_t *config);


#ifdef __cplusplus
}
#endif

#endif /* SPI_DEV_H */