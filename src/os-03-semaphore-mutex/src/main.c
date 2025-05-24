#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define STACK_SIZE 512
#define SEM_THREAD_PRIORITY 5
#define MUTEX_THREAD_PRIORITY 4

/* ========================= */
/* Semaphore Example Section */
/* ========================= */

// Define a binary semaphore (count = 0, max = 1)
K_SEM_DEFINE(sync_sem, 0, 1);

// Thread A: waits for a signal
void sem_receiver_thread(void)
{
    while (1)
    {
        printk("[Semaphore] Receiver: waiting for signal...\n");
        k_sem_take(&sync_sem, K_FOREVER);
        printk("[Semaphore] Receiver: got the signal!\n");
        k_sleep(K_MSEC(1000));
    }
}

// Thread B: sends a signal
void sem_sender_thread(void)
{
    while (1)
    {
        k_sleep(K_MSEC(2000));
        printk("[Semaphore] Sender: sending signal...\n");
        k_sem_give(&sync_sem);
    }
}

K_THREAD_DEFINE(sem_recv_id, STACK_SIZE, sem_receiver_thread, NULL, NULL, NULL,
                SEM_THREAD_PRIORITY, 0, 0);

K_THREAD_DEFINE(sem_send_id, STACK_SIZE, sem_sender_thread, NULL, NULL, NULL,
                SEM_THREAD_PRIORITY, 0, 0);

/* ====================== */
/* Mutex Example Section  */
/* ====================== */

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
