#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

const int PIN_LED_B = 4;
const float conversion_factor = 3.3f / (1 << 12);

// Controle do timer do LED
static struct repeating_timer led_timer;
static struct repeating_timer adc_timer;
static int current_period_ms = 0;  // 0 = desligado, 150 = zona 1, 250 = zona 2

// Callback para piscar o LED
bool led_blink_callback(struct repeating_timer *t) {
    static bool led_state = false;
    led_state = !led_state;
    gpio_put(PIN_LED_B, led_state);
    return true;
}

// Callback para verificar o ADC
bool adc_check_callback(struct repeating_timer *t) {
    // Lê o ADC (já está selecionado o canal 2)
    uint16_t adc_raw = adc_read();
    float voltage = adc_raw * conversion_factor;
    
    // Determina o período necessário baseado na tensão
    int new_period_ms = 0;
    
    if (voltage < 1.0f) {
        // Zona 0: desligado
        new_period_ms = 0;
    }
    else if (voltage < 2.0f) {
        // Zona 1: 300ms de período (150ms on/off)
        new_period_ms = 150;
    }
    else {
        // Zona 2: 500ms de período (250ms on/off)
        new_period_ms = 250;
    }
    
    // Se mudou de zona, atualiza o comportamento
    if (new_period_ms != current_period_ms) {
        // Cancela timer anterior se existir
        if (current_period_ms > 0) {
            cancel_repeating_timer(&led_timer);
        }
        
        // Configura novo comportamento
        if (new_period_ms == 0) {
            // Zona 0: desliga o LED
            gpio_put(PIN_LED_B, 0);
        }
        else {
            // Zona 1 ou 2: inicia o piscar
            gpio_put(PIN_LED_B, 0); // Estado inicial conhecido
            add_repeating_timer_ms(new_period_ms, led_blink_callback, NULL, &led_timer);
        }
        
        current_period_ms = new_period_ms;
    }
    
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
    adc_gpio_init(28);   // GP28 = potenciômetro
    adc_select_input(2); // Canal 2 = GP28
    
    // Timer para verificar o ADC a cada 50ms
    add_repeating_timer_ms(50, adc_check_callback, NULL, &adc_timer);
    
    // Loop principal sem busy waiting
    while (1) {
        __wfi(); // Wait for interrupt
    }
}