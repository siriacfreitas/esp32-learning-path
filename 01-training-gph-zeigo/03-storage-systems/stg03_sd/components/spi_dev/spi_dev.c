/**
 * @file spi_dev.c
 * @author Siria
 * @brief Implementação da camada de abstração de baixo nível para o barramento SPI do ESP32.
 * * Este arquivo contém as validações estritas de hardware e a chamada de inicialização
 * do subsistema de barramento SPI (SPI Bus) nativo da ESP-IDF, configurando os pinos
 * de sinal e o canal de DMA.
 * * @version 1.0
 * @date 2026-05-18
 */

#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "spi_dev.h"

static char *TAG_INIT = "SPI_DEV";

/**
 * @brief Inicializa o barramento Host SPI de baixo nível da ESP-IDF.
 */
esp_err_t spi_dev_init(spi_dev_handle_t *handle, spi_dev_conif_t *config){
    
    /**
     * @note 1. Validação de segurança de software:
     * Garante que os ponteiros de configuração e de contexto operacional 
     * não sejam nulos antes de realizar qualquer acesso à memória.
     */
    if(!handle || !config){
        return ESP_ERR_INVALID_ARG;
    }

    /**
     * @note 2. Validação física dos pinos GPIO:
     * Verifica através de macros nativas se os pinos selecionados são mapeáveis 
     * e fisicamente válidos dentro da matriz de IO da arquitetura do SoC do ESP32.
     */
    if (!(GPIO_IS_VALID_GPIO(config->mosi_pin) && 
          GPIO_IS_VALID_GPIO(config->miso_pin) && 
          GPIO_IS_VALID_GPIO(config->sck_pin))){
        ESP_LOGE(TAG_INIT, "Pinos GPIO inválidos para a arquitetura");
        return ESP_ERR_INVALID_ARG;
    }

    /**
     * @note 3. Verificação de curto-circuito lógico / duplicidade:
     * Lógica defensiva para impedir que o mesmo pino físico de GPIO seja assinalado 
     * para mais de uma função distinta no barramento síncrono (ex: MOSI e SCK no mesmo pino).
     */
    if (config->mosi_pin == config->miso_pin || config->mosi_pin == config->sck_pin || config->miso_pin == config->sck_pin){
        ESP_LOGE(TAG_INIT, "Erro: Existe duplicidade nos pinos GPIO atribuídos");
        return ESP_ERR_INVALID_ARG;
    }

    handle->spi_config = *config;
    
    /**
     * @note Configuração da estrutura nativa da ESP-IDF:
     * Mapeia as linhas de transmissão e desabilita os modos de amostragem paralela 
     * avançados (Quad-SPI: WP e HD) setando-os como -1, já que operamos em modo SPI padrão.
     */
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = handle->spi_config.mosi_pin,
        .miso_io_num = handle->spi_config.miso_pin,
        .sclk_io_num = handle->spi_config.sck_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };

    /**
     * @note Alocação de hardware no Driver de Barramento:
     * Registra o periférico SPI no sistema operacional, aplicando os mapeamentos e
     * ligando o canal de acesso direto à memória (DMA) para transações de alta velocidade.
     */
    esp_err_t ret = spi_bus_initialize(handle->spi_device, &bus_cfg, handle->spi_dma);
    if (ret != ESP_OK){
        ESP_LOGE(TAG_INIT, "Falha no spi_bus_initialize: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    return ESP_OK;
}