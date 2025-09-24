#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

const int PIN_LED_B = 4;
const float conversion_factor = 3.3f / (1 << 12);

static struct repeating_timer timer;
static bool timer_active = false;
static bool led_state = false;  // Estado global do LED
static int current_zone = -1;

// Callback para piscar o LED
bool timer_callback(struct repeating_timer *rt) {
    led_state = !led_state;
    gpio_put(PIN_LED_B, led_state);
    return true;
}

int main() {
    stdio_init_all();
    
    // Configura o LED
    gpio_init(PIN_LED_B);
    gpio_set_dir(PIN_LED_B, GPIO_OUT);
    gpio_put(PIN_LED_B, 0);
    
    // Configura o ADC
    adc_init();
    adc_gpio_init(28);   // GP28
    adc_select_input(2); // Canal 2
    
    uint64_t last_check = 0;
    
    while (1) {
        uint64_t now = time_us_64();
        
        // Verifica a cada 100ms
        if (now - last_check >= 100000) {
            uint16_t result = adc_read();
            float voltage = result * conversion_factor;
            
            int new_zone;
            if (voltage < 1.0f) {
                new_zone = 0;
            } else if (voltage < 2.0f) {
                new_zone = 1;
            } else {
                new_zone = 2;
            }
            
            // Só muda se for uma zona diferente
            if (new_zone != current_zone) {
                // Para o timer anterior
                if (timer_active) {
                    cancel_repeating_timer(&timer);
                    timer_active = false;
                }
                
                if (new_zone == 0) {
                    // Zona 0: LED desligado
                    gpio_put(PIN_LED_B, 0);
                    led_state = false;
                } else if (new_zone == 1) {
                    // Zona 1: 300ms (150ms + 150ms)
                    led_state = false;  // Reseta o estado
                    gpio_put(PIN_LED_B, 0); // Começa desligado
                    add_repeating_timer_ms(150, timer_callback, NULL, &timer);
                    timer_active = true;
                } else {
                    // Zona 2: 500ms (250ms + 250ms)
                    led_state = false;  // Reseta o estado
                    gpio_put(PIN_LED_B, 0); // Começa desligado
                    add_repeating_timer_ms(250, timer_callback, NULL, &timer);
                    timer_active = true;
                }
                
                current_zone = new_zone;
            }
            
            last_check = now;
        }
    }
}