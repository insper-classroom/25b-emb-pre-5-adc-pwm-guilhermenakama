#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

const int PIN_LED_B = 4;
const float conversion_factor = 3.3f / (1 << 12);

// Variáveis globais
volatile bool flag_timer_adc = false;
struct repeating_timer timer_adc;
struct repeating_timer timer_led;
bool timer_led_active = false;
int delay_atual = 0;

// Timer callback para verificar o potenciômetro a cada 10ms
bool timer_adc_callback(struct repeating_timer *t) {
    flag_timer_adc = true;
    return true;
}

// Timer callback para piscar o LED
bool timer_led_callback(struct repeating_timer *t) {
    static bool state = false;
    state = !state;
    gpio_put(PIN_LED_B, state);
    return true;
}

int main() {
    stdio_init_all();
    
    // Inicia o ADC
    adc_init();
    adc_gpio_init(28);   // GP28
    adc_select_input(2); // Canal 2
    
    // Inicia o LED na placa apagado
    gpio_init(PIN_LED_B);
    gpio_set_dir(PIN_LED_B, GPIO_OUT);
    gpio_put(PIN_LED_B, 0);
    
    // Cria timer para verificar ADC a cada 10ms
    add_repeating_timer_ms(10, timer_adc_callback, NULL, &timer_adc);
    
    int delay = 0;
    
    while (1) {
        // Verifica se é hora de ler o potenciômetro
        if (flag_timer_adc) {
            // Lê o valor do potenciômetro em volts
            uint16_t adc_raw = adc_read();
            float potenciometro = adc_raw * conversion_factor;
            
            // Verifica em qual faixa está e se mudou
            if (potenciometro < 1.0f && delay != 0) {
                // Deixa LED apagado
                if (timer_led_active) {
                    cancel_repeating_timer(&timer_led);
                    timer_led_active = false;
                }
                gpio_put(PIN_LED_B, 0);
                delay = 0;
            } 
            else if (potenciometro >= 1.0f && potenciometro < 2.0f && delay != 300) {
                // LED piscando com período de 300ms (150ms on + 150ms off)
                if (timer_led_active) {
                    cancel_repeating_timer(&timer_led);
                    timer_led_active = false;
                }
                gpio_put(PIN_LED_B, 0);
                add_repeating_timer_ms(150, timer_led_callback, NULL, &timer_led);
                timer_led_active = true;
                delay = 300;
            } 
            else if (potenciometro >= 2.0f && delay != 500) {
                // LED piscando com período de 500ms (250ms on + 250ms off)
                if (timer_led_active) {
                    cancel_repeating_timer(&timer_led);
                    timer_led_active = false;
                }
                gpio_put(PIN_LED_B, 0);
                add_repeating_timer_ms(250, timer_led_callback, NULL, &timer_led);
                timer_led_active = true;
                delay = 500;
            }
            
            flag_timer_adc = false;
        }
    }
}