#include <stdbool.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

const int PIN_LED_B = 4;
const float conversion_factor = 3.3f / (1 << 12); // 3.3/4096

// --- estado do LED/timer ---
static struct repeating_timer led_timer;
static bool led_timer_armed = false;
static bool led_state = false;

// callback do timer do LED (pisca conforme delay)
static bool led_timer_cb(struct repeating_timer *t) {
    led_state = !led_state;
    gpio_put(PIN_LED_B, led_state);
    return true;
}

// rearma/cancela timer do LED de acordo com delay
static void rearm_led_timer(int delay_ms, bool do_immediate_toggle) {
    if (led_timer_armed) {
        cancel_repeating_timer(&led_timer);
        led_timer_armed = false;
    }
    if (delay_ms == 0) {
        // zona 0: sempre apagado
        led_state = false;
        gpio_put(PIN_LED_B, 0);
        return;
    }

    // 👉 toggle imediato para o teste detectar mudança de pino logo após trocar de faixa
    if (do_immediate_toggle) {
        led_state = !led_state;
        gpio_put(PIN_LED_B, led_state);
    }

    add_repeating_timer_ms(delay_ms, led_timer_cb, NULL, &led_timer);
    led_timer_armed = true;
}

// seu "timer" polled de 10ms (sem sleep)
static bool timer_callback(void) {
    static bool first = true;
    static absolute_time_t next;
    absolute_time_t now = get_absolute_time();

    if (first) { // dispara logo no início
        next = delayed_by_ms(now, 10);
        first = false;
        return true;
    }
    if (absolute_time_diff_us(next, now) <= 0) { // now >= next
        next = delayed_by_ms(now, 10);
        return true;
    }
    return false;
}

// leitura do potenciômetro em volts (ADC2 / GPIO28)
static inline float read_potentiometer(void) {
    uint16_t raw = adc_read();
    return raw * conversion_factor;
}

int main() {
    stdio_init_all();

    // LED GP4
    gpio_init(PIN_LED_B);
    gpio_set_dir(PIN_LED_B, true);
    gpio_put(PIN_LED_B, 0);

    // ADC2 (GPIO28) conforme diagram.json
    adc_init();
    adc_gpio_init(28);
    adc_select_input(2);

    int delay = -1;      // tua flag (0=apagado, 300, 500)
    int prev_delay = -2; // para detectar mudança de faixa

    while (1) {
        static bool flag_timer = false;
        static float voltagem = 0.0f;

        // faz leitura logo no início e depois a cada 10ms
        flag_timer = timer_callback();
        if (flag_timer) {
            voltagem = read_potentiometer();

            // tua lógica de faixas
            if (voltagem <= 1.0f && delay != 0) {
                delay = 0;              // LED apagado
            } else if (voltagem > 1.0f && voltagem <= 2.0f && delay != 300) {
                delay = 300;            // pisca 300ms
            } else if (voltagem > 2.0f && voltagem <= 3.3f && delay != 500) {
                delay = 500;            // pisca 500ms
            }

            // só rearmar quando a faixa (delay) mudar
            if (delay != prev_delay) {
                // toggle imediato somente quando entrar em zona de pisca
                bool do_immediate_toggle = (delay == 300 || delay == 500);
                rearm_led_timer(delay, do_immediate_toggle);
                prev_delay = delay;
            }

            flag_timer = false;
        }
        // sem sleep_* e sem tight_loop_contents()
    }
}
