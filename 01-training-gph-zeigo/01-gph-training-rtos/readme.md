# Aula 01: RTOS e Sistemas de Tempo Real (GPH)

Este diretório contém os códigos e resoluções desenvolvidos durante a **Aula 01** do treinamento em ESP32 do **Grupo de Pesquisa em Hardware (GPH/UFRN)**. 

O foco deste módulo é a transição do modelo de programação sequencial (*Super Loop*) para o modelo multitarefa utilizando o **FreeRTOS**.

## Conceitos Chave 

* **Sistemas de Tempo Real (STR):** Sistemas onde a correção depende não apenas do resultado lógico, mas do **instante** em que é produzido.
* **Determinismo:** A capacidade de garantir que eventos críticos serão atendidos dentro de prazos (deadlines) previsíveis.
* **Kernel FreeRTOS:** O núcleo que gerencia o escalonamento de tarefas, prioridades e comunicação inter-tarefas no ESP32.

##  Estruturas de Sincronização Utilizadas

Durante os desafios, foram implementadas as seguintes primitivas do FreeRTOS:

1.  **Queues (Filas):** Utilizadas para o envio de dados entre tarefas (ex: Sensor -> Log).
2.  **Mutex:** Garante a exclusão mútua para evitar *Race Conditions* ao acessar recursos compartilhados.
3.  **Semáforos Binários:** Usados para sinalização e sincronização de eventos (Handshaking).
4.  **Event Groups:** Sincronização complexa onde uma tarefa aguarda múltiplos eventos (bits) para prosseguir.

---
> **Instrutor:** José I. S. Camilo (LASEM/GPH)  
> **Instituição:** Universidade Federal do Rio Grande do Norte (UFRN)