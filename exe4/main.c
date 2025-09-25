#include <stdbool.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

const int PIN_LED_B = 4;
const float conversion_factor = 3.3f / (1 << 12);  // 3.3/4096

// --- LED timer state ---
static struct repeating_timer led_timer;
static bool led_timer_armed = false;
static bool led_state = false;

// Pisca o LED no período configurado (delay)
static bool led_timer_cb(struct repeating_timer *t) {
    led_state = !led_state;
    gpio_put(PIN_LED_B, led_state);
    return true;
}

// Rearma o timer do LED conforme "delay" (0/300/500)
static void rearm_led_timer(int delay_ms) {
    if (led_timer_armed) {
        cancel_repeating_timer(&led_timer);
        led_timer_armed = false;
    }
    if (delay_ms == 0) {
        led_state = false;
        gpio_put(PIN_LED_B, 0); // sempre apagado na zona 0
        return;
    }
    add_repeating_timer_ms(delay_ms, led_timer_cb, NULL, &led_timer);
    led_timer_armed = true;
}

// "Timer" polled a cada ~10ms (sem sleep)
bool timer_callback(void) {
    static absolute_time_t next = {0};
    absolute_time_t now = get_absolute_time();
    if (!to_us_since_boot(next)) {
        // primeira chamada: agenda para agora +10ms
        next = delayed_by_us(now, 10 * 1000);
        return true; // dispara já na primeira passagem, como você queria
    }
    if (!absolute_time_diff_us(now, next)) { // now >= next
        next = delayed_by_us(now, 10 * 1000);
        return true;
    }
    return false;
}

// Lê o potenciômetro (volts) no ADC2 (GPIO28)
static inline float read_potentiometer(void) {
    uint16_t raw = adc_read();
    return raw * conversion_factor;
}

int main() {
    stdio_init_all();

    // LED
    gpio_init(PIN_LED_B);
    gpio_set_dir(PIN_LED_B, true);
    gpio_put(PIN_LED_B, 0);

    // ADC em GPIO28 (ADC2)
    adc_init();
    adc_gpio_init(28);
    adc_select_input(2);

    int delay = -1;              // flag da tua lógica (0=apagado, 300, 500)
    int prev_delay = -2;         // para detectar mudança de faixa

    while (1) {
        // faz leitura do potenciômetro logo no início e depois a cada 10ms
        static bool flag_timer = false;
        static float voltagem = 0.0f;

        flag_timer = timer_callback();
        if (flag_timer) {
            // Lê o valor do potenciômetro em volts
            voltagem = read_potentiometer();

            // verifica se manteve na mesma faixa (tua lógica)
            if (voltagem <= 1.0f && delay != 0) {
                delay = 0;              // LED apagado
            } else if (voltagem > 1.0f && voltagem <= 2.0f && delay != 300) {
                delay = 300;            // pisca 300 ms
            } else if (voltagem > 2.0f && voltagem <= 3.3f && delay != 500) {
                delay = 500;            // pisca 500 ms
            }

            // Se mudou de faixa, rearmar timer do LED
            if (delay != prev_delay) {
                rearm_led_timer(delay);
                prev_delay = delay;
            }

            flag_timer = false;
        }
        // nada de sleep_* nem tight_loop_contents
    }
}
