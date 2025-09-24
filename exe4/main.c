#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#define LED_PIN        4      // LED externo no GP4 (led1:A)
#define ADC_INPUT      2      // GP28 = ADC2  (pot1:SIG)
#define VREF           3.3f
#define ADC_MAX        4095.0f
#define CHECK_MS       5      // leitura do ADC a cada 5 ms (↓ latência)

// Intervalo entre alternâncias (metade do período)
#define TOGGLE_MS_Z1   150    // período total: 300 ms
#define TOGGLE_MS_Z2   250    // período total: 500 ms

static repeating_timer_t blink_timer;
static bool timer_running = false;
static bool led_on = false;

// Callback do timer: alterna o LED
static bool blink_cb(repeating_timer_t *t) {
    led_on = !led_on;
    gpio_put(LED_PIN, led_on);
    return true; // continua repetindo
}

static inline void start_blink_timer(int interval_ms, bool start_on) {
    if (timer_running) cancel_repeating_timer(&blink_timer);
    // Estado inicial desejado ao entrar na zona
    led_on = start_on;
    gpio_put(LED_PIN, led_on);
    // Próxima alternância acontecerá após "interval_ms"
    add_repeating_timer_ms(interval_ms, blink_cb, NULL, &blink_timer);
    timer_running = true;
}

static inline void stop_blink_timer(void) {
    if (timer_running) {
        cancel_repeating_timer(&blink_timer);
        timer_running = false;
    }
    led_on = false;
    gpio_put(LED_PIN, 0);
}

static inline float read_voltage(void) {
    const uint16_t raw = adc_read();         // 0..4095
    return (raw * VREF) / ADC_MAX;           // 0..3.3 V
}

int main() {
    // LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    // ADC em GP28/ADC2
    adc_init();
    adc_gpio_init(28);
    adc_select_input(ADC_INPUT);

    absolute_time_t last = get_absolute_time();
    int last_zone = -1;

    while (true) {
        absolute_time_t now = get_absolute_time();
        if (absolute_time_diff_us(last, now) >= (CHECK_MS * 1000)) {
            const float v = read_voltage();

            // 0.0–1.0V -> zona 0; 1.0–2.0V -> zona 1; 2.0–3.3V -> zona 2
            const int zone = (v < 1.0f) ? 0 : (v < 2.0f) ? 1 : 2;

            if (zone != last_zone) {
                if (zone == 0) {
                    stop_blink_timer();                 // sempre apagado
                } else if (zone == 1) {
                    start_blink_timer(TOGGLE_MS_Z1, true);  // liga já e alterna a cada 150 ms
                } else { // zona 2
                    start_blink_timer(TOGGLE_MS_Z2, true);  // liga já e alterna a cada 250 ms
                }
                last_zone = zone;
            }
            last = now;
        }
        // Sem sleep/busy/tight_loop
    }
}
