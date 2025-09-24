#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>

#include "data.h"
QueueHandle_t xQueueData;

void data_task(void *p) {
    vTaskDelay(pdMS_TO_TICKS(400));

    int data_len = sizeof(sine_wave_four_cycles) / sizeof(sine_wave_four_cycles[0]);
    for (int i = 0; i < data_len; i++) {
        xQueueSend(xQueueData, &sine_wave_four_cycles[i], 1000000);
    }

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void process_task(void *p) {
    int data = 0;
    static int buffer[5] = {0};  // Buffer inicializado com zeros
    static int buffer_index = 0;

    while (true) {
        if (xQueueReceive(xQueueData, &data, 100)) {
            // Adiciona o novo valor no buffer circular
            buffer[buffer_index] = data;
            buffer_index = (buffer_index + 1) % 5;
            
            // Calcula a soma de TODOS os 5 elementos do buffer
            int sum = 0;
            for (int i = 0; i < 5; i++) {
                sum += buffer[i];
            }
            
            // SEMPRE divide por 5 (tamanho da janela)
            int average = sum / 5;
            printf("%d\n", average);

            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

int main() {
    stdio_init_all();

    xQueueData = xQueueCreate(64, sizeof(int));

    xTaskCreate(data_task, "Data task ", 4096, NULL, 1, NULL);
    xTaskCreate(process_task, "Process task", 4096, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}