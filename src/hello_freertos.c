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
TaskHandle_t subordinate_task_low, subordinate_task_medium, subordinate_task_high;


#define SUPERVISOR_TASK_PRIORITY (tskIDLE_PRIORITY + 4UL)
#define SUBORDINATE_HIGH_TASK_PRIORITY (tskIDLE_PRIORITY + 3UL)
#define SUBORDINATE_MEDIUM_TASK_PRIORITY (tskIDLE_PRIORITY + 2UL)
#define SUBORDINATE_LOW_TASK_PRIORITY (tskIDLE_PRIORITY + 1UL)
#define SUPERVISOR_TASK_STACK_SIZE configMINIMAL_STACK_SIZE
#define SUBORDINATE_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

void printTaskInfo(TaskHandle_t xTask)
{
    TaskStatus_t xTaskStatus;
    eTaskState eTaskState;
    uint32_t ulFreeStackSpace;

    // Retrieve task information
    vTaskGetInfo(xTask, &xTaskStatus, pdTRUE, eTaskState);

    // Get the task state
    eTaskState = xTaskStatus.eCurrentState;

    // Get the free stack space if required
    ulFreeStackSpace = xTaskStatus.usStackHighWaterMark;

    // Print task information
    printf("\tTask Name: %s, Task Priority: %u\n", xTaskStatus.pcTaskName, xTaskStatus.uxCurrentPriority);
    // printf("Task State: %d, ", eTaskState);
    // printf("Task Priority: %u", xTaskStatus.uxCurrentPriority);
    // printf("Free Stack Space: %lu\n", ulFreeStackSpace);
}

// Low priority task
void handle_subordinate_task_low(__unused void *params)
{
    while (1)
    {
        printf("Low :: In while loop\n");
        // Wait for semaphore to become available
        if (xSemaphoreTake(xSemaphore, 10000) == pdTRUE)
        {
            // Take semaphore, then run busy wait loop to simulate a computation on a shared variable, then give back semaphore
            printf("Low :: Starting computation take.\n");
            printTaskInfo(NULL); // NULL means print running task.
            sleep_ms(10000);
            printf("Low :: Finished computation giving back.\n");
            xSemaphoreGive(xSemaphore);
            sleep_ms(1000); // Give chance to high priority.
        }
    }
}

// Medium priority task
void handle_subordinate_task_medium(__unused void *params)
{
    while (1)
    {

        printf("Medium :: Starting computation.\n");
        printf("Medium :: Printing Low Task Info.\n");
        printTaskInfo(subordinate_task_low); // NULL means print running task.

        // Run busy wait loop to simulate computing something
        printf("Medium :: Printing Medium Task Info.");
        printTaskInfo(NULL);
        busy_wait_ms(20000);
        printf("Medium :: Finished computation.\n");
    }
}

// High priority task
void handle_subordinate_task_high(__unused void *params)
{
    while (1)
    {
        printf("High :: In while loop\n");
        if (xSemaphoreTake(xSemaphore, 10000) == pdTRUE)
        {
            // Take semaphore, then run busy wait loop to simulate a computation on a shared variable,
            // then give back semaphore
            printf("High :: Starting computation take.\n");
            printTaskInfo(NULL); // NULL means print running task.
            sleep_ms(5000);
            printf("High :: Finished computation giving back .\n");
            xSemaphoreGive(xSemaphore);
        }
    }
}

void supervisor_task(__unused void *params)
{
    // Create binary semaphore
    xSemaphore = xSemaphoreCreateMutex();
    if (xSemaphore == NULL)
    {
        printf("Failed to create mutex.\n");
        return;
    }

    // Initially set the semaphore to available state.
    xSemaphoreGive(xSemaphore);

    // Start low task and delay to have it take semaphore
    printf("Starting low priority task...\n");
    xTaskCreate(handle_subordinate_task_low, "SubordinateLow",
                SUBORDINATE_TASK_STACK_SIZE, NULL, SUBORDINATE_LOW_TASK_PRIORITY, &subordinate_task_low);
    vTaskDelay(1);

    printf("Starting high priority task...\n");
    xTaskCreate(handle_subordinate_task_high, "SubordinateHigh",
                SUBORDINATE_TASK_STACK_SIZE, NULL, SUBORDINATE_HIGH_TASK_PRIORITY, &subordinate_task_high);

    vTaskDelay(1);
    // Start medium task to have it interrupt low task and start computation
    printf("Starting medium priority task...\n");
    xTaskCreate(handle_subordinate_task_medium, "SubordinateMedium",
                SUBORDINATE_TASK_STACK_SIZE, NULL, SUBORDINATE_MEDIUM_TASK_PRIORITY, &subordinate_task_medium);

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

    xTaskCreate(supervisor_task, "SupervisorThread",
                SUPERVISOR_TASK_STACK_SIZE, NULL, SUPERVISOR_TASK_PRIORITY, NULL);
    vTaskStartScheduler();

    return 0;
}
