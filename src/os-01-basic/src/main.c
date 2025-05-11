#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define STACK_SIZE 512
#define PRIORITY 5

// Declare thread stacks
K_THREAD_STACK_DEFINE(thread1_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread2_stack, STACK_SIZE);

// Declare thread control blocks
struct k_thread thread1_data;
struct k_thread thread2_data;

// Thread entry function prototypes
void thread1_entry(void *p1, void *p2, void *p3);
void thread2_entry(void *p1, void *p2, void *p3);

// Thread pointers to each other for state inspection
struct k_thread *t1_ptr = &thread1_data;
struct k_thread *t2_ptr = &thread2_data;

void thread1_entry(void *p1, void *p2, void *p3)
{
    while (1)
    {
        printk("Thread 1 says hello!\n");
        char buf[32];
        printk("Thread 1 state: %s\n", k_thread_state_str(t1_ptr, buf, sizeof(buf)));
        printk("Thread 2 state: %s\n", k_thread_state_str(t2_ptr, buf, sizeof(buf)));
        k_sleep(K_MSEC(1000));
    }
}

void thread2_entry(void *p1, void *p2, void *p3)
{
    while (1)
    {
        printk("Thread 2 reporting in.\n");
        char buf[32];
        printk("Thread 1 state: %s\n", k_thread_state_str(t1_ptr, buf, sizeof(buf)));
        printk("Thread 2 state: %s\n", k_thread_state_str(t2_ptr, buf, sizeof(buf)));
        k_sleep(K_MSEC(1000));
    }
}

void main(void)
{
    // Create thread 1
    k_thread_create(&thread1_data, thread1_stack,
                    STACK_SIZE, thread1_entry,
                    NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

    // Create thread 2
    k_thread_create(&thread2_data, thread2_stack,
                    STACK_SIZE, thread2_entry,
                    NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

    printk("Main thread done launching threads.\n");
}
