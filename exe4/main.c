#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

const int PIN_LED_B = 4;
const float volt_conversion = 3.3f / 4096.0f; // 12 bits => 4096

static struct repeating_timer timer;
volatile bool timer_active = false;   // indica se o timer está ativo
volatile bool timer_tick = false;     // flag de evento do timer
volatile bool led_on = false;         // estado lógico do LED
static int current_zone = 0;

// Callback do timer: apenas marca flag e retorna true
bool timer_callback(struct repeating_timer *rt) {
    (void)rt;
    timer_tick = true;  // sinaliza para o main que o tempo passou
    return true;        // mantém o timer ativo
}

int main() {
    stdio_init_all();
    
    // Configura o LED
    gpio_init(PIN_LED_B);
    gpio_set_dir(PIN_LED_B, GPIO_OUT);
    gpio_put(PIN_LED_B, 0);
    led_on = false;
    
    // Configura o ADC (GP28 -> ADC2)
    adc_init();
    adc_gpio_init(28);
    adc_select_input(2);
    
    uint64_t last_check = time_us_64();
    
    while (1) {
        uint64_t now = time_us_64();
        
        // 1) Ler ADC a cada 10 ms
        if (now - last_check >= 10000) {
            uint16_t result = adc_read();
            float voltage = result * volt_conversion;
            
            // 2) Determinar zona
            int new_zone;
            if (voltage < 1.0f) {
                new_zone = 0;
            } else if (voltage < 2.0f) {
                new_zone = 1;
            } else {
                new_zone = 2;
            }
            
            // 3) Se zona mudou, reconfigura o timer
            if (new_zone != current_zone) {
                if (timer_active) {
                    cancel_repeating_timer(&timer);
                    timer_active = false;
                }

                if (new_zone == 0) {
                    // LED apagado, sem timer
                    gpio_put(PIN_LED_B, 0);
                    led_on = false;
                } else {
                    // Zona 1 → 300 ms total (150 ms por ciclo)
                    // Zona 2 → 500 ms total (250 ms por ciclo)
                    int half_ms = (new_zone == 1) ? (300 / 2) : (500 / 2);
                    if (half_ms <= 0) half_ms = 1;

                    bool ok = add_repeating_timer_ms(half_ms, timer_callback, NULL, &timer);
                    if (ok) {
                        timer_active = true;
                        gpio_put(PIN_LED_B, 0);
                        led_on = false;
                    } else {
                        timer_active = false;
                        gpio_put(PIN_LED_B, 0);
                        led_on = false;
                    }
                }

                current_zone = new_zone;
            }

            last_check = now;
        }

        // 4) Se o timer disparou, alternar o LED
        if (timer_tick) {
            timer_tick = false;       // consome o evento
            led_on = !led_on;         // troca o estado lógico
            gpio_put(PIN_LED_B, led_on);
        }

        // loop leve — sem sleep
        tight_loop_contents();
    }
}
