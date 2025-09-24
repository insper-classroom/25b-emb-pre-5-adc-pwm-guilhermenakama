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
    static int samples[5];
    static int count = 0;

    while (true) {
        if (xQueueReceive(xQueueData, &data, 100)) {
            if (count < 5) {
                samples[count] = data;
                count++;
            } else {
                for (int i = 0; i < 4; i++) {
                    samples[i] = samples[i + 1];
                }
                samples[4] = data;
            }
            
            int sum = 0;
            int items = (count < 5) ? count : 5;
            for (int i = 0; i < items; i++) {
                sum += samples[i];
            }
            
            int filtered = sum / items;
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