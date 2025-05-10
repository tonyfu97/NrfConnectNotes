#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/reboot.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define CMD_BUF_SIZE 128
#define PRINT_MSG_SIZE 128
#define PRINT_QUEUE_SIZE 8
#define PROMPT "> "

static const struct device *uart;
static uint8_t rx_buf[1];
static char cmd_buf[CMD_BUF_SIZE];
static size_t cmd_len = 0;

K_MSGQ_DEFINE(print_msgq, PRINT_MSG_SIZE, PRINT_QUEUE_SIZE, 4);
static struct k_work_delayable print_work;
static void process_command(const char *cmd);

// -----------------------------------------------------------------------------
// Print system
// -----------------------------------------------------------------------------

static void print_work_handler(struct k_work *work)
{
    char msg[PRINT_MSG_SIZE];
    while (k_msgq_get(&print_msgq, msg, K_NO_WAIT) == 0)
    {
        uart_tx(uart, (const uint8_t *)msg, strlen(msg), 100);
        k_msleep(2);
    }
}

static void print(const char *str)
{
    if (strlen(str) >= PRINT_MSG_SIZE)
        return;

    char tmp[PRINT_MSG_SIZE];
    strcpy(tmp, str);

    k_msgq_put(&print_msgq, tmp, K_NO_WAIT);
    k_work_schedule(&print_work, K_MSEC(1));
}

// -----------------------------------------------------------------------------
// UART callback
// -----------------------------------------------------------------------------

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(user_data);

    switch (evt->type)
    {
    case UART_RX_RDY:
        for (size_t i = 0; i < 1; i++)
        {
            char c = evt->data.rx.buf[evt->data.rx.offset + i];

            if (c == '\r' || c == '\n')
            {
                cmd_buf[cmd_len] = '\0';
                process_command(cmd_buf);
                cmd_len = 0;
                print(PROMPT);
            }
            else if (c == '\b' || c == 127)
            {
                if (cmd_len > 0)
                {
                    cmd_len--;
                    print("\b \b");
                }
            }
            else if (isprint((unsigned char)c) && cmd_len < CMD_BUF_SIZE - 1)
            {
                cmd_buf[cmd_len++] = c;
                char echo[2] = {c, '\0'};
                print(echo);
            }
        }
        break;

    case UART_RX_DISABLED:
        uart_rx_enable(dev, rx_buf, sizeof(rx_buf), 100);
        break;

    default:
        break;
    }
}

// -----------------------------------------------------------------------------
// Command processing
// -----------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

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
