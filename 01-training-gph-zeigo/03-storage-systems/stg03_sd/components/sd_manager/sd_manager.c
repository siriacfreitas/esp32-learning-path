/**
 * @file sd_manager.c
 * @brief Implementação do gerenciador do cartão SD e sistema de arquivos FAT para ESP32.
 * * Este arquivo implementa as funções de inicialização, montagem, formatação e 
 * manipulação de arquivos (E/S) em cartões SD, utilizando o barramento SPI e a 
 * interface VFS FAT da ESP-IDF. Conta com proteção por exclusão mútua via Mutex.
 * * @version 1.1
 * @date 2026-05-18
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h> 
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"




#include "esp_err.h"
#include "esp_log.h"


#include "sd_manager.h"

static char *TAG = "SD_MANAGER";


/**
 * @brief Inicializa o barramento SPI periférico (caso não esteja ativo) e monta o volume FATFS.
 */
esp_err_t sd_manager_init(sd_manager_handle_t *handle, sd_manager_conif_t *config) {
    esp_err_t ret;
    if (!handle || !config) {
        return ESP_ERR_INVALID_ARG;
    }

    handle->sd_config = *config;
    handle->card = NULL; // Garante ponteiro limpo no início de runtime

    /**
     * @note Alocação defensiva de recursos de concorrência:
     * Só cria o mutex se ele ainda não existir para evitar vazamentos de 
     * memória no caso de reinicializações consecutivas em quente.
     */
    if (handle->mutex == NULL) {
        handle->mutex = xSemaphoreCreateMutex();
        if (handle->mutex == NULL) {
            ESP_LOGE(TAG, "Falha ao criar o Mutex de proteção");
            return ESP_FAIL;
        }
    }

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    handle->sd_config.spi.spi_device = host.slot;

    /**
     * @note Inicialização do barramento físico através do componente customizado spi_dev:
     * Configura as linhas lógicas do periférico SPI e mapeia os pinos MISO/MOSI/SCLK.
     */
    ret = spi_dev_init(&(handle->sd_config.spi), &(handle->sd_config.spi.spi_config));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar o protocolo SPI: %s", esp_err_to_name(ret));
        vSemaphoreDelete(handle->mutex);
        handle->mutex = NULL;
        return ESP_FAIL;
    }

    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.gpio_cs = handle->sd_config.cs_pin;

    esp_vfs_fat_mount_config_t mount_cfg = {
        .format_if_mount_failed = false, // Evita formatação automática indesejada em caso de mal contato
        .max_files = handle->sd_config.mxfiles,
        .allocation_unit_size = handle->sd_config.clusters
    };

    /**
     * @note Gerenciamento de Estado do Virtual File System (VFS):
     * Força uma desmontagem lógica anterior do ponto "/sdcard" para limpar tabelas de 
     * descritores residuais que possam ter sobrevivido a resets parciais de hardware.
     */
    esp_vfs_fat_sdcard_unmount("/sdcard", handle->card);

    ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_cfg, &mount_cfg, &(handle->card));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Montagem inicial falhou: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 * @brief Executa a remontagem dinâmica do cartão SD (Hot-Plug/Failover recovery).
 */
esp_err_t sd_manager_mount(sd_manager_handle_t *handle) {
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(handle->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.gpio_cs = handle->sd_config.cs_pin;

    esp_vfs_fat_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files = handle->sd_config.mxfiles,
        .allocation_unit_size = handle->sd_config.clusters
    };

    /**
     * @note Desmontagem Obrigatória Prévia:
     * Libera o registro lógico do VFS e limpa a referência para que a nova 
     * tentativa física inicialize do zero, evitando travamento de semáforos internos.
     */
    esp_vfs_fat_sdcard_unmount("/sdcard", handle->card);
    handle->card = NULL;

    esp_err_t ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_cfg, &mount_cfg, &(handle->card));

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Remontagem falhou em runtime: %s", esp_err_to_name(ret));
        xSemaphoreGive(handle->mutex);
        return ESP_FAIL;
    }

    xSemaphoreGive(handle->mutex);
    return ESP_OK;
}

/**
 * @brief Formata a partição ativa no formato FAT.
 */
esp_err_t sd_manager_format(sd_manager_handle_t *handle) {
    if (!handle || !handle->card) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(handle->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t ret = esp_vfs_fat_sdcard_format("/sdcard", handle->card); 

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha critica ao formatar o volume FATFS: %s", esp_err_to_name(ret));
        xSemaphoreGive(handle->mutex);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Cartao SD formatado com sucesso em FATFS!");
    
    xSemaphoreGive(handle->mutex);
    return ESP_OK;
}

/**
 * @brief Grava uma linha de dados de maneira síncrona e segura contra perdas energéticas.
 */
esp_err_t sd_manager_write_file(sd_manager_handle_t *handle, char *modo) {
    if (!handle || !handle->sd_config.data || !handle->sd_config.file_name) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(handle->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    FILE *f = fopen(handle->sd_config.file_name, modo);
    if (f == NULL) {
        ESP_LOGE(TAG, "Falha ao abrir o arquivo para escrita: %s", handle->sd_config.file_name);
        xSemaphoreGive(handle->mutex);
        return ESP_FAIL;
    }

    if (fprintf(f, "%s\n", handle->sd_config.data) < 0) {
        ESP_LOGE(TAG, "Erro interno no fprintf!");
        fclose(f);
        xSemaphoreGive(handle->mutex);
        return ESP_FAIL;
    }

    /**
     * @note Esvaziamento de Buffer Lógico:
     * Força a descarga imediata dos dados retidos na stream de software do runtime 
     * da biblioteca padrão C para os descritores do sistema de arquivos subjacente.
     */
    fflush(f);
    
    if (ferror(f)) {
        ESP_LOGE(TAG, "Stream de arquivo detectou erro ativo (ferror)!");
        fclose(f);
        xSemaphoreGive(handle->mutex);
        return ESP_FAIL;
    }

    /**
     * @note Interpelação direta em hardware (fsync):
     * Força o controlador de barramento a esvaziar os buffers físicos e gravar 
     * os setores na memória Flash real do cartão SD, prevenindo perdas por blackouts.
     */
    if (fsync(fileno(f)) != 0) {
        ESP_LOGE(TAG, "Falha de barramento detectada via fsync!");
        fclose(f);
        xSemaphoreGive(handle->mutex);
        return ESP_FAIL; 
    }

    /**
     * @note Validação de Commit de Fechamento:
     * Verifica o valor de retorno de fclose. Caso ocorra uma falha de barramento 
     * no instante final de escrita física, o erro é capturado para acionar o failover.
     */
    if (fclose(f) != 0) {
        ESP_LOGE(TAG, "Erro critico retornado pelo fclose! Hardware falhou no commit.");
        xSemaphoreGive(handle->mutex);
        return ESP_FAIL; 
    }

    xSemaphoreGive(handle->mutex);
    return ESP_OK;
}

/**
 * @brief Lê o arquivo configurado por completo linha por linha e joga na console.
 */
esp_err_t sd_manager_read_file(sd_manager_handle_t *handle) {
    if (!handle || !handle->sd_config.file_name) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(handle->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    FILE *f = fopen(handle->sd_config.file_name, "r");
    if (f == NULL) {
        ESP_LOGW(TAG, "Arquivo nao encontrado ou vazio.");
        xSemaphoreGive(handle->mutex);
        return ESP_ERR_NOT_FOUND;
    }

    char linha[128];
    while (fgets(linha, sizeof(linha), f) != NULL) {
        linha[strcspn(linha, "\n")] = 0; 
        printf("[LOG SD] %s\n", linha); 
        
        /**
         * @note Liberação Voluntária de CPU:
         * Um pequeno delay de ticks impede que loops intensivos de leitura de arquivos 
         * grandes saturem o núcleo de processamento, evitando starve de tarefas prioritárias.
         */
        vTaskDelay(pdMS_TO_TICKS(10));  
    }

    fclose(f);
    xSemaphoreGive(handle->mutex);
    return ESP_OK;
}

/**
 * @brief Verifica a presença de um arquivo específico na partição FAT montada usando stat.
 */
esp_err_t sd_manager_exists(sd_manager_handle_t *handle, const char *file_name) {
    if (!handle || !file_name) {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(handle->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    struct stat st;
    if (stat(file_name, &st) == 0) {
        xSemaphoreGive(handle->mutex);
        return ESP_OK;
    }
    xSemaphoreGive(handle->mutex);
    return ESP_ERR_NOT_FOUND;
}

/**
 * @brief Exclui fisicamente do cartão o arquivo configurado no handler.
 */
esp_err_t sd_manager_delete_file(sd_manager_handle_t *handle) {
    if (!handle || !handle->sd_config.file_name) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(handle->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    struct stat st;
    if (stat(handle->sd_config.file_name, &st) != 0) {
        ESP_LOGW(TAG, "Tentativa de exclusao falhou: arquivo nao existe.");
        xSemaphoreGive(handle->mutex);
        return ESP_ERR_NOT_FOUND;
    }

    if (remove(handle->sd_config.file_name) != 0) {
        ESP_LOGE(TAG, "Falha interna ao deletar o arquivo.");
        xSemaphoreGive(handle->mutex);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Arquivo deletado com sucesso.");
    xSemaphoreGive(handle->mutex);
    return ESP_OK;
}

/**
 * @brief Obtém dados de volumetria e capacidade do sistema de arquivos FATFS.
 */
esp_err_t sd_manager_get_info(sd_manager_handle_t *handle) {
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(handle->mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    uint64_t bytes_totais = 0;
    uint64_t bytes_livres = 0;

    esp_err_t ret = esp_vfs_fat_info("/sdcard", &bytes_totais, &bytes_livres);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao obter informacoes do volume FATFS: %s", esp_err_to_name(ret));
        xSemaphoreGive(handle->mutex);
        return ESP_FAIL;
    }

    uint32_t total_mb = (uint32_t)(bytes_totais / (1024 * 1024));
    uint32_t livre_mb = (uint32_t)(bytes_livres / (1024 * 1024));

    ESP_LOGI(TAG, "====== STATUS DO CARTAO SD ======");
    ESP_LOGI(TAG, "Espaco Total: %lu MB", total_mb);
    ESP_LOGI(TAG, "Espaco Livre: %lu MB", livre_mb);
    ESP_LOGI(TAG, "Espaco Usado: %lu MB", total_mb - livre_mb);
    ESP_LOGI(TAG, "=================================");

    xSemaphoreGive(handle->mutex);
    return ESP_OK;
}