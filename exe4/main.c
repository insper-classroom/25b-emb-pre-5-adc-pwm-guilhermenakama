#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

const int PIN_LED_B = 4;
const float conversion_factor = 3.3f / (1 << 12);

static struct repeating_timer timer;
static bool timer_active = false;
static bool led_state = false;
static int current_zone = -1;  // -1 = não inicializado, 0 = desligado, 1 = 300ms, 2 = 500ms

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
    adc_gpio_init(28);  // GP28 - onde está o potenciômetro
    adc_select_input(2); // Canal 2 para GP28
    
    uint64_t last_check = 0;
    
    while (1) {
        uint64_t now = time_us_64();
        
        // Verifica ADC a cada 100ms (100000 microsegundos)
        if (now - last_check >= 100000) {
            uint16_t result = adc_read();
            float voltage = result * conversion_factor;
            
            // Determina a zona baseada na tensão
            int new_zone;
            if (voltage < 1.0f) {
                new_zone = 0;  // Zona 0: desligado
            } else if (voltage < 2.0f) {
                new_zone = 1;  // Zona 1: 300ms
            } else {
                new_zone = 2;  // Zona 2: 500ms
            }
            
            // Só atualiza se mudou de zona
            if (new_zone != current_zone) {
                // Cancela timer anterior se estiver ativo
                if (timer_active) {
                    cancel_repeating_timer(&timer);
                    timer_active = false;
                }
                
                // Configura novo comportamento baseado na zona
                if (new_zone == 0) {
                    // Zona 0: LED desligado
                    gpio_put(PIN_LED_B, 0);
                } else if (new_zone == 1) {
                    // Zona 1: pisca com período de 300ms (150ms on/off)
                    add_repeating_timer_ms(150, timer_callback, NULL, &timer);
                    timer_active = true;
                } else {
                    // Zona 2: pisca com período de 500ms (250ms on/off)
                    add_repeating_timer_ms(250, timer_callback, NULL, &timer);
                    timer_active = true;
                }
                
                current_zone = new_zone;
            }
            
            last_check = now;
        }
        
        // Loop vazio - não usa tight_loop_contents() conforme requisito
    }
}