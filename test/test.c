#include <stdio.h>
#include <pico/stdlib.h>
#include <stdint.h>
#include <unity.h>
#include "unity_config.h"
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include <semphr.h>

#include "pico/multicore.h"
#include "pico/cyw43_arch.h"

void setUp(void) {}

void tearDown(void) {}
/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */



int x = 0;
int y = 0;
SemaphoreHandle_t xSemaphore;

#define SUPERVISOR_TASK_PRIORITY (tskIDLE_PRIORITY + 4UL)
#define SUBORDINATE_TASK_HIGH_PRIORITY (tskIDLE_PRIORITY + 3UL)
#define SUBORDINATE_TASK_MED_PRIORITY (tskIDLE_PRIORITY + 2UL)
#define SUBORDINATE_TASK_LOW_PRIORITY (tskIDLE_PRIORITY + 1UL)
#define SUPERVISOR_TASK_STACK_SIZE configMINIMAL_STACK_SIZE
#define SUBORDINATE_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

//Low priority task
void subordinate_task_low(__unused void *params)
{
    while (1)
    {
        printf("starting low task while loop\n");
        //Wait for semaphore to become available
        if (xSemaphoreTake(xSemaphore, 10000) == pdTRUE)
        {
            //Take semaphore, then run busy wait loop to simulate a computation on a shared variable, then give back semaphore
            printf("Starting busy wait in low task\n");
            busy_wait_ms(10000);
            printf("Finished busy wait in low task\n");
            xSemaphoreGive(xSemaphore);
        }
    }
}

//Medium priority task
void subordinate_task_medium(__unused void *params)
{
    while (1)
    {
        printf("Starting busy wait in medium task\n");
        //Run busy wait loop to simulate computing something
        busy_wait_ms(5000);
        x = 1;
        printf("Finished busy wait in medium task\n");
    }
}

//High priority task
void subordinate_task_high(__unused void *params)
{
    while (1)
    {
        printf("starting high subordinate task while loop\n");
        if (xSemaphoreTake(xSemaphore, 10000) == pdTRUE)
        {
            //Take semaphore, then run busy wait loop to simulate a computation on a shared variable, 
            //then give back semaphore
            printf("Starting busy wait in high task\n");
            busy_wait_ms(10000);
            y = 1;
            printf("Finished busy wait in high task\n");
            xSemaphoreGive(xSemaphore);
        }
    }
}






void test_binary(void) {
    //initialize test variables
    x = 0;
    y = 0;
    //Create binary semaphore
    xSemaphore = xSemaphoreCreateBinary();
    if (xSemaphore == NULL)
    {
        printf("Failed to create binary semaphore.\n");
        return;
    }

    // Initially set the semaphore to available state.
    xSemaphoreGive(xSemaphore);

    //need these to delete tasks
    TaskHandle_t low, med, high;

    //Start low task and delay to have it take semaphore 
    printf("Stating low priority task\n");
    xTaskCreate(subordinate_task_low, "SubordinateLow",
                SUBORDINATE_TASK_STACK_SIZE, NULL, SUBORDINATE_TASK_LOW_PRIORITY, &low);
    vTaskDelay(1);

    //Start high task right after
    printf("Stating high priority task\n");
    xTaskCreate(subordinate_task_high, "SubordinateHigh",
                SUBORDINATE_TASK_STACK_SIZE, NULL, SUBORDINATE_TASK_HIGH_PRIORITY, &high);
    vTaskDelay(1);

    //Start medium task to have it interrupt low task and start computation
    printf("Stating medium priority task\n");
    xTaskCreate(subordinate_task_medium, "SubordinateMedium",
                SUBORDINATE_TASK_STACK_SIZE, NULL, SUBORDINATE_TASK_MED_PRIORITY, &med);


    //wait 20 seconds for tasks to run
    vTaskDelay(20000);

    //tear down
    vSemaphoreDelete(xSemaphore);
    vTaskDelete(low);
    vTaskDelete(med);
    vTaskDelete(high);

    //If x equals 1 and y equals zero, the medium task completed before the high task
    //and priority inversion happened
    TEST_ASSERT_EQUAL_INT(1, x);
    TEST_ASSERT_EQUAL_INT(0, y);
}




void test_mutex(void) {
    //initialize test variables
    x = 0;
    y = 0;
    printf("starting test_mutex\n");
    //Create mutex
    xSemaphore = xSemaphoreCreateMutex();
    if (xSemaphore == NULL)
    {
        printf("Failed to create binary semaphore.\n");
        return;
    }

    // Initially set the semaphore to available state.
    xSemaphoreGive(xSemaphore);

    //need these to delete tasks
    TaskHandle_t low, med, high;

    //Start low task and delay to have it take semaphore 
    printf("Stating low priority task\n");
    xTaskCreate(subordinate_task_low, "SubordinateLow",
                SUBORDINATE_TASK_STACK_SIZE, NULL, SUBORDINATE_TASK_LOW_PRIORITY, &low);
    vTaskDelay(1);

    //Start high task right after
    printf("Stating high priority task\n");
    xTaskCreate(subordinate_task_high, "SubordinateHigh",
                SUBORDINATE_TASK_STACK_SIZE, NULL, SUBORDINATE_TASK_HIGH_PRIORITY, &high);
    vTaskDelay(1);

    //Start medium task to have it interrupt low task and start computation
    printf("Stating medium priority task\n");
    xTaskCreate(subordinate_task_medium, "SubordinateMedium",
                SUBORDINATE_TASK_STACK_SIZE, NULL, SUBORDINATE_TASK_MED_PRIORITY, &med);


    //wait 20 seconds for tasks to run
    vTaskDelay(20000);

    //tear down
    vSemaphoreDelete(xSemaphore);
    vTaskDelete(low);
    vTaskDelete(med);
    vTaskDelete(high);

    //If x equals 0 and y equals 1, the high task completed before the medium task
    //and priority inversion did not happen
    TEST_ASSERT_EQUAL_INT(0, x);
    TEST_ASSERT_EQUAL_INT(1, y);

}







void supervisor_task(__unused void *params)
{

    printf("Starting test run.\n");
        UNITY_BEGIN();
        RUN_TEST(test_binary);
        RUN_TEST(test_mutex);
        UNITY_END();
        sleep_ms(5000);



    
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
