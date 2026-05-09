# Desafio 2: Concorrência e Variáveis Globais no FreeRTOS

Este projeto explora o comportamento de múltiplas tarefas acessando um recurso compartilhado (uma variável global) sem mecanismos de sincronização. O objetivo é observar o fenômeno de **Condição de Corrida (Race Condition)**.

## 📝 Enunciado do Desafio

1. **Variável Global:** Criar uma variável global chamada `counter`.
2. **Duas Tasks:** Ambas devem incrementar o mesmo contador e imprimir o resultado no log.
3. **Temporização:** Cada tarefa executa sua operação a cada 500 ms.

---

## 📊 Análise de Resultados

Ao observar os logs de execução, nota-se que os valores podem apresentar comportamentos inesperados, como saltos ou repetições de números entre as tarefas `COUNT1` e `COUNT2`.
<img src="resultados_uart-1.png" width="250">
### Respostas às Perguntas de Reflexão

#### 1. Os valores aparecem sempre em ordem?
**Não necessariamente.** Embora as tarefas tenham a mesma prioridade e tempo de delay, o escalonador do FreeRTOS pode alternar entre elas em momentos distintos. Além disso, a operação de incremento e a operação de log não são atômicas, o que pode causar uma desordem visual ou lógica nos logs.

#### 2. Por que ocorrem inconsistências?
As inconsistências ocorrem devido à **Race Condition (Condição de Corrida)**. 
- O incremento `counter++` em C, no nível do processador, envolve três etapas:
    1. Ler o valor da memória para um registrador.
    2. Incrementar o registrador.
    3. Escrever o valor de volta na memória.
- Se a Task 1 for interrompida pelo escalonador após o passo 1, e a Task 2 ler o mesmo valor antigo, ambas escreverão o mesmo resultado final, causando a perda de um incremento.

---

## 🛠️ Como solucionar?
Para garantir a integridade dos dados em sistemas reais, deve-se utilizar mecanismos de sincronização do FreeRTOS:
*   **Mutexes:** Para garantir exclusão mútua no acesso à variável.
*   **Semáforos:** Para sinalização entre tarefas.
*   **Operações Atômicas:** Para incrementos simples e rápidos.

---
**Desenvolvido por:** Siria  
**Contexto:** Engenharia Mecatrônica - UFRN