#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

const int PIN_LED_B = 4;
const float conversion_factor = 3.3f / (1 << 12);

static repeating_timer_t timer;
static bool timer_active = false;
static bool led_state = false;

bool timer_callback(repeating_timer_t *rt) {
    led_state = !led_state;
    gpio_put(PIN_LED_B, led_state);
    return true;
}

int main() {
    stdio_init_all();
    
    gpio_init(PIN_LED_B);
    gpio_set_dir(PIN_LED_B, GPIO_OUT);
    gpio_put(PIN_LED_B, 0);
    
    adc_init();
    adc_gpio_init(27);
    
    uint64_t last_check = 0;
    
    while (1) {
        uint64_t now = time_us_64();
        
        if (now - last_check >= 100000) {
            adc_select_input(1);
            uint16_t result = adc_read();
            float voltage = result * conversion_factor;
            
            if (timer_active) {
                cancel_repeating_timer(&timer);
                timer_active = false;
            }
            
            if (voltage >= 1.0f && voltage < 2.0f) {
                add_repeating_timer_ms(300, timer_callback, NULL, &timer);
                timer_active = true;
            }
            else if (voltage >= 2.0f) {
                add_repeating_timer_ms(500, timer_callback, NULL, &timer);
                timer_active = true;
            }
            else {
                gpio_put(PIN_LED_B, 0);
                led_state = false;
            }
            
            last_check = now;
        }
        
        tight_loop_contents();
    }
}