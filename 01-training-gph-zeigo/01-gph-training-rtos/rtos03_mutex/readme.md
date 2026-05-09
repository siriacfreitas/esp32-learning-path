# Desafio 3: Resolvendo Race Conditions com Mutex

Este projeto apresenta a solução para o problema de inconsistência de dados observado no desafio anterior. Utilizamos um **Mutex** (Mutual Exclusion) do FreeRTOS para garantir que apenas uma tarefa acesse a variável global por vez.

## Enunciado do Desafio

1. **Mutex:** Criar um objeto Mutex utilizando `xSemaphoreCreateMutex()`.
2. **Proteção:** Envolver o incremento e a impressão da variável `counter` com as funções de `Take` (tomar) e `Give` (devolver) o Mutex.
3. **Comparação:** Analisar a estabilidade dos logs comparando com a execução sem proteção.

---

## Análise de Resultados

Ao utilizar o Mutex, a seção crítica do código (o incremento da variável) torna-se atômica em relação às tarefas concorrentes.
<img src="resultados_uart.png" width="250">

### Respostas às Perguntas de Reflexão

#### 1. O problema foi resolvido?
**Sim.** Com o uso do Mutex, garantimos a **Atomicidade**. Mesmo que o escalonador tente trocar de tarefa no meio do incremento, a segunda tarefa ficará em estado de bloqueio (*Blocked*) aguardando a liberação do Mutex pela primeira. Isso evita que incrementos sejam "atropelados" e garante que cada valor impresso seja único e sequencial.

#### 2. O que acontece se esquecermos de liberar o mutex?
Se uma tarefa executar o `xSemaphoreTake()` mas nunca chamar o `xSemaphoreGive()`, ocorrerá um fenômeno chamado **Deadlock (Impasse)** ou **Starvation (Inanição)**.
- A tarefa que detém o Mutex continuará sua execução, mas nenhuma outra tarefa conseguirá acessar a região protegida.
- As demais tarefas ficarão presas para sempre no estado *Blocked*, esperando por um recurso que nunca será liberado, o que pode travar funcionalidades essenciais do firmware.

---

## 🛠️ Conceitos Chave
*   **Seção Crítica:** Parte do código que acessa um recurso compartilhado.
*   **Mutex:** Um semáforo especial binário usado para gerenciar o acesso exclusivo a recursos.
*   **Exclusão Mútua:** Propriedade que impede que dois processos acessem o mesmo recurso ao mesmo tempo.
