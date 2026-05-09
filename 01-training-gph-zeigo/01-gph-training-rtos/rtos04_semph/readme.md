
# Desafio 4: Sincronização com Semáforos no FreeRTOS

Este projeto demonstra a implementação de sincronização entre tarefas utilizando **Semáforos** no ESP32. O objetivo é coordenar a execução de uma tarefa de processamento baseada em um evento gerado por uma tarefa de sensor, garantindo que o processamento só ocorra quando o dado estiver disponível.


## Descrição do Desafio

O fluxo de execução foi estruturado da seguinte forma:

1. **Tarefa de Sensor:** Simula a leitura de um sensor que gera um evento (ex: uma leitura randômica que atende a um critério específico).
2. **Tarefa de Processamento:** Permanece em estado bloqueado (*Blocked*), não consumindo ciclos de CPU, até que o evento ocorra.
3. **Sincronização:** Utiliza um `SemaphoreHandle_t` (do tipo Binário) para notificar a tarefa de processamento assim que o "evento" é detectado.

---

## Respostas do Desafio

### 1. Qual a diferença entre mutex e semáforo?

* **Mutex**: É utilizado para o compartilhamento de recursos de forma mutuamente exclusiva. Possui o conceito de "propriedade" (*ownership*), ou seja, a mesma task que pega o recurso deve devolvê-lo.

* **Semáforo**: É utilizado prioritariamente para sincronizar tarefas. Nele, uma tarefa (ou interrupção) pode liberar o semáforo para que outra tarefa o receba, funcionando como um sinalizador de prontidão.

### 2. Em quais situações usamos semáforos?

* Usamos semáforos principalmente para **sincronizar tasks**, garantindo que uma tarefa de processamento só inicie sua execução após a sinalização de que um evento ou dado foi concluído por outra parte do sistema.

### 3. Como esse mecanismo pode ser usado com interrupções?

* Uma interrupção de hardware (ISR) pode liberar o semáforo através da função `xSemaphoreGiveFromISR()`. Isso permite que a interrupção notifique uma task de alto nível sobre a ocorrência de um evento físico (como um sinal de satélite ou sensor crítico), permitindo que o processamento pesado ocorra fora do contexto da interrupção.

---

## 🛠️ Tecnologias e Conceitos Aplicados

* **ESP-IDF v5.x**
* **FreeRTOS Semaphores**: Implementação de semáforos binários para controle de fluxo.
* **Linguagem C**: Focada em sistemas embarcados críticos.
* **Sincronização de Threads**: Conceito fundamental para sistemas de tempo real.

