#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

const int PIN_LED_B = 4;
const float conversion_factor = 3.3f / (1 << 12);

// Variáveis globais
struct repeating_timer timer;
bool timer_active = false;
bool led_state = false;  // Estado global do LED para garantir previsibilidade
int current_zone = -1;

// Callback para piscar LED
bool led_callback(struct repeating_timer *t) {
    led_state = !led_state;
    gpio_put(PIN_LED_B, led_state);
    return true;
}

int main() {
    stdio_init_all();
    
    // Configura LED
    gpio_init(PIN_LED_B);
    gpio_set_dir(PIN_LED_B, GPIO_OUT);
    gpio_put(PIN_LED_B, 0);
    
    // Configura ADC
    adc_init();
    adc_gpio_init(28);   // GP28 = potenciômetro
    adc_select_input(2); // Canal 2 = GP28
    
    // Controle de tempo para verificação
    uint64_t last_check = 0;
    
    while (1) {
        uint64_t now = time_us_64();
        
        // Verifica ADC a cada 10ms (10000 microsegundos)
        if (now - last_check >= 10000) {
            // Lê ADC
            uint16_t adc_value = adc_read();
            float voltage = adc_value * conversion_factor;
            
            // Determina zona
            int zone;
            if (voltage < 1.0f) {
                zone = 0;  // Desligado
            } else if (voltage < 2.0f) {
                zone = 1;  // 300ms (150ms + 150ms)
            } else {
                zone = 2;  // 500ms (250ms + 250ms)
            }
            
            // Se mudou de zona, reconfigura
            if (zone != current_zone) {
                // Para timer anterior se ativo
                if (timer_active) {
                    cancel_repeating_timer(&timer);
                    timer_active = false;
                }
                
                // Configura nova zona
                if (zone == 0) {
                    // Zona 0: LED desligado
                    led_state = false;
                    gpio_put(PIN_LED_B, 0);
                } else if (zone == 1) {
                    // Zona 1: pisca 300ms (timer de 150ms)
                    led_state = false;  // Reset estado para comportamento previsível
                    gpio_put(PIN_LED_B, 0);  // Começa desligado
                    add_repeating_timer_ms(150, led_callback, NULL, &timer);
                    timer_active = true;
                } else {
                    // Zona 2: pisca 500ms (timer de 250ms)
                    led_state = false;  // Reset estado para comportamento previsível
                    gpio_put(PIN_LED_B, 0);  // Começa desligado
                    add_repeating_timer_ms(250, led_callback, NULL, &timer);
                    timer_active = true;
                }
                
                current_zone = zone;
            }
            
            last_check = now;
        }
    }
    
    return 0;
}