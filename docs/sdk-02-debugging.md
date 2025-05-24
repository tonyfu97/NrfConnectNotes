# SDK 02: Debugging Tips

**Author:** Tony Fu  
**Date:** 2025/05/24  
**Device:** nRF52840 DK  
**Toolchain:** nRF Connect SDK v3.0.0  

---

I recommend watching [this video](https://youtu.be/IDC2m91xMb0?si=U4wxxo3WKrAcScap) for a quick overview of debugging in the SDK.

---

## In Action

We are doing to use part of the code from the [OS-03: Semaphores and Mutexes](./os-03-semaphore-mutex.md) example. This code has two threads that increment a counter with mutex protection. The code is as follows.


### Step 1: Enable Debugging

We need to enable debugging in the `prj.conf` file. Add the following lines to enable debugging features:

```kconfig
CONFIG_DEBUG_THREAD_INFO=y
CONFIG_DEBUG_OPTIMIZATIONS=y
```

Then, when setting up the build configuration in VSCode, make sure to select `Optimize for debugging (-Og)` as the optimization level. This will ensure that the code is compiled with debugging information.

### Step 2: Set Breakpoints

In VSCode, you can set breakpoints by clicking on the left margin next to the line numbers in your source code. For example, set a breakpoint at the line where the counter is incremented:

```c
// Shared function to safely update and print the counter
void safe_increment(void)
{
    k_mutex_lock(&counter_mutex, K_FOREVER);

    shared_counter++; // <- set breakpoint here
    if (shared_counter > counter_limit)
    {
        shared_counter = 0;
    }

    printk("[Mutex] Thread %p: counter = %d\n", k_current_get(), shared_counter);

    k_mutex_unlock(&counter_mutex);
}
```

There are also conditional breakpoints available in VSCode, but I could not get it to work properly.

### Step 3: Add Variables to Watch

You can add variables to the watch list in VSCode to monitor their values during debugging. Right-click on the variable you want to watch (e.g., `shared_counter`) and select "Add to Watch". This will allow you to see the value of `shared_counter` as you step through the code.

### Step 4: Start Debugging

To start debugging, click on the "Run and Debug" icon in the left sidebar of VSCode. Then click on the "Continue" button to start the debugging session. The program will run until it hits the breakpoint you set earlier. You can check the value of `shared_counter` in the watch list and step through the code using the debugging controls.

### Step 5: Play with the Thread Viewer

You can also use the Thread Viewer in VSCode to see the state of all threads. Click on the "NRF DEBUG" tab next to the terminal tab. This will show you a list of all threads, their states, and the current thread's stack trace. You can click on a thread to see its details and stack trace.

### Step 6: Manually Toggle LEDs

The nRF52840 DK has four LEDs that you can manually toggle the GPIO pins to turn on/off the LEDs.

1. Go to "Peripherals" in the left sidebar of VSCode.
2. Click on "GPIO" to open the GPIO view.
3. On the nRF52840 DK, the four green LEDs are connected to GPIO P0 pins 13, 14, 15, and 16. Click on the "DIR" menu to set the direction of the GPIO pins to "Output".
4. The LEDs are active low, so to turn on an LED, you need to set the GPIO pin to low (0). To turn off an LED, set the GPIO pin to high (1). Usually, they are already set to low by default, so the previous step should have already turned on the LEDs.
