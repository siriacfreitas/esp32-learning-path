# Desafio Prático: Contador de Falhas e Anti-Panic (NVS)

Este projeto implementa um sistema de segurança persistente para uma máquina industrial utilizando o ESP32. A lógica detecta "boot loops" ou instabilidades consecutivas e bloqueia o funcionamento do hardware caso o sistema resete 3 vezes seguidas em um intervalo menor que 10 segundos.

## Cenário e Regras de Negócio

* **Detecção de Falha**: Ao iniciar, o sistema realiza a leitura da chave `err_count` no namespace `system_recovery`.
* **Incremento Persistente**: O contador é incrementado e salvo imediatamente na memória Flash.
* **Bloqueio de Segurança**: Se `err_count >= 3`, o sistema imprime "SISTEMA BLOQUEADO" e entra em um loop infinito (`while(1)`), impedindo o acionamento da máquina.
* **Validação de Estabilidade**: Se o sistema operar por 10 segundos sem novos resets, o contador é zerado na NVS, confirmando que a inicialização foi bem-sucedida.

---

## Configuração da Partição NVS (`partitions.csv`)

Para que o ESP32 reserve espaço na memória Flash para o armazenamento de dados persistentes, é necessário definir uma tabela de partições customizada.

### 1. Criar o arquivo `partitions.csv`

Crie este arquivo na raiz do seu projeto com o seguinte conteúdo:

```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     ,        0x4000,
otadata,  data, ota,     ,        0x2000,
phy_init, data, phy,     ,        0x1000,
factory,  app,  factory, ,        1M,

```

### 2. Habilitar no Menuconfig

1. Execute `idf.py menuconfig`.
2. Vá em **Partition Table** > **Partition Table Selection**.
3. Selecione **Custom partition table CSV**.
4. Em **Custom partition CSV file**, insira `partitions.csv`.

---

## Perguntas e Reflexões

### O que acontece se você esquecer o `nvs_commit()`?

O comando `nvs_set_xx` apenas prepara o dado no cache do driver. Sem o `nvs_commit()`, as alterações **não são gravadas fisicamente na Flash**. Em caso de reset ou queda de energia, o incremento do contador seria perdido, tornando o sistema de segurança ineficaz, pois ele sempre leria o valor antigo no próximo boot.

---

## Estrutura do Driver

O driver foi modularizado para oferecer uma interface simplificada e segura para o FreeRTOS:

* **`arq_nvs.h`**: Define as estruturas de configuração e os protótipos das funções.
* **`arq_nvs.c`**: Implementa a lógica de abertura/fechamento da NVS e utiliza um **Mutex** para garantir que apenas uma tarefa acesse a Flash por vez.
* **`main.c`**: Contém a lógica de aplicação e o teste de estabilidade de 10 segundos.

