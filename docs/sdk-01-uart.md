# SDK 01: UART

**Author:** Tony Fu  
**Date:** 2025/05/10   
**Device:** nRF52840 DK  
**Toolchain:** nRF Connect SDK v3.0.0  

---

## Comparing UART Access Methods in Zephyr

| Method | How it Works | Pros | Cons |
| --- | --- | --- | --- |
| **Polling**          | Your code checks the UART constantly (read/write one byte at a time).         | Simple to implement                 | Wastes CPU time, blocks other tasks        |
| **Interrupt-driven** | UART generates interrupts on TX/RX; you write ISR handlers to manage buffers. | Non-blocking, better performance    | Requires careful buffer and state handling |
| **Asynchronous**     | Built on top of interrupt-driven, but uses **EasyDMA** and high-level events. | Most efficient, background transfer | More complex API                           |

We will use the **asynchronous** method in this example.

### What's the Difference Between Interrupt-Driven and Asynchronous?

* **Interrupt-driven API** gives you **manual control**: you handle each byte in your own interrupt handlers (ISR). You must manually manage data queues or FIFOs.

* **Asynchronous API** is **higher-level**: it uses interrupts under the hood but gives you events like "RX buffer ready" or "TX done". It also uses **EasyDMA**, which lets the UART peripheral send/receive data **directly to/from memory** without CPU involvement.

---

## In Action

In thie example, we will use asynchronous UART to commucate our nRF52840 DK with a PC. We have the option to send three commands from our PC terminal to the nRF52840 DK:

1. **hello**: The nRF52840 DK will respond with "Hello, world!"
2. **add <num1> <num2>**: The nRF52840 DK will respond with the sum of the two numbers.
3. **reboot**: The nRF52840 DK will reboot.

Typically, we will not use the UART directly. Instead we will use the LOG module or the CONSOLE module if we want to use the UART as a console.

---

## Step 1: Configuration

Edit the `prj.conf` file:

```kconfig
CONFIG_GPIO=y
CONFIG_SERIAL=y
CONFIG_UART_ASYNC_API=y
CONFIG_REBOOT=y
```

The first 3 options are required for the UART driver. The last option is required for the reboot command.

---

## Step 2: Handle UART Events

We shall define a function with the following signature:

```c
#include <zephyr/drivers/uart.h>

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
    switch (evt->type)
    {
    case UART_TX_DONE:
        // Occurs when the entire TX buffer has been sent.
        // Typically used to clear a busy flag or start the next TX.
        break;

    case UART_TX_ABORTED:
        // Occurs if an ongoing TX was canceled.
        // Typically used to handle errors or retry sending.
        break;

    case UART_RX_RDY: // <-- This is the event we will use
        // Called when new data is received in the RX buffer.
        // Typically used to read incoming bytes and process input.
        break;

    case UART_RX_BUF_REQUEST:
        // UART driver asks for a new RX buffer when using double-buffering.
        // Typically used to provide another buffer with uart_rx_buf_rsp().
        break;

    case UART_RX_BUF_RELEASED:
        // Indicates a previously used RX buffer can now be reused or freed.
        // Typically used to manage memory (e.g., recycle buffers).
        break;

    case UART_RX_DISABLED: // <-- This is the event we will use
        // RX has been disabled, either intentionally or after an error.
        // Typically used to re-enable RX with uart_rx_enable().
        break;

    case UART_RX_STOPPED:
        // RX stopped due to a hardware error.
        // Typically used to report the error or reset the interface.
        break;

    default:
        break;
    }
}
```

In our example, we will only use the `UART_RX_RDY` and `UART_RX_DISABLED` events. The `UART_RX_RDY` event is triggered when new data is received in the RX buffer. The `UART_RX_DISABLED` event is triggered at startup when our app calls `uart_rx_enable()` for the first time during `main()`, as Zephyr’s UART driver may internally transition through a disabled state.

Later on in `main()`, we will call `uart_callback_set()` to set the callback function. More on this later.

---

## Step 3: Create a `uart_tx` wrapper

I noticed that if I call the `uart_tx()` function back to back without any delay between them to send the data, the second call will cause the whole program to hang. Therefore, we need to create a wrapper function to defer each call to `uart_tx()`. We will do this with a message queue and a delayed work item. Briefly, we will create a queue to hold the data to be sent, and then create a delayed work item to send the data in the queue. This also allows us to send data in a non-blocking way and add a delay between each call to `uart_tx()`.

Overall, it is a good practice to defer I/O operations in the UART callback function, as it is basically an interrupt handler.

We will start by defining a message queue and a delayed work item. The message queue will hold the data to be sent, and the delayed work item will send the data in the queue.

```c
#define PRINT_MSG_SIZE 128
#define PRINT_QUEUE_SIZE 8

K_MSGQ_DEFINE(print_msgq, PRINT_MSG_SIZE, PRINT_QUEUE_SIZE, 4);
static struct k_work_delayable print_work;

static void print_work_handler(struct k_work *work)
{
    char msg[PRINT_MSG_SIZE];
    while (k_msgq_get(&print_msgq, msg, K_NO_WAIT) == 0)
    {
        uart_tx(uart, (const uint8_t *)msg, strlen(msg), 100);
        k_msleep(2);
    }
}
```

Remember to initialize the work item in `main()`:

```c
    k_work_init_delayable(&print_work, print_work_handler);
```

Then, we can create our own `print()` function to send data to the UART. This function will add the data to the message queue and start the delayed work item.

```c
static void print(const char *str)
{
    if (strlen(str) >= PRINT_MSG_SIZE)
        return;

    char tmp[PRINT_MSG_SIZE];
    strcpy(tmp, str);

    k_msgq_put(&print_msgq, tmp, K_NO_WAIT);
    k_work_schedule(&print_work, K_MSEC(1));
}
```

---


## Step 4: Parse Commands

We need to revisit our `uart_cb()` function to parse the commands. It contains a input parameter `struct uart_event *evt`, which contains the data received from the UART. Here are some important fields in the `struct uart_event`:

- `evt->data.rx.len`: The length of the data received.
- `evt->data.rx.offset`: The offset of the data received.

So to read the data received, we need to iterate through the data from `evt->rx.buf[rx.offset]` to `evt->rx.buf[rx.offset+rx.len]`. In our case, since our receive buffer is 1 byte long, we can just read the first byte. Depending on the command received, we will call the appropriate function to handle the command.


```c
static uint8_t rx_buf[1];

#define CMD_BUF_SIZE 128
static char cmd_buf[CMD_BUF_SIZE];
static size_t cmd_len = 0;

#define PROMPT "> "

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(user_data);

    switch (evt->type)
    {
    case UART_RX_RDY:
        char c = evt->data.rx.buf[evt->data.rx.offset];

        if (c == '\r' || c == '\n') // Enter key pressed
        {
            cmd_buf[cmd_len] = '\0';
            process_command(cmd_buf);
            cmd_len = 0;
            print(PROMPT);
        }
        else if (c == '\b' || c == 127) // Backspace key pressed
        {
            if (cmd_len > 0)
            {
                cmd_len--;
                print("\b \b");
            }
        }
        else if (isprint((unsigned char)c) && cmd_len < CMD_BUF_SIZE - 1) // Printable character
        {
            cmd_buf[cmd_len++] = c;
            char echo[2] = {c, '\0'};
            print(echo);
        }
        break;

    case UART_RX_DISABLED:
        uart_rx_enable(dev, rx_buf, sizeof(rx_buf), 100); // 100 ms timeout
        break;

    default:
        break;
    }
}
```

Note that we also handle the `UART_RX_DISABLED` event to re-enable the RX buffer. This is important because the RX buffer is disabled after the first call to `uart_rx_enable()`. We will also need to define a `rx_buf` variable to hold the data received from the UART.

---

## Step 5: Process Commands

This is just a simple command processor. Not really relavent to the teaching UART, so I will not go into details.

```c
static void process_command(const char *cmd)
{
    if (strncmp(cmd, "hello", 5) == 0)
    {
        print("\r\nHello, world!\r\n");
    }
    else if (strncmp(cmd, "add ", 4) == 0)
    {
        char cmd_copy[CMD_BUF_SIZE];
        strncpy(cmd_copy, cmd, CMD_BUF_SIZE);
        cmd_copy[CMD_BUF_SIZE - 1] = '\0';

        char *arg1 = NULL, *arg2 = NULL, *rest = NULL;
        arg1 = strtok_r(cmd_copy + 4, " ", &rest);
        arg2 = strtok_r(NULL, " ", &rest);

        if (arg1 && arg2)
        {
            int a = strtol(arg1, NULL, 10);
            int b = strtol(arg2, NULL, 10);

            char result[32];
            snprintf(result, sizeof(result), "\r\n%d\r\n", a + b);
            print(result);
        }
        else
        {
            print("Error: usage is add <num1> <num2>\r\n");
        }
    }
    else if (strncmp(cmd, "reboot", 6) == 0)
    {
        k_sleep(K_MSEC(100));
        sys_reboot(SYS_REBOOT_COLD);
    }
    else
    {
        print("Unknown command\r\n");
    }
}
```

---

## Step 6: Main Function

Finally, we need to initialize the UART and start the RX buffer. We will also set the callback function for the UART events.

```c
static const struct device *uart;

int main(void)
{
    k_work_init_delayable(&print_work, print_work_handler);

    uart = DEVICE_DT_GET(DT_NODELABEL(uart0));
    if (!device_is_ready(uart))
        return 0;

    uart_callback_set(uart, uart_cb, NULL);
    uart_rx_enable(uart, rx_buf, sizeof(rx_buf), 100);

    print("UART CLI Ready\r\n");
    print(PROMPT);

    while (1)
    {
        k_sleep(K_FOREVER);
    }
}
```
