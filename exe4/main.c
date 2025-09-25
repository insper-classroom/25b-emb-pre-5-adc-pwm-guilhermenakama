#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

const int PIN_LED_B = 4;

volatile int led_period = 0;     // período atual em ms (0 = desligado)
volatile bool led_state = false; // estado atual do LED

// Callback para piscar o LED
bool led_timer_callback(struct repeating_timer *t) {
    if (led_period == 0) {
        // zona 0 → LED sempre apagado
        gpio_put(PIN_LED_B, 0);
    } else {
        // alterna LED
        led_state = !led_state;
        gpio_put(PIN_LED_B, led_state);
    }
    return true; // mantém o timer rodando
}

// Callback para leitura do ADC
bool adc_timer_callback(struct repeating_timer *t) {
    uint16_t raw = adc_read();
    float volts = raw * (3.3f / (1 << 12));

    int novo_periodo = 0;
    if (volts < 1.0f) {
        novo_periodo = 0;      // desligado
    } else if (volts < 2.0f) {
        novo_periodo = 300;    // pisca a cada 300ms
    } else {
        novo_periodo = 500;    // pisca a cada 500ms
    }

    // só atualiza se houve mudança
    if (novo_periodo != led_period) {
        led_period = novo_periodo;
    }

    return true;
}

int main() {
    stdio_init_all();

    // Inicializa LED
    gpio_init(PIN_LED_B);
    gpio_set_dir(PIN_LED_B, true);
    gpio_put(PIN_LED_B, 0);

    // Inicializa ADC
    adc_init();
    adc_gpio_init(26);   // GPIO26 → ADC0
    adc_select_input(0);

    // Timer para o LED (sempre chama a cada 100ms, mas controla com led_period)
    struct repeating_timer led_timer;
    add_repeating_timer_ms(100, led_timer_callback, NULL, &led_timer);

    // Timer para leitura do potenciômetro a cada 10ms
    struct repeating_timer adc_timer;
    add_repeating_timer_ms(10, adc_timer_callback, NULL, &adc_timer);

    while (1) {
        tight_loop_contents(); // mantém CPU ativa sem sleep
    }
}
