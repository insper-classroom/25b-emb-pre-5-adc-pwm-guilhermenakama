#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

const int LED_PIN = 4;
const float volt_conversion = 3.3f / (1 << 12); // 12 bits => 4096

volatile bool troca_estado_led = false;
volatile int time_delay = 0;

// Callback do timer: apenas marca flag e retorna true
bool led_timer_callback(repeating_timer_t *rt) {    
  troca_estado_led = true;        // Sinaliza evento    
  return true;                    // true=continua, false=para
}

int set_zone(float voltage) {
    if (voltage < 1.0f) {
        return 0;
    } else if (voltage < 2.0f) {
        return 1;
    } else {
        return 2;
    }
}

int time_delay_for_zone(int zone) {
    if (zone == 0) {
        return 0;
    } else if (zone == 1) {
        return 300;
    } else {
        return 500;
    }
}

void init_pin() {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    adc_init();
    adc_gpio_init(28);
    adc_select_input(2);
    printf("ADC initialized\n");
}


int main() {
    stdio_init_all();
    init_pin();

    //uint64_t last_check = time_us_64();
    int delay = 0;
    int led_state = 0;
    int last_zone = 0;
    int current_zone = 0;
    repeating_timer_t timer_0;
    
    while(1) {

        uint16_t result = adc_read();
        float voltage = result * volt_conversion;

        current_zone = set_zone(voltage);
        delay = time_delay_for_zone(current_zone);

        // Se mudou de zona, atualiza o timer
        if (current_zone != last_zone) {
            cancel_repeating_timer(&timer_0);
            if (current_zone == 0) {
                gpio_put(LED_PIN, 0);
            } else {
                printf("Changing to zone %d with delay %d ms\n", current_zone, delay);
                if (!add_repeating_timer_us(delay * 1000,
                                        led_timer_callback,
                                        NULL,
                                        &timer_0)) {
                    printf("Failed to add timer\n");
                }
            }
            last_zone = current_zone;
            printf("Zone: %d, Delay: %d ms, Voltage: %.2f V\n", current_zone, delay, voltage);
        }

        if(troca_estado_led){
            troca_estado_led = false;
            led_state = !led_state;
            printf("LED %s\n", led_state ? "ON" : "OFF");
            gpio_put(LED_PIN, led_state);
        }
    }
}       
