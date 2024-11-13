## Activty 2

Write tests for two threads running the following scenarios. Try to predict the runtime for each thread.
1. Threads with same priority:
    1. Both run `busy_busy`.
    1. Both run `busy_yield`
    1. One run `busy_busy` one run `busy_yield`
1. Threads with different priority.
    1. Both run `busy_busy`.
        1. Higher priority starts first.
        1. Lower priority starts first.
    1. Both run `busy_yield`.
        1. Higher priority starts first.
        1. Lower priority starts first.

### Prediction

1. Threads with same priority:
    1. Scheduler will give equal opportunity for both threads. There will be preemption and scheduler should allocate equal run time.
    1. Scheduler will give equal opportunity for both threads. There will be preemption and scheduler should allocate equal run time. But the threads always yield to each other and there will no forward progress.
    1. Scheduler will give equal opportunity for both threads. There will be preemption but run time will be higher for `busy_busy` thread. Because when scheduler gives opportunity for `busy_yield` thread to run it always yields and gives up its time to `busy_busy` thread.

1. Threads with different priority.
    1. Both run `busy_busy`.
        1. Scheduler will give higher opportunity for higher thread. Since higher priority runs `busy_busy` thread it will always consume the CPU cycles and never yields. Therefore, the lower priority thread will never get a chance.
        1. Scheduler will give higher opportunity for higher thread. Since lower priority thread starts first and it is the only thread scheduler provides low priority CPU time until higher thread is started. Once the higher thread starts it will keep consuming the run time and is never preempted.

    1. Both run `busy_yield`.
        1. Scheduler will give higher opportunity for higher thread. If there is higher or equal thread then `taskYIELD()` will context and give opportunity to other thead. Since higher priority starts fist and there is no other task priority equal or higher. Therefore, it will never yield and takes up the whole run time.

        1. Scheduler will give higher opportunity for higher thread. Since the lower priority thread starts first it will run initially. At `taskYIELD()` it will context switch if higher thread is started because there is higher priority thread available. Thereafter, higher priority thread will never `taskYIELD()` from higher priority thread because there is no higher or equal task to context switch from.