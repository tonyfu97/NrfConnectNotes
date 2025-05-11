# OS 01: Basics

**Author:** Tony Fu  
**Date:** 2025/05/11   
**Device:** nRF52840 DK  
**Toolchain:** nRF Connect SDK v3.0.0  

---

## **Zephyr Thread States**

In Zephyr RTOS, each thread's status is represented internally as a **bitmask** field (`thread_state`) within the thread structure. This allows a thread to simultaneously hold multiple state flags, enabling nuanced control and scheduling decisions. The states are decoded by functions like `k_thread_state_str()`, which interpret and format these bit flags into readable strings.

Below is a summary of the known internal thread states:

* **DUMMY**: The thread structure is not associated with an actual thread. Often used for initialization or placeholders.
* **PENDING**: The thread is waiting for a resource (e.g., semaphore, mutex, queue). It is not ready to run.
* **PRESTART**: The thread has been created but not yet started (i.e., `k_thread_start()` hasn’t been called).
* **DEAD**: The thread has terminated and is no longer active.
* **SUSPENDED**: The thread has been explicitly suspended and cannot run until resumed.
* **ABORTING**: The thread is in the process of being aborted (usually via `k_thread_abort()`).
* **SUSPENDING**: The thread is transitioning into the suspended state.
* **QUEUED**: The thread is queued in an object (e.g., waiting in a FIFO or work queue).

These states are **not mutually exclusive**. A thread can, and often does, hold more than one state flag at the same time. The system uses these combinations to precisely reflect what is happening with the thread at any given moment.

### **Example**:

A thread waiting on a queue and also marked for suspension could have the state:

```
PENDING + SUSPENDING
```

This indicates that the thread is waiting for data and, simultaneously, in the process of being suspended.

### **Simplified View (as used in Nordic's nRF Connect SDK course)**

To make the model more digestible, Nordic classifies threads into just three broad states:

| **Course State** | **Meaning**                                         | **Internal Representation**                                         |
| ---------------- | --------------------------------------------------- | ------------------------------------------------------------------- |
| **Running**      | Actively executing on the CPU.                      | Not stored in `thread_state`, but implied by the scheduler.         |
| **Runnable**     | Ready to run, only waiting for CPU time.            | Internally: `thread_state == 0` (no flags set)                      |
| **Non-runnable** | Blocked, suspended, or otherwise ineligible to run. | Any combination of internal flags like `PENDING`, `SUSPENDED`, etc. |

This simplification omits internal transitions and overlaps for the sake of clarity, especially when teaching fundamentals. However, understanding the bitmask-based system helps when debugging or implementing lower-level scheduling logic.

---

## **Thread Priority in Zephyr**

In Zephyr, each thread is assigned an integer priority. Lower numbers indicate higher priority. For example, a thread with priority `2` will run before one with priority `5`.

Zephyr distinguishes between two types of threads based on priority:

* **Preemptible threads**: These have **non-negative priorities** (`0` and up). They can be interrupted by other threads of **equal or higher** priority.
* **Cooperative threads**: These use **negative priorities** and **cannot be preempted**. Once running, they keep the CPU until they yield or block.

The default configuration allows:

* **15 preemptible levels**, from `0` (high) to `15` (low, used by the idle thread).
* **16 cooperative levels**, from `-1` down to `-16`.

Preemptible threads are the norm; cooperative threads are used rarely and typically not covered in beginner material.

---

## **Rescheduling Points in Zephyr**

Zephyr is a **tickless, event-driven RTOS**, meaning the scheduler runs only when needed—not on a fixed timer. A **rescheduling point** is any moment when the scheduler is invoked to reconsider which thread should run next. This happens whenever the **state of the Ready queue changes**.

### **Common rescheduling triggers:**

* A thread calls `k_yield()`: its state changes from `Running` to `Ready`.
* A thread becomes unready (e.g., when calling `k_sleep()`): its state changes from `Running` to `Non-runnable`.
* A blocked thread is unblocked by a sync object (e.g., semaphore, mutex): its state becomes `Ready`.
* A thread waiting for data receives input: its state transitions to `Ready`.
* **Time slicing** is enabled and the current thread exceeds its allowed slice: it's moved from `Running` to `Ready`.

These transitions prompt the scheduler to re-evaluate which thread to run based on priority and readiness.

---

## **Time Slicing in Zephyr**

**Time slicing** allows threads of the **same (non-negative)** priority to share CPU time in a **round-robin** fashion.

### **How it works:**

* You configure a **time slice duration** using `CONFIG_TIMESLICE_SIZE`.
* If enabled, each **preemptible** thread (priority ≥ 0) gets up to that amount of time before being moved back to the Ready queue.
* This lets other threads of **equal priority** run in turn.
* This prevents a single thread from monopolizing the CPU, especially in when it provides no explicit rescheduling points.

### **Key points:**

* Only applies to **preemptible threads** (non-negative priority).
* **Cooperative threads** (negative priority) are **not affected** by time slicing—they must yield explicitly.
* Time slicing does **not override** priority. Higher-priority threads always preempt lower-priority ones, regardless of time slices.

---

## **Creating Threads in Zephyr**

To work with threads in Zephyr, include the core kernel API header:

```c
#include <zephyr/kernel.h>
```

This gives access to thread management functions, synchronization primitives, and timing utilities.

### **Why the `k_` Prefix?**

Zephyr uses the `k_` prefix to mark functions, types, and macros that are part of the **kernel API**. This naming convention makes it clear that you're working with internal kernel mechanisms, helping distinguish them from application-level symbols.

---

## **Basic Thread Creation Example**

Here’s an example that defines two threads. Each prints a message and displays the current state of both threads. Notice that even though the threads are created in runtime, their stack space is allocated at compile time.

```c
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define STACK_SIZE 512
#define PRIORITY 5

K_THREAD_STACK_DEFINE(thread1_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(thread2_stack, STACK_SIZE);

struct k_thread thread1_data;
struct k_thread thread2_data;

void thread1_entry(void *p1, void *p2, void *p3);
void thread2_entry(void *p1, void *p2, void *p3);

struct k_thread *t1_ptr = &thread1_data;
struct k_thread *t2_ptr = &thread2_data;
```

Each thread runs a loop, prints a message, and inspects thread states:

```c
void thread1_entry(void *p1, void *p2, void *p3)
{
    while (1) {
        printk("Thread 1 says hello!\n");
        char buf[32];
        printk("Thread 1 state: %s\n", k_thread_state_str(t1_ptr, buf, sizeof(buf)));
        printk("Thread 2 state: %s\n", k_thread_state_str(t2_ptr, buf, sizeof(buf)));
        k_sleep(K_MSEC(1000));
    }
}

void thread2_entry(void *p1, void *p2, void *p3)
{
    while (1) {
        printk("Thread 2 reporting in.\n");
        char buf[32];
        printk("Thread 1 state: %s\n", k_thread_state_str(t1_ptr, buf, sizeof(buf)));
        printk("Thread 2 state: %s\n", k_thread_state_str(t2_ptr, buf, sizeof(buf)));
        k_sleep(K_MSEC(1000));
    }
}
```

Threads are created in the `main()` function using `k_thread_create()`:

```c
void main(void)
{
    k_thread_create(&thread1_data, thread1_stack,
                    STACK_SIZE, thread1_entry,
                    NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&thread2_data, thread2_stack,
                    STACK_SIZE, thread2_entry,
                    NULL, NULL, NULL,
                    PRIORITY, 0, K_NO_WAIT);

    printk("Main thread done launching threads.\n");
}
```

---

### **Explanation of `k_thread_create()`**

```c
k_tid_t k_thread_create(
    struct k_thread *new_thread,
    k_thread_stack_t *stack,
    size_t stack_size,
    k_thread_entry_t entry,
    void *p1, void *p2, void *p3,
    int prio,
    uint32_t options,
    k_timeout_t delay);
```

* `new_thread`: Pointer to a `k_thread` struct used for tracking the thread.
* `stack`: Pointer to stack memory defined with `K_THREAD_STACK_DEFINE()`.
* `stack_size`: Must match the size used when defining the stack.
* `entry`: Function to run when the thread starts.
* `p1–p3`: Optional arguments passed to the thread entry function.
* `prio`: Thread priority. Lower values mean higher priority.
* `options`: Architecture-specific flags; use `0` for basic threads.
* `delay`: Use `K_NO_WAIT` to start immediately, or a timeout for delayed start.

---


## Results

When you run the code, you should see output similar to this:

```
Thread 1 state: sleeping
Thread 2 state: queued
Thread 1 says hello!
Thread 1 state: queued
Thread 2 state: sleeping
Thread 2 reporting in.
Thread 1 state: sleeping
Thread 2 state: queued
Thread 1 says hello!
Thread 1 state: queued
Thread 2 state: sleeping
Thread 2 reporting in.
```

This output shows the state of each thread as they run. The states will change based on the actions taken by each thread and the scheduler's decisions.
