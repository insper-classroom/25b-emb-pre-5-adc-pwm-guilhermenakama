#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

const int PIN_LED_B = 4;

volatile int faixa_atual = -1;   // faixa do potenciômetro
volatile bool led_state = false;

struct repeating_timer led_timer;

// Callback que alterna o LED
bool led_timer_callback(struct repeating_timer *t) {
    led_state = !led_state;
    gpio_put(PIN_LED_B, led_state);
    return true;
}

// Determina a faixa do potenciômetro
int get_faixa(float volts) {
    if (volts < 1.0f) return 0;
    else if (volts < 2.0f) return 1;
    else return 2;
}

// Callback que lê o ADC
bool adc_timer_callback(struct repeating_timer *t) {
    uint16_t raw = adc_read();
    float volts = raw * (3.3f / (1 << 12));
    int nova_faixa = get_faixa(volts);

    if (nova_faixa != faixa_atual) {
        faixa_atual = nova_faixa;

        // Se mudar de faixa, ajusta comportamento
        cancel_repeating_timer(&led_timer);

        if (faixa_atual == 0) {
            // Zona 0 → LED apagado
            gpio_put(PIN_LED_B, 0);
            led_state = false;
        } else if (faixa_atual == 1) {
            // Zona 1 → pisca a cada 300 ms
            add_repeating_timer_ms(300, led_timer_callback, NULL, &led_timer);
        } else {
            // Zona 2 → pisca a cada 500 ms
            add_repeating_timer_ms(500, led_timer_callback, NULL, &led_timer);
        }
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

    // Timer para leitura do potenciômetro
    struct repeating_timer adc_timer;
    add_repeating_timer_ms(10, adc_timer_callback, NULL, &adc_timer);

    while (1) {
        tight_loop_contents();
    }
}
