# 08 - BLE Whitelisting

**Author:** Tony Fu  
**Date:** 2025/04/26  
**Device:** nRF52840 DK  
**Toolchain:** nRF Connect SDK v3.0.0  

---

## Bonding with Persistent Storage

To store Bluetooth bond information across device resets (power cycles), we must enable several configuration options in `prj.conf`:

```kconfig
CONFIG_BT_SETTINGS=y
CONFIG_FLASH=y
CONFIG_FLASH_PAGE_LAYOUT=y
CONFIG_FLASH_MAP=y
CONFIG_NVS=y
CONFIG_SETTINGS_NVS=y
CONFIG_SETTINGS=y
```

| Setting | Purpose |
|:--------|:--------|
| **CONFIG_BT_SETTINGS** | Allows the Bluetooth stack to save and restore bonding and identity information (like keys) using the settings subsystem. Without this, bonds are lost on reset. |
| **CONFIG_FLASH** | Provides low-level flash driver support needed to write/read from non-volatile memory. |
| **CONFIG_FLASH_PAGE_LAYOUT** | Abstracts how flash memory is divided into pages, allowing the system to know how to manage writes and erasures correctly. Important for portable flash handling. |
| **CONFIG_FLASH_MAP** | Provides logical partitioning of the flash. Required to map certain parts of flash for NVS (and others like MCUBoot, if used). |
| **CONFIG_NVS** | Enables a simple key-value storage system over flash, suitable for small frequent updates like Bluetooth bonding information. |
| **CONFIG_SETTINGS_NVS** | Makes the settings subsystem use NVS as the storage backend (instead of, for example, FCB or other flash drivers). |
| **CONFIG_SETTINGS** | Provides a general system for saving and loading configuration values. The Bluetooth stack uses it under the hood to store keys and other persistent information. |


Then, in your `main.c` file, you need to include the settings header:

```c
#include <zephyr/settings/settings.h>
```

and call the following functions in your `main()` function to initialize the settings subsystem and load the stored settings:

```c
bt_enable(NULL);
settings_load();
start_advertising();
```


---


## Whitelisting (a.k.a. Filter Accept List)
