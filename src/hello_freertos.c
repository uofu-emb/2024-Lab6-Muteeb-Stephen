/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include <semphr.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/cyw43_arch.h"

bool shared_value = false;
SemaphoreHandle_t xSemaphore;

#define SUPERVISOR_TASK_PRIORITY (tskIDLE_PRIORITY + 1UL)
#define SUBORDINATE_TASK_1_PRIORITY (tskIDLE_PRIORITY + 2UL)
#define SUBORDINATE_TASK_2_PRIORITY (tskIDLE_PRIORITY + 3UL)
#define SUPERVISOR_TASK_STACK_SIZE configMINIMAL_STACK_SIZE
#define SUBORDINATE_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

void subordinate_task(__unused void *params)
{
    while (1)
    {
        printf("In the subordinate\n");
        if (xSemaphoreTake(xSemaphore, 10000) == pdTRUE)
        {
            shared_value = !shared_value;
            printf("Updated semaphore %d\n", shared_value);
            vTaskDelay(10000);
            xSemaphoreGive(xSemaphore);
        }
    }
}

void supervisor_task(__unused void *params)
{
    // printf("where0...\n");

    // printf("where1...\n");

    // printf("where2...\n");
    xSemaphore = xSemaphoreCreateBinary();
    if (xSemaphore == NULL)
    {
        printf("Failed to create binary semaphore.\n");
        return;
    }

    // Initially set the semaphore to available state.
    xSemaphoreGive(xSemaphore);
    // TaskHandle_t subordinate_task1, subordinate_task2;

    printf("Stating low priority...\n");
    xTaskCreate(subordinate_task, "SubordinateThread2",
                SUBORDINATE_TASK_STACK_SIZE, NULL, SUBORDINATE_TASK_2_PRIORITY, NULL);

    vTaskDelay(10000);

    printf("Stating high priority...\n");
    xTaskCreate(subordinate_task, "SubordinateThread1",
                SUBORDINATE_TASK_STACK_SIZE, NULL, SUBORDINATE_TASK_1_PRIORITY, NULL);
    while (1)
    {
        printf("Hi there!\n");
        vTaskDelay(1000);
    }
}

int main(void)
{
    stdio_init_all();
    hard_assert(cyw43_arch_init() == PICO_OK);
    sleep_ms(5000);
    printf("Starting...\n");

    // TaskHandle_t supervisor_task;
    xTaskCreate(supervisor_task, "SupervisorThread",
                SUPERVISOR_TASK_STACK_SIZE, NULL, SUPERVISOR_TASK_PRIORITY, NULL);
    vTaskStartScheduler();

    return 0;
}
