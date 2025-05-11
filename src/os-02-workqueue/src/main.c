#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <string.h>

// Define a work item
static struct k_work my_work;

// Simulated non-urgent task
void work_handler(struct k_work *work)
{
    printk("Work handler running in system workqueue.\n");

    // Simulated work (short busy loop)
    for (volatile int i = 0; i < 5000000; i++)
        ;

    printk("Work handler done.\n");
}

// Main thread
void main(void)
{
    printk("Main thread started.\n");

    // Initialize the work item and associate it with the handler
    k_work_init(&my_work, work_handler);

    // Submit work item to the system workqueue
    k_work_submit(&my_work);

    printk("Work item submitted to system workqueue.\n");

    // Sleep to allow workqueue thread to process the item
    k_sleep(K_MSEC(500));
}
