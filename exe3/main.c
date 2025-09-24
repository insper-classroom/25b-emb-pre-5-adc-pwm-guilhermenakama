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
    static int samples[5] = {0}; 
    static int count = 0;        

    while (true) {
        if (xQueueReceive(xQueueData, &data, 100)) {
            for (int i = 0; i < 4; i++) {
                samples[i] = samples[i + 1];
            }
            
            samples[4] = data;
            
            if (count < 5) {
                count++;
            }
            
            int sum = 0;
            for (int i = 5 - count; i < 5; i++) {
                sum += samples[i];
            }
            
            int filtered = sum / count;
            printf("%d\n", filtered);

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