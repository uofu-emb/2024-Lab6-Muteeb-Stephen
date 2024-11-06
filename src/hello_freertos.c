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

int x = 0;
SemaphoreHandle_t xSemaphore;

#define SUPERVISOR_TASK_PRIORITY (tskIDLE_PRIORITY + 1UL)
#define SUBORDINATE_TASK_1_PRIORITY (tskIDLE_PRIORITY + 2UL)
#define SUBORDINATE_TASK_2_PRIORITY (tskIDLE_PRIORITY + 3UL)
#define SUBORDINATE_TASK_3_PRIORITY (tskIDLE_PRIORITY + 4UL)
#define SUPERVISOR_TASK_STACK_SIZE configMINIMAL_STACK_SIZE
#define SUBORDINATE_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

// Low priority task
void subordinate_task_low(__unused void *params)
{
    while (1)
    {
        printf("starting low subordinate task while loop\n");
        // Wait for semaphore to become available
        if (xSemaphoreTake(xSemaphore, 10000) == pdTRUE)
        {
            // Take semaphore, then run busy wait loop to simulate a computation on a shared variable, then give back semaphore
            printf("Starting computation in low subordinate task\n");
            sleep_ms(10000);
            printf("Finished computation giving back semaphore in low subordinate task\n");
            xSemaphoreGive(xSemaphore);
            sleep_ms(10000);
        }
    }
}

// Medium priority task
void subordinate_task_medium(__unused void *params)
{
    while (1)
    {

        printf("Starting computation 2 in medium subordinate task\n");
        // Run busy wait loop to simulate computing something
        sleep_ms(20000);
        printf("Finished computation 2 in medium subordinate task\n");
    }
}

// High priority task
void subordinate_task_high(__unused void *params)
{
    while (1)
    {
        printf("starting high subordinate task while loop\n");
        if (xSemaphoreTake(xSemaphore, 10000) == pdTRUE)
        {
            // Take semaphore, then run busy wait loop to simulate a computation on a shared variable,
            // then give back semaphore
            printf("Starting computation in high subordinate task\n");
            sleep_ms(5000);
            printf("Finished computation giving back semaphore in high subordinate task\n");
            xSemaphoreGive(xSemaphore);
        }
    }
}

void supervisor_task(__unused void *params)
{
    // Create binary semaphore
    xSemaphore = xSemaphoreCreateBinary();
    if (xSemaphore == NULL)
    {
        printf("Failed to create binary semaphore.\n");
        return;
    }

    // Initially set the semaphore to available state.
    xSemaphoreGive(xSemaphore);

    // Start low task and delay to have it take semaphore
    printf("Stating low priority task\n");
    xTaskCreate(subordinate_task_low, "SubordinateLow",
                SUBORDINATE_TASK_STACK_SIZE, NULL, SUBORDINATE_TASK_3_PRIORITY, NULL);
    vTaskDelay(1);

    xTaskCreate(subordinate_task_high, "SubordinateHigh",
                SUBORDINATE_TASK_STACK_SIZE, NULL, SUBORDINATE_TASK_1_PRIORITY, NULL);

    vTaskDelay(1);
    // Start medium task to have it interrupt low task and start computation
    printf("Stating medium priority task\n");
    xTaskCreate(subordinate_task_medium, "SubordinateMedium",
                SUBORDINATE_TASK_STACK_SIZE, NULL, SUBORDINATE_TASK_2_PRIORITY, NULL);

    // Start high task, but it won't be able to do anything since low task has semaphore
    // printf("Stating high priority...\n");

    vTaskDelay(100000);

    // Expect to see:
    // starting low prioirty task, starting computation in low priority task
    // starting medium prioirty task, starting computation in medium priority task
    // starting high prioirty
    // finished computation in medium priority task
    // finished computation in low priority task
    // starting computation in high priority task

    while (1)
    {
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
