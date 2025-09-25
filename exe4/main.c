#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

const int PIN_LED_B = 4;
const float conversion_factor = 3.3f / (1 << 12);

// Flags e variáveis de controle
volatile bool flag_timer_adc = false;
volatile bool flag_timer_led = false;
int faixa_atual = -1;
int faixa_anterior = -1;

// Timers
struct repeating_timer timer_adc;
struct repeating_timer timer_led;
bool timer_led_ativo = false;

// Callback do timer para verificar o potenciômetro a cada 10ms
bool timer_adc_callback(struct repeating_timer *t) {
    flag_timer_adc = true;
    return true; // continua repetindo
}

// Callback do timer para piscar o LED
bool timer_led_callback(struct repeating_timer *t) {
    flag_timer_led = !flag_timer_led;
    gpio_put(PIN_LED_B, flag_timer_led);
    return true; // continua repetindo
}

// Lê o potenciômetro e identifica qual faixa está
int read_potentiometer() {
    // Lê o valor do ADC
    uint16_t adc_raw = adc_read();
    float voltage = adc_raw * conversion_factor;
    
    if (voltage < 1.0f) {
        return 0; // Faixa 0-1V
    } else if (voltage < 2.0f) {
        return 1; // Faixa 1-2V  
    } else {
        return 2; // Faixa 2-3.3V
    }
}

// Configura o LED baseado na faixa
void configura_led(int faixa) {
    // Cancela timer anterior do LED se existir
    if (timer_led_ativo) {
        cancel_repeating_timer(&timer_led);
        timer_led_ativo = false;
        flag_timer_led = false;
    }
    
    if (faixa == 0) {
        // LED desligado
        gpio_put(PIN_LED_B, 0);
    } else if (faixa == 1) {
        // LED piscando com período de 300ms (150ms on + 150ms off)
        gpio_put(PIN_LED_B, 0);  // Começa desligado
        add_repeating_timer_ms(150, timer_led_callback, NULL, &timer_led);
        timer_led_ativo = true;
    } else {
        // LED piscando com período de 500ms (250ms on + 250ms off)
        gpio_put(PIN_LED_B, 0);  // Começa desligado
        add_repeating_timer_ms(250, timer_led_callback, NULL, &timer_led);
        timer_led_ativo = true;
    }
}

int main() {
    stdio_init_all();
    
    // Configura o GPIO do LED
    gpio_init(PIN_LED_B);
    gpio_set_dir(PIN_LED_B, GPIO_OUT);
    gpio_put(PIN_LED_B, 0);
    
    // Configura o ADC
    adc_init();
    adc_gpio_init(28);   // GP28 - onde está o potenciômetro
    adc_select_input(2); // Canal 2 corresponde ao GP28
    
    // Configura timer para verificar o ADC a cada 10ms
    add_repeating_timer_ms(10, timer_adc_callback, NULL, &timer_adc);
    
    while (1) {
        // Verifica se é hora de ler o potenciômetro
        if (flag_timer_adc) {
            // Lê o valor do potenciômetro e identifica a faixa
            faixa_atual = read_potentiometer();
            
            // Verifica se mudou de faixa
            if (faixa_atual != faixa_anterior) {
                // Configura o LED de acordo com a nova faixa
                configura_led(faixa_atual);
                
                // Atualiza a faixa anterior
                faixa_anterior = faixa_atual;
            }
            
            // Limpa a flag
            flag_timer_adc = false;
        }
    }
}