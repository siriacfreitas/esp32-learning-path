
# LittleFS Manager Component for ESP-IDF

[![ESP-IDF Version](https://img.shields.io/badge/ESP--IDF-v5.5.3-blue)](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/index.html)
[![Platform](https://img.shields.io/badge/Platform-ESP32-orange)](https://www.espressif.com/en/products/socs/esp32)
[![License](https://img.shields.io/badge/License-MIT-green)](https://opensource.org/licenses/MIT)

Um componente robusto, encapsulado e **Thread-Safe** (seguro para uso concorrente entre múltiplas Tasks) desenvolvido em C para o ecossistema **ESP-IDF v5.5.3**. Ele simplifica o gerenciamento do sistema de arquivos **LittleFS** em memórias Flash SPI internas ou externas.

## Características

* **Sincronismo via Mutex:** Todas as operações críticas de I/O são protegidas por Semáforos de Exclusão Mútua (`Mutex`), evitando corrupção de dados por concorrência.
* **Segurança contra Crashes:** Sistema blindado contra ponteiros nulos em tempo de execução (impede *Kernel Panic* se chamado pós-desinicialização).
* **Controle de Escalonamento:** A leitura de arquivos longos possui um delay voluntário (`vTaskDelay`) embutido para evitar o *starvation* (fome de CPU) de outras tarefas do FreeRTOS.
* **Documentação Padrão:** Código 100% comentado seguindo as diretrizes do **Doxygen**.

---

## Estrutura do Projeto


stg04_littlefs/
├── components/
│   └── littlefs_manager/
│       ├── include/
│       │   └── littlefs_manager.h
│       ├── CMakeLists.txt
│       └── littlefs_manager.c
├── main/
│   ├── CMakeLists.txt
│   └── main.c
├── partitions.csv
└── CMakeLists.txt


---
## Instalação da Dependência
Este componente utiliza o driver low-level do LittleFS mantido pela comunidade. Para adicionar essa dependência necessária ao seu projeto, execute o seguinte comando no terminal da raiz do projeto:

Bash
idf.py add-dependency "joltwallet/littlefs^1.0.0"
#### Nota: Esse comando criará ou atualizará o arquivo idf_component.yml dentro do seu projeto, fazendo com que o ESP-IDF baixe e gerencie o pacote automaticamente durante o próximo build.


## Configuração da Partição

Para que o LittleFS funcione, você precisa definir uma partição do tipo `data` com o subtipo `littlefs` no arquivo `partitions.csv` do seu projeto.

Exemplo de `partitions.csv`:

```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     ,        0x6000,
phy_init, data, phy,     ,        0x1000,
factory,  app,  factory, ,        1M,
storage,  data, littlefs,,        1M,

```

#### **Nota:** O `part_label` configurado no código C deve ser exatamente igual ao nome definido na coluna `Name` (neste exemplo, `storage`).

---

## Como Usar (API Básica)

### Inicialização e Escrita

```c
#include "littlefs_manager.h"

void app_main(void) {
    // 1. Configurar as diretrizes da partição
    littlefs_manager_config_t config = {
        .path = "/littlefs",
        .part_label = "storage",
        .namesp = "/littlefs/log.txt",
        .data = NULL
    };

    littlefs_manager_handle_t handle = { .mutex = NULL };

    // 2. Inicializar o Driver e registrar no VFS
    if (littlefs_manager_init(&handle, &config) == ESP_OK) {
        
        // 3. Definir o dado e gravar (Modo "w" para sobrescrever, "a" para append)
        handle.littlefs_config.data = "Mensagem de Log do Sistema";
        littlefs_manager_write(&handle, "w");
    }
}

```

### Funções Disponíveis

| Função | Descrição |
| --- | --- |
| `littlefs_manager_init` | Inicializa o LittleFS, monta o VFS e aloca o Mutex. |
| `littlefs_manager_format` | Formata a partição de maneira forçada (Apaga tudo). |
| `littlefs_manager_write` | Abre o arquivo e escreve/adiciona uma string. |
| `littlefs_manager_read` | Lê o arquivo linha por linha imprimindo no console. |
| `littlefs_manager_exists` | Verifica atomicamente a existência de um arquivo. |
| `littlefs_manager_delete_file` | Deleta um arquivo específico do sistema. |
| `littlefs_manager_deinit` | Desregistra o VFS e libera o Mutex da memória Heap. |

---

## Anatomia do Arquivo de Testes (`main.c`)

O arquivo `main.c` incluso funciona como um **firmware de testes unitários e validação de estresse**. Ele simula o ciclo de vida completo do driver, forçando falhas para validar as proteções de concorrência.

### Fluxo de Execução dos Testes:

1. **Preparação e Montagem (Passos 1 e 2):** Inicializa o componente. Se a memória flash estivesse corrompida, o parâmetro estrutural formataria o chip automaticamente. Valida também que buscas por arquivos inexistentes retornam corretamente `ESP_ERR_NOT_FOUND`.
2. **Escrita Consecutiva (Passos 3, 4 e 5):** Abre o arquivo no modo escrita (`"w"`), injeta um bloco e fecha. Abre novamente no modo append (`"a"`), adicionando uma linha complementar de logs. Em seguida, lê os dados e remove o arquivo físicamente com o `delete_file`.
3. **Simulação de Desastre (Passo 6):** Grava um arquivo e força uma formatação de baixo nível (`littlefs_manager_format`), simulando um comando de *Factory Reset* remoto. O teste valida que a tabela de alocação foi limpa.
4. **O Teste de Invasão de Ponteiro (Passo 7):** O firmware intencionalmente desativa o driver com o `deinit` (destruindo o Mutex) e tenta ler o arquivo logo em seguida. A proteção interna do componente intercepta a chamada nula, impede um *Kernel Crash* no FreeRTOS e retorna com segurança o código `ESP_ERR_INVALID_ARG`.

---
## Licença
Este projeto está sob a licença MIT. Veja o arquivo [LICENSE](https://www.google.com/search?q=LICENSE) para mais detalhes.
