/**
 * @file main.c
 * @author Siria
 * @brief Sistema de Log de Eventos integrando NVS e SPIFFS.
 * 
 * O sistema recupera um contador de boots da NVS, incrementa-o e salva 
 * um novo registro em um arquivo de texto na partição SPIFFS, mantendo o histórico.
 * 
 * @version 1.0
 * @date 2026-05-14
 */

#include <stdio.h>
#include "esp_log.h"
#include "esp_random.h"
#include "arq_spiffs.h" 
#include "arq_nvs.h"    

static const char *TAG_MAIN = "TAREFA_LOG";

void app_main(void)
{
    esp_err_t ret;

    /** 
     * @section Passo 1: Configuração e Montagem do Sistema de Arquivos (SPIFFS).
     * Define o ponto de montagem, a partição no CSV e o caminho do arquivo de log.
     */
    arq_spiffs_handle_t spiffs_h;
    arq_spiffs_config_t spiffs_cfg = {
        .path = "/spiffs",
        .part_label = "storage",
        .mxfiles = 5,
        .namesp = "/spiffs/log.txt"
    };

    if (arq_spiffs_init(&spiffs_h, &spiffs_cfg) != ESP_OK) {
        ESP_LOGE(TAG_MAIN, "Erro ao inicializar SPIFFS!");
        return;
    }

    /** 
     * @section Passo 2: Recuperação do Contador de Boots via NVS.
     * Garante que o número da inicialização atual seja persistido entre resets.
     */
    int32_t boot_counter = 0; 
    arq_nvs_handle_t nvs_h;
    arq_nvs_config_t nvs_cfg = {
        .namesp = "armazenamento",
        .key = "cont_boot",
        .data = &boot_counter,
        .type = NVS_TYPE_I32
    };

    arq_nvs_init(&nvs_h, &nvs_cfg);
    
    ret = arq_nvs_read(&nvs_h);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG_MAIN, "Primeira execucao, iniciando contador.");
    }

    boot_counter++; // Incrementa para o boot atual
    arq_nvs_write(&nvs_h);

    /** 
     * @section Passo 3: Gravação de Telemetria (Modo Append).
     * Prepara uma string com o número do boot e um valor de sensor aleatório.
     */
    int valor_sensor = (esp_random() % 25) + 10; 
    char buffer_log[100];
    
    snprintf(buffer_log, sizeof(buffer_log), "Boot Count: %ld | Sensor Temp: %d C", boot_counter, valor_sensor);
    
    spiffs_h.spiffs_config.data = buffer_log;
    ESP_LOGI(TAG_MAIN, "Gravando no log: %s", buffer_log);
    
    // Modo "a" (Append) adiciona ao final do arquivo sem apagar o anterior
    ret = arq_spiffs_write(&spiffs_h, "a"); 
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_MAIN, "Erro ao gravar no arquivo log.txt");
    }

    /** 
     * @section Passo 4: Leitura do Histórico.
     * Lê todo o conteúdo do arquivo log.txt para exibir o histórico completo.
     */
    ESP_LOGI(TAG_MAIN, "--- INICIO DO LOG HISTORICO ---");
    arq_spiffs_read(&spiffs_h); 
    ESP_LOGI(TAG_MAIN, "--- FIM DO LOG HISTORICO ---");

    /** @section Passo 5: Desalocação de recursos e Mutexes. */
    arq_nvs_deinit(&nvs_h);
    arq_spiffs_deinit(&spiffs_h);

    ESP_LOGI(TAG_MAIN, "Tarefa concluida. Reinicie para acumular logs.");
}