#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define STACK_SIZE 512
#define MUTEX_THREAD_PRIORITY 4

// Protect shared access to a counter
K_MUTEX_DEFINE(counter_mutex);

int shared_counter = 0;
const int counter_limit = 100;

// Shared function to safely update and print the counter
void safe_increment(void)
{
    k_mutex_lock(&counter_mutex, K_FOREVER);

    shared_counter++;
    if (shared_counter > counter_limit)
    {
        shared_counter = 0;
    }

    printk("[Mutex] Thread %p: counter = %d\n", k_current_get(), shared_counter);

    k_mutex_unlock(&counter_mutex);
}

void mutex_worker_thread_1(void)
{
    while (1)
    {
        safe_increment();
        k_sleep(K_MSEC(500));
    }
}

void mutex_worker_thread_2(void)
{
    while (1)
    {
        safe_increment();
        k_sleep(K_MSEC(700));
    }
}

K_THREAD_DEFINE(mutex_thread1_id, STACK_SIZE, mutex_worker_thread_1, NULL, NULL, NULL,
                MUTEX_THREAD_PRIORITY, 0, 0);

K_THREAD_DEFINE(mutex_thread2_id, STACK_SIZE, mutex_worker_thread_2, NULL, NULL, NULL,
                MUTEX_THREAD_PRIORITY, 0, 0);
