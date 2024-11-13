/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

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

int x = 0;
int y = 0;
SemaphoreHandle_t xSemaphore;

// Define the function signature for thread functions
typedef void (*thread_function)(void);

struct task_args
{
    char thread_1_name[50];
    char thread_2_name[50];
    int thread_1_start_delay;
    int thread_2_start_delay;
    int thread_1_priority;
    int thread_2_priority;
    thread_function thread_1_function;
    thread_function thread_2_function;
};

struct task_reply
{
    configRUN_TIME_COUNTER_TYPE thread_1_duration;
    configRUN_TIME_COUNTER_TYPE thread_2_duration;
};

#define SUPERVISOR_TASK_PRIORITY (tskIDLE_PRIORITY + 4UL)
#define SUBORDINATE_TASK_HIGH_PRIORITY (tskIDLE_PRIORITY + 3UL)
#define SUBORDINATE_TASK_MED_PRIORITY (tskIDLE_PRIORITY + 2UL)
#define SUBORDINATE_TASK_LOW_PRIORITY (tskIDLE_PRIORITY + 1UL)
#define SUPERVISOR_TASK_STACK_SIZE configMINIMAL_STACK_SIZE
#define SUBORDINATE_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

#define LOW_PRIORITY (tskIDLE_PRIORITY + 1UL)
#define HIGH_PRIORITY (tskIDLE_PRIORITY + 2UL)

// Low priority task
void subordinate_task_low(__unused void *params)
{
    while (1)
    {
        printf("starting low task while loop\n");
        // Wait for semaphore to become available
        if (xSemaphoreTake(xSemaphore, 10000) == pdTRUE)
        {
            // Take semaphore, then run busy wait loop to simulate a computation on a shared variable, then give back semaphore
            printf("Starting busy wait in low task\n");
            busy_wait_ms(10000);
            printf("Finished busy wait in low task\n");
            xSemaphoreGive(xSemaphore);
        }
    }
}

// Medium priority task
void subordinate_task_medium(__unused void *params)
{
    while (1)
    {
        printf("Starting busy wait in medium task\n");
        // Run busy wait loop to simulate computing something
        busy_wait_ms(5000);
        x = 1;
        printf("Finished busy wait in medium task\n");
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
            printf("Starting busy wait in high task\n");
            busy_wait_ms(10000);
            y = 1;
            printf("Finished busy wait in high task\n");
            xSemaphoreGive(xSemaphore);
        }
    }
}

void test_binary(void)
{
    printf("\nTEST :: Starting test_binary\n");
    // initialize test variables
    x = 0;
    y = 0;
    // Create binary semaphore
    xSemaphore = xSemaphoreCreateBinary();
    if (xSemaphore == NULL)
    {
        printf("Failed to create binary semaphore.\n");
        return;
    }

    // Initially set the semaphore to available state.
    xSemaphoreGive(xSemaphore);

    // need these to delete tasks
    TaskHandle_t low, med, high;

    // Start low task and delay to have it take semaphore
    printf("Stating low priority task\n");
    xTaskCreate(subordinate_task_low, "SubordinateLow",
                SUBORDINATE_TASK_STACK_SIZE, NULL, SUBORDINATE_TASK_LOW_PRIORITY, &low);
    vTaskDelay(10);

    // Start high task right after
    printf("Stating high priority task\n");
    xTaskCreate(subordinate_task_high, "SubordinateHigh",
                SUBORDINATE_TASK_STACK_SIZE, NULL, SUBORDINATE_TASK_HIGH_PRIORITY, &high);
    vTaskDelay(10);

    // Start medium task to have it interrupt low task and start computation
    printf("Stating medium priority task\n");
    xTaskCreate(subordinate_task_medium, "SubordinateMedium",
                SUBORDINATE_TASK_STACK_SIZE, NULL, SUBORDINATE_TASK_MED_PRIORITY, &med);

    // wait 20 seconds for tasks to run
    vTaskDelay(20000);

    // tear down
    vSemaphoreDelete(xSemaphore);
    vTaskDelete(low);
    vTaskDelete(med);
    vTaskDelete(high);

    // If x equals 1 and y equals zero, the medium task completed before the high task
    // and priority inversion happened
    TEST_ASSERT_EQUAL_INT(1, x);
    TEST_ASSERT_EQUAL_INT(0, y);
}

void test_mutex(void)
{
    printf("\nTEST :: Starting test_mutex\n");
    // initialize test variables
    x = 0;
    y = 0;
    // Create mutex
    xSemaphore = xSemaphoreCreateMutex();
    if (xSemaphore == NULL)
    {
        printf("Failed to create binary semaphore.\n");
        return;
    }

    // Initially set the semaphore to available state.
    xSemaphoreGive(xSemaphore);

    // need these to delete tasks
    TaskHandle_t low, med, high;

    // Start low task and delay to have it take semaphore
    printf("Stating low priority task\n");
    xTaskCreate(subordinate_task_low, "SubordinateLow",
                SUBORDINATE_TASK_STACK_SIZE, NULL, SUBORDINATE_TASK_LOW_PRIORITY, &low);
    vTaskDelay(10);

    // Start high task right after
    printf("Stating high priority task\n");
    xTaskCreate(subordinate_task_high, "SubordinateHigh",
                SUBORDINATE_TASK_STACK_SIZE, NULL, SUBORDINATE_TASK_HIGH_PRIORITY, &high);
    vTaskDelay(10);

    // Start medium task to have it interrupt low task and start computation
    printf("Stating medium priority task\n");
    xTaskCreate(subordinate_task_medium, "SubordinateMedium",
                SUBORDINATE_TASK_STACK_SIZE, NULL, SUBORDINATE_TASK_MED_PRIORITY, &med);

    // wait 20 seconds for tasks to run
    vTaskDelay(20000);

    // tear down
    vSemaphoreDelete(xSemaphore);
    vTaskDelete(low);
    vTaskDelete(med);
    vTaskDelete(high);

    // If x equals 0 and y equals 1, the high task completed before the medium task
    // and priority inversion did not happen
    TEST_ASSERT_EQUAL_INT(0, x);
    TEST_ASSERT_EQUAL_INT(1, y);
}

void busy_busy(char *thread_name)
{
    printf("Start busy_busy: %s\n", thread_name);
    for (int i = 0;; i++)
        ;
}

void busy_yield(char *thread_name)
{
    printf("Start busy_yield: %s\n", thread_name);
    for (int i = 0;; i++)
    {
        taskYIELD();
    }
}

struct task_reply run_task(void *params)
{
    TaskHandle_t thread_1, thread_2;
    struct task_reply reply = {0, 0};
    struct task_args *args = (struct task_args *)params;

    vTaskDelay(args->thread_1_start_delay);
    xTaskCreate(args->thread_1_function, "Thread1", SUBORDINATE_TASK_STACK_SIZE, args->thread_1_name, args->thread_1_priority, &thread_1);
    printf("Started %s with %d delay...\n", args->thread_1_name, args->thread_1_start_delay);

    vTaskDelay(args->thread_2_start_delay);
    xTaskCreate(args->thread_2_function, "Thread2", SUBORDINATE_TASK_STACK_SIZE, args->thread_2_name, args->thread_2_priority, &thread_2);
    printf("Started %s with %d delay...\n", args->thread_2_name, args->thread_2_start_delay);

    printf("Collecting runtime...\n");
    vTaskDelay(30 * 1000); // Wait for 30 seconds to collect results.

    reply.thread_1_duration = ulTaskGetRunTimeCounter(thread_1);
    reply.thread_2_duration = ulTaskGetRunTimeCounter(thread_2);
    printf("%s duration: %lld, %s duration: %lld\n", args->thread_1_name, reply.thread_1_duration, args->thread_2_name, reply.thread_2_duration);

    return reply;
}

void test_same_priority_busy_busy(__unused void *params)
{
    printf("\nTEST :: test_same_priority_busy_busy\n");

    struct task_args args;
    strcpy(args.thread_1_name, "High Priority Thread");
    strcpy(args.thread_2_name, "Low Priority Thread");
    args.thread_1_start_delay = 0;
    args.thread_2_start_delay = 0;
    args.thread_1_priority = LOW_PRIORITY;
    args.thread_2_priority = LOW_PRIORITY;
    args.thread_1_function = busy_busy;
    args.thread_2_function = busy_busy;

    struct task_reply reply;
    reply = run_task(&args);

    TEST_ASSERT_INT_WITHIN(2000, reply.thread_1_duration, reply.thread_2_duration);
}

void test_same_priority_yield_yield(__unused void *params)
{
    printf("\nTEST :: test_same_priority_yield_yield\n");

    struct task_args args;
    strcpy(args.thread_1_name, "High Priority Thread");
    strcpy(args.thread_2_name, "Low Priority Thread");
    args.thread_1_start_delay = 0;
    args.thread_2_start_delay = 0;
    args.thread_1_priority = LOW_PRIORITY;
    args.thread_2_priority = LOW_PRIORITY;
    args.thread_1_function = busy_yield;
    args.thread_2_function = busy_yield;

    struct task_reply reply;
    reply = run_task(&args);

    TEST_ASSERT(reply.thread_1_duration < 50000);
    TEST_ASSERT(reply.thread_2_duration < 50000);
}

void test_same_priority_busy_yield(__unused void *params)
{
    printf("\nTEST :: test_same_priority_busy_yield\n");

    struct task_args args;
    strcpy(args.thread_1_name, "High Priority Thread");
    strcpy(args.thread_2_name, "Low Priority Thread");
    args.thread_1_start_delay = 0;
    args.thread_2_start_delay = 0;
    args.thread_1_priority = LOW_PRIORITY;
    args.thread_2_priority = LOW_PRIORITY;
    args.thread_1_function = busy_busy;
    args.thread_2_function = busy_yield;

    struct task_reply reply;
    reply = run_task(&args);

    TEST_ASSERT(reply.thread_1_duration > 50000);
    TEST_ASSERT(reply.thread_2_duration < 50000);
}

void test_diff_priority_busy_busy_high_first(__unused void *params)
{
    printf("\nTEST :: test_diff_priority_busy_busy_high_first\n");

    struct task_args args;
    strcpy(args.thread_1_name, "High Priority Thread");
    strcpy(args.thread_2_name, "Low Priority Thread");
    args.thread_1_start_delay = 0;
    args.thread_2_start_delay = 100;
    args.thread_1_priority = HIGH_PRIORITY;
    args.thread_2_priority = LOW_PRIORITY;
    args.thread_1_function = busy_busy;
    args.thread_2_function = busy_busy;

    struct task_reply reply;
    reply = run_task(&args);

    TEST_ASSERT(reply.thread_1_duration > 100000);
    TEST_ASSERT(reply.thread_2_duration < 1000);
}

void test_diff_priority_busy_busy_low_first(__unused void *params)
{
    printf("\nTEST :: test_diff_priority_busy_busy_low_first\n");

    struct task_args args;
    strcpy(args.thread_1_name, "Low Priority Thread");
    strcpy(args.thread_2_name, "High Priority Thread");
    args.thread_1_start_delay = 0;
    args.thread_2_start_delay = 100;
    args.thread_1_priority = LOW_PRIORITY;
    args.thread_2_priority = HIGH_PRIORITY;
    args.thread_1_function = busy_busy;
    args.thread_2_function = busy_busy;

    struct task_reply reply;
    reply = run_task(&args);

    TEST_ASSERT(reply.thread_1_duration < 100000);
    TEST_ASSERT(reply.thread_2_duration > 1000000);
}

void test_diff_priority_yield_yield_high_first(__unused void *params)
{
    printf("\nTEST :: test_diff_priority_yield_yield_high_first\n");

    struct task_args args;
    strcpy(args.thread_1_name, "High Priority Thread");
    strcpy(args.thread_2_name, "Low Priority Thread");
    args.thread_1_start_delay = 0;
    args.thread_2_start_delay = 100;
    args.thread_1_priority = HIGH_PRIORITY;
    args.thread_2_priority = LOW_PRIORITY;
    args.thread_1_function = busy_yield;
    args.thread_2_function = busy_yield;

    struct task_reply reply;
    reply = run_task(&args);

    TEST_ASSERT(reply.thread_1_duration > 10000);
    TEST_ASSERT(reply.thread_2_duration < 10000);
}

void test_diff_priority_yield_yield_low_first(__unused void *params)
{
    printf("\nTEST :: test_diff_priority_yield_yield_low_first\n");

    struct task_args args;
    strcpy(args.thread_1_name, "Low Priority Thread");
    strcpy(args.thread_2_name, "High Priority Thread");
    args.thread_1_start_delay = 0;
    args.thread_2_start_delay = 100;
    args.thread_1_priority = LOW_PRIORITY;
    args.thread_2_priority = HIGH_PRIORITY;
    args.thread_1_function = busy_yield;
    args.thread_2_function = busy_yield;

    struct task_reply reply;
    reply = run_task(&args);

    TEST_ASSERT(reply.thread_1_duration < 10000);
    TEST_ASSERT(reply.thread_2_duration > 20000);
}

void supervisor_task(__unused void *params)
{

    printf("Running Test...\n");
    UNITY_BEGIN();

    RUN_TEST(test_binary);
    // sleep_ms(500);

    RUN_TEST(test_mutex);
    // sleep_ms(500);

    RUN_TEST(test_same_priority_busy_busy);
    sleep_ms(500);

    RUN_TEST(test_same_priority_yield_yield);
    sleep_ms(500);

    RUN_TEST(test_same_priority_busy_yield);
    sleep_ms(500);

    RUN_TEST(test_diff_priority_busy_busy_high_first);
    sleep_ms(500);

    RUN_TEST(test_diff_priority_busy_busy_low_first);
    sleep_ms(500);

    RUN_TEST(test_diff_priority_yield_yield_high_first);
    sleep_ms(500);

    RUN_TEST(test_diff_priority_yield_yield_low_first);
    sleep_ms(500);

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
