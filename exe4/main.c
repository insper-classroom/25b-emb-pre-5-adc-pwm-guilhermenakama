#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/timer.h"

#define LED_PIN        4          // LED externo no GP4
#define ADC_INPUT      2          // GP28 = ADC2
#define VREF           3.3f
#define ADC_MAX        4095.0f
#define CHECK_MS       20         // ~50 Hz de leitura do ADC

// Metade do período de pisca (intervalo entre alternâncias)
#define TOGGLE_MS_Z1   150        
#define TOGGLE_MS_Z2   250        

static repeating_timer_t blink_timer;
static bool timer_running = false;
static bool led_state = false;

// Callback do timer: alterna o LED
static bool blink_cb(repeating_timer_t *t) {
    led_state = !led_state;
    gpio_put(LED_PIN, led_state);
    return true; // continua repetindo
}

// Inicia/reinicia o timer de pisca com o intervalo desejado
static void start_blink_timer(int interval_ms) {
    if (timer_running) cancel_repeating_timer(&blink_timer);
    led_state = false;
    gpio_put(LED_PIN, 0);
    add_repeating_timer_ms(interval_ms, blink_cb, NULL, &blink_timer);
    timer_running = true;
}

// Para o timer e garante LED apagado
static void stop_blink_timer(void) {
    if (timer_running) {
        cancel_repeating_timer(&blink_timer);
        timer_running = false;
    }
    led_state = false;
    gpio_put(LED_PIN, 0);
}

// Lê o ADC e converte para tensão (0–3.3 V)
static inline float read_voltage(void) {
    uint16_t raw = adc_read();
    return (raw * VREF) / ADC_MAX;
}

int main() {
    // Configura LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    // Configura ADC no GP28/ADC2
    adc_init();
    adc_gpio_init(28);
    adc_select_input(ADC_INPUT);

    uint64_t last_check = to_us_since_boot(get_absolute_time());
    int last_zone = -1;

    while (true) {
        uint64_t now = to_us_since_boot(get_absolute_time());
        if ((now - last_check) >= (CHECK_MS * 1000ULL)) {
            float v = read_voltage();

            int zone;
            if (v < 1.0f)       zone = 0;
            else if (v < 2.0f)  zone = 1;
            else                zone = 2;

            if (zone != last_zone) {
                switch (zone) {
                    case 0:
                        stop_blink_timer();
                        break;
                    case 1:
                        start_blink_timer(TOGGLE_MS_Z1);
                        break;
                    case 2:
                        start_blink_timer(TOGGLE_MS_Z2);
                        break;
                }
                last_zone = zone;
            }
            last_check = now;
        }
    }
}
