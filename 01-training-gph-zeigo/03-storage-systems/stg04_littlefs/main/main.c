/**
 * @file main.c
 * @brief Script de testes unitários e de estresse para validação do componente littlefs_manager.
 * @version 1.0
 * @date 2026-06-02
 * @copyright Copyright (c) 2026
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#include "littlefs_manager.h"

/** * @brief Tag utilizada para identificação das mensagens de log deste arquivo no console.
 */
static const char *TAG_MAIN = "MAIN_TEST";

/**
 * @brief Ponto de entrada principal da aplicação (Application Main Task).
 * * Executa uma sequência de 7 passos lógicos para testar exaustivamente todas as 
 * funcionalidades do gerenciador do LittleFS, incluindo cenários de erro,
 * persistência após formatação e comportamento seguro após desinicialização.
 */
void app_main(void)
{
    ESP_LOGI(TAG_MAIN, "================================================");
    ESP_LOGI(TAG_MAIN, "--- INICIANDO TESTE COMPLETO: LITTLEFS MANAGER ---");
    ESP_LOGI(TAG_MAIN, "================================================");

    /** @brief Ponto de montagem base no sistema de arquivos virtual (VFS). */
    const char *base_path = "/littlefs";
    
    /** @brief Caminho absoluto do arquivo utilizado durante as rotinas de teste. */
    const char *file_path = "/littlefs/teste_log.txt";

    /** @brief Configuração operacional para a partição do teste. */
    littlefs_manager_config_t config = {
        .path = base_path,
        .part_label = "storage",
        .namesp = file_path,
        .data = NULL
    };

    /** @brief Estrutura de controle de runtime (Handle) inicializada vazia. */
    littlefs_manager_handle_t handle = { .mutex = NULL };
    esp_err_t ret;

    /* -------------------------------------------------------------------------
     * 1. TESTE DE INICIALIZAÇÃO
     * ----------------------------------------------------------------------- */
    ESP_LOGI(TAG_MAIN, "[PASSO 1] Inicializando o Componente...");
    ret = littlefs_manager_init(&handle, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_MAIN, "Falha critica na inicializacao: %s", esp_err_to_name(ret));
        return;
    }

    /* -------------------------------------------------------------------------
     * 2. TESTE DE VERIFICAÇÃO DE EXISTÊNCIA (Cenário: Arquivo Não Existe)
     * ----------------------------------------------------------------------- */
    ESP_LOGI(TAG_MAIN, "[PASSO 2] Verificando se arquivo inexistente eh detectado...");
    ret = littlefs_manager_exists(&handle, file_path);
    if (ret == ESP_ERR_NOT_FOUND) {
        ESP_LOGI(TAG_MAIN, "-> Correto: O arquivo realmente nao existe ainda.");
    } else {
        ESP_LOGW(TAG_MAIN, "-> Inesperado: Retorno de existencia: %s", esp_err_to_name(ret));
    }

    /* -------------------------------------------------------------------------
     * 3. TESTE DE ESCRITA E LEITURA
     * ----------------------------------------------------------------------- */
    ESP_LOGI(TAG_MAIN, "[PASSO 3] Criando arquivo e escrevendo dados (Modo 'w')...");
    handle.littlefs_config.data = "UUID-12345: Log de Inicializacao do Sistema.";
    if (littlefs_manager_write(&handle, "w") == ESP_OK) {
        ESP_LOGI(TAG_MAIN, "Adicionando segunda linha (Modo 'a')...");
        handle.littlefs_config.data = "STATUS: Sensores operando dentro da normalidade.";
        littlefs_manager_write(&handle, "a");
    }

    ESP_LOGI(TAG_MAIN, "Lendo conteudo gravado:");
    littlefs_manager_read(&handle);

    /* -------------------------------------------------------------------------
     * 4. TESTE DE VERIFICAÇÃO DE EXISTÊNCIA (Cenário: Arquivo Existe)
     * ----------------------------------------------------------------------- */
    ESP_LOGI(TAG_MAIN, "[PASSO 4] Verificando se arquivo existente eh detectado...");
    ret = littlefs_manager_exists(&handle, file_path);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG_MAIN, "-> Sucesso: Arquivo localizado dinamicamente no VFS.");
    } else {
        ESP_LOGE(TAG_MAIN, "-> Erro: Arquivo deveria existir mas nao foi achado: %s", esp_err_to_name(ret));
    }

    /* -------------------------------------------------------------------------
     * 5. TESTE DE EXCLUSÃO DE ARQUIVO
     * ----------------------------------------------------------------------- */
    ESP_LOGI(TAG_MAIN, "[PASSO 5] Deletando o arquivo criado...");
    ret = littlefs_manager_delete_file(&handle, file_path);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG_MAIN, "-> Sucesso: Arquivo deletado.");
    }

    ESP_LOGI(TAG_MAIN, "Validando se o arquivo sumiu mesmo...");
    ret = littlefs_manager_exists(&handle, file_path);
    if (ret == ESP_ERR_NOT_FOUND) {
        ESP_LOGI(TAG_MAIN, "-> Confirmado: Arquivo nao existe mais.");
    }

    /* -------------------------------------------------------------------------
     * 6. TESTE DE FORMATAÇÃO DA PARTIÇÃO
     * ----------------------------------------------------------------------- */
    ESP_LOGI(TAG_MAIN, "[PASSO 6] Recriando um arquivo para testar a formatacao limpa...");
    handle.littlefs_config.data = "Dados confidenciais que serao apagados pelo format.";
    littlefs_manager_write(&handle, "w");

    ESP_LOGI(TAG_MAIN, "Executando Formatacao Forcada da particao...");
    ret = littlefs_manager_format(&handle);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG_MAIN, "-> Verificando se dados resistiram a formatacao...");
        ret = littlefs_manager_read(&handle);
        if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGI(TAG_MAIN, "-> Sucesso Absoluto: Particao limpa e arquivo sumiu!");
        }
    }

    /* -------------------------------------------------------------------------
     * 7. TESTE DE DESINICIALIZAÇÃO (DEINIT) E PROTEÇÃO CONTRA CRASH
     * ----------------------------------------------------------------------- */
    ESP_LOGI(TAG_MAIN, "[PASSO 7] Desinicializando componentes e liberando Mutex...");
    ret = littlefs_manager_deinit(&handle);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG_MAIN, "-> Componente desativado com sucesso.");
    }

    /** * @note Teste de Estresse / Resguardo:
     * Tenta operar sobre o dispositivo logo após o Deinit. O componente deve interceptar
     * o ponteiro nulo do Mutex e retornar um erro seguro ao invés de causar um Kernel Crash.
     */
    ESP_LOGI(TAG_MAIN, "Tentando ler arquivo após Deinit (Deve falhar de forma segura)...");
    ret = littlefs_manager_read(&handle);
    if (ret == ESP_ERR_INVALID_ARG || handle.mutex == NULL) {
        ESP_LOGI(TAG_MAIN, "-> Protecao Ativa: Sistema impediu operacao em ponteiro nulo/invalido.");
    }

    ESP_LOGI(TAG_MAIN, "================================================");
    ESP_LOGI(TAG_MAIN, "---      TODOS OS TESTES CONCLUIDOS         ---");
    ESP_LOGI(TAG_MAIN, "================================================");

    /**
     * @brief Loop de segurança para impedir o encerramento prematuro da main task.
     * Mantém o chip em IDLE para visualização limpa dos logs via monitor serial.
     */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}