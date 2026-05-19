# Gerenciador de Armazenamento Híbrido e Failover (SD Card & SPIFFS) para ESP32

Este repositório contém a implementação de uma pilha de drivers modular, robusta e segura para o ecossistema ESP-IDF (v5.x+), projetada para aplicações embarcadas críticas que exigem alta integridade de dados e tolerância a falhas (Failover).

O sistema gerencia o armazenamento síncrono em um **Cartão SD externo (via barramento SPI periférico)** e implementa uma partição **SPIFFS (memória Flash interna)** como camada secundária de backup para prevenção de perda de logs em caso de falha física do cartão.

## Arquitetura do Sistema

A pilha de software é dividida em três camadas principais:

1. **`spi_dev` (Low-Level):** Camada de abstração do hardware do barramento SPI. Realiza validações estritas de pinagem, duplicidade de GPIO e gerencia a alocação de canais de DMA.
2. **`sd_manager` (High-Level / Principal):** Driver de gerenciamento do Cartão SD sobre o Virtual File System (VFS) FAT da ESP-IDF. Conta com mecanismos de segurança contra corrupção energética via `fsync()` e proteção por exclusão mútua (*Mutex*).
3. **`spiffs_manager` (High-Level / Backup):** Driver de gerenciamento da Flash interna. Atua como o mecanismo de failover instantâneo quando o barramento SPI ou o hardware do SD Card reportam falhas críticas de barramento ou commit.

---

## Guia Prático de Debug e Diagnóstico

### 1. Falhas de Hardware & Camada Física

Se o cartão SD **não monta**, execute a seguinte lista de verificação na bancada:

* **Alimentação Elétrica:** O cartão SD opera estritamente em **3.3V**. O uso de módulos regulados para 5V causa superaquecimento do cartão e queima do controlador interno.
* **Resistores de Pull-up:** É obrigatório o uso de resistores de pull-up externos de **10 kΩ** nas linhas `CMD` e `DAT[0-3]`. Sem esses resistores, o ESP32 não receberá o sinal de *Ready* do periférico, resultando no erro clássico `ESP_ERR_TIMEOUT`.
* **Integridade do Sinal & Ruído:** Trilhas longas, conexões frouxas ou cabos jumper soltos geram indutância e capacitância parasita no barramento síncrono.
    * *Sintoma:* Erros de verificação de redundância cíclica (`CRC`) intermitentes em runtime.
    * *Solução de Contorno:* Reduza a frequência do clock para `SDMMC_FREQ_PROBING` (400 kHz) para validar a estabilidade da fiação antes de subir para frequências nominais de barramento.

> ** Ferramenta de Inspeção em Hardware:**
> Logo após uma montagem bem-sucedida, chame a função nativa:
> ```c
> sdmmc_card_print_info(stdout, handle.card);
> ```
> Isso imprimirá no terminal a velocidade real negociada e se o cartão foi reconhecido corretamente como **SDSC**, **SDHC** ou **SDXC**.

### 2. Falhas de Software & Camada Lógica

Se o hardware está validado, mas a pilha de software retorna códigos de erro no monitor serial, atente-se aos seguintes diagnósticos:

* **`ESP_ERR_INVALID_STATE`:** Ocorre quando tenta-se montar o cartão SD antes de inicializar o barramento físico SPI. O subsistema SPI ainda não "existe" para o driver de alto nível. Certifique-se de que a função `spi_dev_init()` completou com sucesso antes de chamar o gerenciador do SD.
* **`ESP_ERR_NOT_FOUND`:** A partição mapeada não foi localizada. Caso ocorra na inicialização da SPIFFS, verifique se o arquivo de configuração de partições (`partitions.csv`) define corretamente o label mapeado no código (ex: `"storage"`).
* **Nomes Longos (LFN) e exFAT:** Se o firmware tentar manipular nomes de arquivos que excedam o padrão 8.3 ou se o cartão SD utilizado for superior a 32 GB (utilizando exFAT por padrão):
    * Acesse o `menuconfig` do projeto.
    * Habilite a flag `CONFIG_FATFS_LFN_HEAP`.
    * *Nota:* Sem essa configuração ativa, o FatFs truncará os nomes dos arquivos ou falhará sumariamente ao abrir diretórios.
* **Corrupção de Volume / Arquivos Vazios:** Se após um reset ou queda de energia o arquivo de log aparecer zerado ou sem as últimas linhas, significa que o fechamento ou a sincronização de escrita foram omitidos. A arquitetura deste driver mitiga isso aplicando de maneira síncrona:
    ```c
    fflush(f);
    fsync(fileno(f));
    fclose(f); // Valida o retorno do commit de hardware
    ```
## Estrutura de Uso Recomendada (Failover)

Para implementar a lógica de contingência (Failover do SD Card para a SPIFFS) na tarefa principal (`app_main`), utilize o fluxo defensivo baseado no retorno dos códigos de erro (`esp_err_t`):

```c
// Tentativa de escrita padrão no Cartão SD externo
esp_err_t status = sd_manager_write_file(&sd_handle, "a");

if (status != ESP_OK) {
    ESP_LOGW("MAIN", "Falha crítica no Cartão SD! Acionando failover para SPIFFS...");
    
    // Atualiza o buffer da SPIFFS com a informação e efetua a gravação interna
    spiffs_handle.spiffs_config.data = sd_handle.sd_config.data;
    if (spiffs_manager_write(&spiffs_handle, "a") == ESP_OK) {
        ESP_LOGI("MAIN", "Dados salvos com segurança na memória Flash interna.");
    } else {
        ESP_LOGE("MAIN", "Falha catastrófica: Armazenamento interno também falhou.");
    }
}
## Requisitos de Ambiente
Framework: ESP-IDF v5.x (ou superior) instalado e configurado.
Sistema Operacional RTOS: FreeRTOS (nativo da ESP-IDF).
