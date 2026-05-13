/**
 * @file main.c
 * @author Siria
 * @brief Implementação de um sistema de recuperação de erros persistente (Anti-Panic) utilizando NVS.
 * 
 * Este programa monitora a estabilidade do sistema durante o boot. Se o sistema sofrer
 * múltiplas reinicializações inesperadas (crashes) em um curto intervalo, a execução 
 * é bloqueada para proteger o hardware e permitir diagnóstico.
 * 
 * @version 1.1
 * @date 2026-05-12
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "arq_nvs.h"

/** 
 * @brief Tag utilizada para os logs do sistema no console.
 */
static const char *TAG = "APP_STABILITY";

/**
 * @brief Ponto de entrada principal da aplicação.
 * 
 * Realiza a leitura do contador de erros na NVS, aplica a lógica de bloqueio por
 * instabilidade e gerencia o reset do contador após um período de operação estável.
 */
void app_main(void) {

    int8_t err_count = 0;
    /** 
     * @name Configuração do Driver NVS
     * @{
     */
    arq_nvs_config_t nvs_config = {
        .namesp = "system_recovery", ///< Namespace da partição NVS
        .key = "err_count",          ///< Chave para o contador de erros
        .data = &err_count,          ///< Referência para escrita/leitura direta
        .type = NVS_TYPE_I8          ///< Tipo de dado: Inteiro de 8 bits assinado
    };
    /** @} */

    arq_nvs_handler_t nvs_handler;

    /**
     * @step 1: Inicialização do Driver.
     * Cuida da inicialização da partição flash e do semáforo de proteção.
     */
    if (nvs_init(&nvs_handler, &nvs_config) != ESP_OK) {
        ESP_LOGE(TAG, "Erro fatal ao inicializar driver NVS");
        return;
    }

    /**
     * @step 2: Leitura do estado de erro persistente.
     * Se a chave não existir, assume-se a primeira execução do sistema.
     */
    esp_err_t err = nvs_read(&nvs_handler);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Primeira execução: Contador não encontrado, iniciando em 0.");
        err_count = 0;
    }

    ESP_LOGI(TAG, "Contador de erros atual: %d", err_count);

    /**
     * @step 3: Verificação de Limite de Falhas (Anti-Panic).
     * Se o sistema falhou 3 ou mais vezes consecutivas, entra em loop infinito de segurança.
     */
    if (err_count >= 3) {
        ESP_LOGE(TAG, "!!! SISTEMA BLOQUEADO POR INSTABILIDADE !!!");
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }

    /**
     * @step 4: Registro de Início de Processo.
     * Incrementa o contador na NVS para marcar que uma nova tentativa de boot iniciou.
     */
    err_count++;
    ESP_LOGI(TAG, "Incrementando contador para %d...", err_count);
    nvs_write(&nvs_handler);

    /**
     * @step 5: Período de Teste de Estabilidade.
     * O sistema deve permanecer ativo por 10 segundos para ser considerado funcional.
     */
    ESP_LOGI(TAG, "Iniciando teste de estabilidade (10s)...");
    vTaskDelay(pdMS_TO_TICKS(10000)); 

    /**
     * @step 6: Confirmação de Estabilidade.
     * Após 10s sem reinicializações, o contador é zerado na memória não volátil.
     */
    err_count = 0;
    if (nvs_write(&nvs_handler) == ESP_OK) {
        ESP_LOGI(TAG, "Sistema estavel! Contador resetado para 0 na NVS.");
    }

    /**
     * @step 7: Ciclo de Operação Normal.
     * Início da lógica principal da aplicação após validação de estabilidade.
     */
    for (;;) {
        ESP_LOGI(TAG, "Maquina Operando normalmente...");
        vTaskDelay(pdMS_TO_TICKS(5000)); 
    }
}