#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

const int PIN_LED_B = 4;
const float conversion_factor = 3.3f / (1 << 12);

static repeating_timer_t timer;
static bool led_enabled = false;
static bool led_state = false;

bool timer_callback(repeating_timer_t *rt) {
    if (led_enabled) {
        led_state = !led_state;
        gpio_put(PIN_LED_B, led_state);
    }
    return true;
}

int main() {
    stdio_init_all();
    
    gpio_init(PIN_LED_B);
    gpio_set_dir(PIN_LED_B, GPIO_OUT);
    gpio_put(PIN_LED_B, 0);
    
    adc_init();
    adc_gpio_init(27);
    adc_select_input(1);
    
    uint64_t next_adc_time = time_us_64();
    
    while (1) {
        if (time_us_64() >= next_adc_time) {
            uint16_t result = adc_read();
            float voltage = result * conversion_factor;
            
            cancel_repeating_timer(&timer);
            
            if (voltage < 1.0f) {
                led_enabled = false;
                gpio_put(PIN_LED_B, 0);
                led_state = false;
            }
            else if (voltage < 2.0f) {
                led_enabled = true;
                add_repeating_timer_ms(150, timer_callback, NULL, &timer);
            }
            else {
                led_enabled = true;
                add_repeating_timer_ms(400, timer_callback, NULL, &timer);
            }
            
            next_adc_time = time_us_64() + 100000;
        }
    }
}