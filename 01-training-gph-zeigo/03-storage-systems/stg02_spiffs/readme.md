# Datalogger Integrado: Persistência com NVS e SPIFFS

Este projeto implementa um sistema de log de eventos para o ESP32, combinando o armazenamento de estados críticos em **NVS** (Non-Volatile Storage) e o registro de históricos extensos em **SPIFFS** (SPI Flash File System).

## Objetivos do Projeto

1. **Rastreabilidade de Boot**: Recuperar e atualizar o contador de inicializações via NVS.
2. **Registro de Telemetria**: Gravar dados de sensores simulados em um arquivo de texto (`log.txt`) na partição SPIFFS.
3. **Segurança Multitarefa**: Garantir que o acesso à Flash seja protegido por **Mutexes**, evitando conflitos de escrita.
4. **Histórico Cumulativo**: Utilizar o modo de abertura `append` ("a") para preservar registros de boots anteriores.

---

## Configuração da Tabela de Partições

Para o funcionamento correto, o arquivo `partitions.csv` na raiz do seu projeto deve reservar espaços distintos para a NVS e para o sistema de arquivos.

### 1. Criar o arquivo `partitions.csv`

```csv
# Name,     Type, SubType, Offset,  Size, Flags
nvs,        data, nvs,     ,        0x6000,
storage,    data, spiffs,  ,        0xF0000,
factory,    app,  factory, ,        1M,

```

### 2. Ativar no Projeto

* Execute `idf.py menuconfig`.
* Vá em **Partition Table** > **Partition Table Selection** > **Custom partition table CSV**.
* Defina o nome como `partitions.csv`.
* Defina a Flash size para 4MB
---

## Arquitetura dos Drivers

O sistema é dividido em dois componentes principais que trabalham de forma complementar:

### NVS (Contador de Boot)

* **Finalidade**: Armazenar variáveis simples (pares chave-valor).
* **Vantagem**: Rápido acesso e baixo overhead de memória.
* **Uso**: Salva o `boot_counter` para que o sistema saiba em qual ciclo de operação se encontra.

### SPIFFS (Arquivo de Log)

* **Finalidade**: Armazenar grandes volumes de texto ou dados estruturados.
* **Vantagem**: Interface padrão de arquivos em C (`fopen`, `fprintf`, `fgets`).
* **Uso**: Registra a string formatada contendo o número do boot e o valor do sensor.

---

## Operação do Sistema

Ao iniciar, o firmware executa o seguinte fluxo:

1. **Inicializa a NVS** e lê o contador atual. Se for o primeiro boot, inicia em 1.
2. **Incrementa e Salva** o contador imediatamente (Commit).
3. **Monta a partição SPIFFS** chamada `storage`.
4. **Grava no Arquivo**: Adiciona uma nova linha ao `log.txt` com a telemetria atual.
5. **Leitura Completa**: Varre o arquivo do início ao fim, exibindo todo o histórico de logs acumulados desde a primeira execução.

---

## Considerações de Engenharia

* **Tratamento de Falhas**: O driver está configurado para formatar a SPIFFS automaticamente (`format_if_mount_failed = true`) caso ocorra corrupção nos dados, garantindo que o sistema sempre volte a operar.
* **Gerenciamento de Recursos**: O uso de `arq_spiffs_deinit` e `arq_nvs_deinit` garante que os Mutexes sejam destruídos e os handles fechados, evitando vazamento de memória (memory leaks).
