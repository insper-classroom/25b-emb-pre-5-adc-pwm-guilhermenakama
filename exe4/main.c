#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

const int PIN_LED_B = 4;
const float conversion_factor = 3.3f / (1 << 12);

// Timers
static repeating_timer_t led_timer;
static repeating_timer_t adc_timer;
static bool led_timer_active = false;
static bool led_state = false;

// Callback para piscar o LED
bool led_callback(repeating_timer_t *rt) {
    led_state = !led_state;
    gpio_put(PIN_LED_B, led_state);
    return true;  // Continua repetindo
}

// Callback para ler o ADC e ajustar o LED
bool adc_callback(repeating_timer_t *rt) {
    // Lê o ADC
    uint16_t result = adc_read();
    float voltage = result * conversion_factor;
    
    // Cancela o timer do LED se estiver ativo
    if (led_timer_active) {
        cancel_repeating_timer(&led_timer);
        led_timer_active = false;
    }
    
    // Define o comportamento baseado na tensão
    if (voltage < 1.0f) {
        // Zona 0: LED desligado
        gpio_put(PIN_LED_B, 0);
        led_state = false;
    }
    else if (voltage >= 1.0f && voltage < 2.0f) {
        // Zona 1: Piscar com período de 300ms (150ms on, 150ms off)
        add_repeating_timer_ms(150, led_callback, NULL, &led_timer);
        led_timer_active = true;
    }
    else {
        // Zona 2: Piscar com período de 500ms (250ms on, 250ms off)  
        add_repeating_timer_ms(250, led_callback, NULL, &led_timer);
        led_timer_active = true;
    }
    
    return true;  // Continua repetindo
}

int main() {
    stdio_init_all();
    
    // Configura o LED
    gpio_init(PIN_LED_B);
    gpio_set_dir(PIN_LED_B, GPIO_OUT);
    gpio_put(PIN_LED_B, 0);
    
    // Configura o ADC
    adc_init();
    adc_gpio_init(28);  // GP28 - onde está conectado o potenciômetro
    adc_select_input(2); // Canal 2 corresponde ao GP28
    
    // Configura timer para ler o ADC a cada 100ms
    add_repeating_timer_ms(100, adc_callback, NULL, &adc_timer);
    
    // Loop infinito sem tight_loop_contents()
    while (1) {
        // Aguarda sem fazer nada
        __wfi(); // Wait For Interrupt - economiza energia
    }
}