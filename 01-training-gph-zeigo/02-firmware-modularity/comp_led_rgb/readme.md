# Componente LED RGB para ESP32 (PWM/LEDC)

Este componente foi desenvolvido para facilitar o controle de LEDs RGB no ESP32 utilizando o periférico de hardware **LEDC (LED Control)**. Ele suporta o ajuste de cores através de Duty Cycle (PWM) e possui proteção nativa para ambientes **RTOS**.

<div align="center">
  <img src="led_imag.png" width="350">
</div>


## Funcionalidades

- **Controle PWM de Hardware:** Utiliza o periférico LEDC para controle suave e eficiente.
- **Thread-Safe:** Implementação protegida por **Mutex**, permitindo que múltiplas tarefas alterem a cor do LED sem corromper as configurações dos canais.
- **Validação de Parâmetros:** Checagem rigorosa de GPIOs, canais e resolução de timer para evitar falhas de hardware.
- **Documentação Doxygen:** Código totalmente comentado para geração automática de documentação.

---

## Como Usar

### 1. Configuração e Inicialização
Defina os pinos e canais que deseja utilizar na estrutura de configuração:

```c
led_rgb_config_t led_config = {
    .red_pin = RED_PIN_GPIO,
    .blue_pin = BLUE_PIN_GPIO,
    .green_pin = GREEN_PIN_GPIO,

    .red_channel = RED_LEDC_CHANNEL,
    .green_channel = GREEN_LEDC_CHANNEL,
    .blue_channel = BLUE_LEDC_CHANNEL,

    .timer_num = LEDC_TIMER,
    .duty_resolution = LEDC_DUTY_RESOLUTION,
};

led_rgb_handle_t led_handle;
led_rgb_init(&led_handle, &led_config);
```

### 2. Alterando Cores
O comando set_color aceita valores baseados na resolução escolhida (ex: 0 a 255 para 8 bits).

```c
// Define cor Branca (Brilho máximo em todos os canais)
led_rgb_set_color(&led_handle, 255, 255, 255);

// Desliga o LED
led_rgb_set_color(&led_handle, 0, 0, 0);
```
