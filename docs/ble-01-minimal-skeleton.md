# 01 - Minimal BLE Skeleton

**Author:** Tony Fu  
**Date:** 2025/4/3  
**Device:** nRF52840 Dongle
**Toolchain:** nRF Connect SDK v2.8.0

This example sets up the **Bluetooth stack only** — no advertising, no services, no connections. Useful as a clean starting point.

## Configuration: `prj.conf`

```conf
# Enable basic logging
CONFIG_LOG=y

# Enable Bluetooth stack
CONFIG_BT=y

# Optional: Set device name (not used here but required for CONFIG_BT)
CONFIG_BT_DEVICE_NAME="Minimal_BLE"

# Increase stack sizes for stability
CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=2048
CONFIG_MAIN_STACK_SIZE=2048
```

## Code Structure: `main.c`

```c
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>

LOG_MODULE_REGISTER(minimal_ble, LOG_LEVEL_INF);

int main(void)
{
    int err;

    LOG_INF("Minimal BLE Example Start");

    // Initialize the Bluetooth stack
    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return -1;
    }

    LOG_INF("Bluetooth initialized");

    // Idle loop
    while (1) {
        k_sleep(K_SECONDS(1));
    }
}
```
