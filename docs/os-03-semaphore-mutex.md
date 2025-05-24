# OS 03: Semaphores and Mutexes

**Author:** Tony Fu  
**Date:** 2025/05/11   
**Device:** nRF52840 DK  
**Toolchain:** nRF Connect SDK v3.0.0  

Every RTOS has its own implementation of semaphores and mutexes. While the core concepts are similar, the details can vary, so pay close attention to the specifics described here:

---

## Semaphores

* Used to control access to a shared resource by limiting how many threads can access it simultaneously. It can also be used for event signaling between threads.
* Semaphores typically start with a non-zero initial count, indicating how many threads can acquire the semaphore without blocking.
* Semaphores are not "owned" by any thread; ownership does not apply, and therefore **priority inheritance** is **not supported**.
* A semaphore is **given** (i.e., its count is incremented) using `k_sem_give()`. This operation is **safe to call from ISRs**, but it has no effect if the count is already at its configured maximum.
* A semaphore is **taken** (i.e., its count is decremented) using `k_sem_take()`. If the count is zero, the calling thread blocks until the semaphore is available (unless a timeout is specified). **Blocking take is not allowed from ISRs**.

---

## Mutexes

* Used to ensure mutual exclusion in critical sections, allowing only one thread at a time to access a protected resource.
* A **mutex is owned** by the thread that successfully locks it. Only the owning thread can unlock it.
* **Priority inheritance** is supported: if a lower-priority thread holds a mutex and a higher-priority thread attempts to acquire it, the lower-priority thread temporarily inherits the higher priority. This mechanism reduces the risk of **priority inversion**, where a low-priority thread blocks high-priority ones by holding a shared resource.
* A mutex has two states: **locked** and **unlocked**. It is always initially unlocked.
* Zephyr’s mutexes support **recursive locking**, meaning the owning thread can lock the mutex multiple times. The mutex is only released when it is unlocked the same number of times it was locked.
* **Mutexes cannot be used in ISRs**, because mutex logic depends on thread scheduling and priority handling, which are not meaningful in interrupt context.

---

## Synchronization vs. Data Sharing

Semaphores and mutexes are used primarily for synchronization — not data passing, which typically uses queues. We will cover different types of queues in a later note.

---

## In Action: Semaphore

In this example, we define two semaphore threads: one continuously waits for a signal (`sem_receiver_thread`), and the other periodically sends that signal (`sem_sender_thread`).

### Step 1: Project Configuration

Although this config is usually enabled by default, for clarity, we explicitly enable multithreading support in `prj.conf`:

```kconfig
CONFIG_MULTITHREADING=y
```

### Step 2: Semaphore Definition

In `src/main.c`, we define a semaphore with an initial count of 0, meaning it starts in a state where no threads can take it until it is given. We have also set the maximum count to 1, which is a common practice for semaphores used for signaling.

```c
K_SEM_DEFINE(sync_sem, 0, 1);
```

### Step 3: Semaphore Threads
We define two threads: `sem_receiver_thread` and `sem_sender_thread`. The receiver thread waits for the semaphore to be given, while the sender thread periodically gives the semaphore.

```c
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
```

### Results

When you run the program, you should see the receiver thread waiting for a signal, and every two seconds, the sender thread sends a signal. The receiver thread then prints that it received the signal and sleeps for one second before waiting for the next signal.

```
[Semaphore] Sender: sending signal...
[Semaphore] Receiver: got the signal!
```

---

## In Action: Mutex

In this example, we create two mutex threads (`mutex_worker_thread_1 and _2`) safely increment a shared counter using a mutex (`counter_mutex`) to avoid race conditions. The counter resets after reaching a limit of 100, and each thread prints its activity.

### Step 1: Project Configuration

Just like with semaphores, we ensure multithreading support is enabled in `prj.conf`.

### Step 2: Mutex Definition

In `src/main.c`, we define a mutex for protecting access to the shared counter:

```c
K_MUTEX_DEFINE(counter_mutex);
```

### Step 3: Create a Shared Function

We will create a function `safe_increment()` that safely increments the shared counter using the mutex. This function will be called by both threads.

```c
int shared_counter = 0;
const int counter_limit = 100;

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
```

### Step 4: Mutex Threads

We define two threads that will call `safe_increment()` to increment the shared counter.

```c
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
```

### Results

When you run the program, both threads will increment the shared counter safely using the mutex. The output will show the counter value being incremented by each thread, demonstrating that the mutex prevents the counter from being updated simultaneously by both threads:

```
[Mutex] Thread 0x20000660: counter = 92
[Mutex] Thread 0x200006e0: counter = 93
[Mutex] Thread 0x20000660: counter = 94
[Mutex] Thread 0x200006e0: counter = 95
[Mutex] Thread 0x20000660: counter = 96
[Mutex] Thread 0x200006e0: counter = 97
[Mutex] Thread 0x200006e0: counter = 98
[Mutex] Thread 0x20000660: counter = 99
[Mutex] Thread 0x200006e0: counter = 100
[Mutex] Thread 0x20000660: counter = 0
[Mutex] Thread 0x200006e0: counter = 1
[Mutex] Thread 0x20000660: counter = 2
[Mutex] Thread 0x200006e0: counter = 3
```