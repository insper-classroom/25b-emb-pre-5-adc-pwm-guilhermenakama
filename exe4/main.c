/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

const int PIN_LED_B = 4;                 // LED no GP4
const float conversion_factor = 3.3f / (1 << 12); // tensão = raw * factor

// Leitura do ADC em alta frequência para reduzir latência do teste
#define CHECK_MS        5        // checa o ADC a cada 5 ms

// Metade do período (intervalo entre alternâncias)
#define TOGGLE_MS_Z1    150      // período total: 300 ms
#define TOGGLE_MS_Z2    250      // período total: 500 ms

static repeating_timer_t blink_timer;
static bool timer_running = false;
static bool led_on = false;

// Alterna o LED (callback do timer)
static bool blink_cb(repeating_timer_t *t) {
    led_on = !led_on;
    gpio_put(PIN_LED_B, led_on);
    return true; // continua repetindo
}

static inline void start_blink_timer(int interval_ms, bool start_on) {
    if (timer_running) cancel_repeating_timer(&blink_timer);
    led_on = start_on;
    gpio_put(PIN_LED_B, led_on);
    add_repeating_timer_ms(interval_ms, blink_cb, NULL, &blink_timer);
    timer_running = true;
}

static inline void stop_blink_timer(void) {
    if (timer_running) {
        cancel_repeating_timer(&blink_timer);
        timer_running = false;
    }
    led_on = false;
    gpio_put(PIN_LED_B, 0);
}

// Lê tensão no GP28/ADC2
static inline float read_voltage(void) {
    uint16_t raw = adc_read();           // 0..4095
    return raw * conversion_factor;      // 0..3.3 V
}

/**
 * Zonas:
 * 0..1.0V: Desligado
 * 1..2.0V: 300 ms (toggle 150 ms)
 * 2..3.3V: 500 ms (toggle 250 ms)
 */
int main() {
    // stdio_init_all(); // evitar prints no teste

    // LED
    gpio_init(PIN_LED_B);
    gpio_set_dir(PIN_LED_B, GPIO_OUT);
    gpio_put(PIN_LED_B, 0);

    // ADC em GP28 (ADC2)
    adc_init();
    adc_gpio_init(28);
    adc_select_input(2);

    absolute_time_t last = get_absolute_time();
    int last_zone = -1;

    while (1) {
        absolute_time_t now = get_absolute_time();
        if (absolute_time_diff_us(last, now) >= (CHECK_MS * 1000)) {
            float v = read_voltage();
            int zone = (v < 1.0f) ? 0 : (v < 2.0f) ? 1 : 2;

            if (zone != last_zone) {
                if (zone == 0) {
                    stop_blink_timer();                     // sempre apagado
                } else if (zone == 1) {
                    start_blink_timer(TOGGLE_MS_Z1, true);  // liga já e alterna a cada 150 ms
                } else {
                    start_blink_timer(TOGGLE_MS_Z2, true);  // liga já e alterna a cada 250 ms
                }
                last_zone = zone;
            }
            last = now;
        }
        // sem sleep/busy-wait/tight_loop
    }
}
