# OS 02: Workqueue

**Author:** Tony Fu  
**Date:** 2025/05/11   
**Device:** nRF52840 DK  
**Toolchain:** nRF Connect SDK v3.0.0  

---

## Types of Threads

In Zephyr RTOS, threads are the primary units of execution. There are three main categories you’ll encounter: **system threads**, **user-defined threads**, and **workqueue threads**. Each serves a distinct purpose in managing how work is scheduled and executed.

---

### 1. System Threads

System threads are created automatically by Zephyr during system startup. Two are always present:

* **Main thread**: After basic kernel setup, this thread runs the application’s `main()` function. If no `main()` is defined, the thread simply exits once initialization is complete, and the system continues running.

* **Idle thread**: When no other threads are ready to run, the idle thread takes over. It typically enters a low-power state to conserve energy, especially on platforms like Nordic’s nRF series.

These threads ensure the RTOS is always operational, even with no application-specific threads running.

---

### 2. User-Defined Threads

We as developers can define additional threads to structure our application logic using `k_thread_create()`. Each thread runs independently and is assigned a priority to control scheduling.

---

### 3. Workqueue Threads

A **workqueue thread** processes lightweight tasks called **work items**. A work item is simply a function submitted to a queue, typically from an interrupt or high-priority context. The workqueue thread executes these functions in the order they were received (FIFO).

Zephyr includes a **system workqueue** by default. You can submit work items to it without creating a custom workqueue. The thread backing the system workqueue is itself a system thread. In this example, we will try using the system workqueue to process a simple task.

---

## In Action

This example demonstrates how to use the **default system workqueue** in Zephyr to offload a non-urgent task. The work item runs in a separate thread managed by the kernel, which helps prevent blocking the main thread or higher-priority contexts.

---

### 1. Declare a Work Item

```c
static struct k_work my_work;
```

This defines a **work item structure**. It's a lightweight object used to represent a unit of deferred work. The system workqueue will call a user-defined function (`work_handler`) when this work item is processed.

---

### 2. Define the Work Handler

```c
void work_handler(struct k_work *work)
{
    printk("Work handler running in system workqueue.\n");

    for (volatile int i = 0; i < 5000000; i++);  // Simulated delay

    printk("Work handler done.\n");
}
```

This function will be executed by the **system workqueue thread**, not the main thread. It simulates a time-consuming task using a busy loop.

---

### 3. Initialize and Submit the Work Item

Now we initialize a work item and associates it with a handler function:

```c
void main(void)
{
    printk("Main thread started.\n");

    k_work_init(&my_work, work_handler);
```

and later submit it to the system workqueue:

```c
    k_work_submit(&my_work);
```
