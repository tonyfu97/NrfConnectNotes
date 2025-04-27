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

Whitelisting restricts which devices can connect to your Bluetooth advertiser.  
It is based on a list of trusted (bonded) devices, called the Filter Accept List (FAL).  
When a device is bonded, its address is added to the FAL.  
When a device tries to connect, the Bluetooth stack checks if it is in the FAL.

To enable whitelisting, add these configurations in your `prj.conf`:

```kconfig
CONFIG_BT_FILTER_ACCEPT_LIST=y
CONFIG_BT_PRIVACY=y
CONFIG_BT_MAX_PAIRED=3
```

| Config | Purpose |
|:-------|:--------|
| **CONFIG_BT_FILTER_ACCEPT_LIST** | Enables the use of an internal list of allowed devices for scanning and connection filtering. |
| **CONFIG_BT_PRIVACY** | Enables the use of Resolvable Private Addresses (RPAs) to protect user identity and avoid static MAC addresses. This helps when using whitelisting, because bonded devices can still recognize each other even if their addresses change. |
| **CONFIG_BT_MAX_PAIRED** | Limits how many bonded devices can be stored and managed by the Bluetooth stack. |


### Advertising with Whitelisting

To actually use the FAL, you must configure your advertising parameters to filter both:
- **Scan Requests** (devices trying to get more info like UUIDs)
- **Connection Requests** (devices trying to establish a link)

You do this by adding two advertising options:

- **BT_LE_ADV_OPT_FILTER_SCAN_REQ**: only allow devices in the whitelist to send scan requests.
- **BT_LE_ADV_OPT_FILTER_CONN**: only allow devices in the whitelist to initiate connections.

You can define your whitelist-enabled advertising like this:

```c
#define BT_LE_ADV_CONN_ACCEPT_LIST \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_FILTER_CONN | BT_LE_ADV_OPT_FILTER_SCAN_REQ, \
					BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL)
```

This ensures that only known, bonded devices can see your scan response and initiate a connection.

### Adding Devices to the Whitelist

Before using the Filter Accept List (whitelist) during advertising, you must populate it with the addresses of bonded devices.  
This process is typically done after Bluetooth is initialized but before starting advertising.

```c
static void add_bonded_device_to_whitelist(const struct bt_bond_info *bond, void *context)
{
    int *added_count = context;

    if (*added_count < 0) {
        return;
    }

    int ret = bt_le_filter_accept_list_add(&bond->addr);
    if (ret) {
        LOG_INF("Failed to add device to whitelist (err: %d)", ret);
        *added_count = -EIO;
    } else {
        (*added_count)++;
        LOG_INF("Device added to whitelist: %02X %02X", bond->addr.a.val[0], bond->addr.a.val[1]);
    }
}
```

We will use this callback function below to add bonded devices to the whitelist:

```c
static int configure_whitelist(uint8_t id)
{
    int ret = bt_le_filter_accept_list_clear();
    if (ret) {
        LOG_INF("Whitelist clear failed (err: %d)", ret);
        return ret;
    }

    int bonded_devices = 0;
    bt_foreach_bond(id, add_bonded_device_to_whitelist, &bonded_devices);

    return bonded_devices;
}
```

This function:
1. Clears any existing whitelist.
2. Iterates over all bonded devices (`bt_foreach_bond()`).
3. Adds each bonded device to the whitelist.
4. Returns the number of devices added.

Now we just need to call `configure_whitelist()` before starting advertising:


---


## Erasing Bonding Information

Sometimes you need to remove all stored bonds, for example when:
- Resetting the device for new users.
- Testing new pairing procedures.

You can erase all bonding information with a simple API call:

```c
int ret = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
```

- `BT_ID_DEFAULT` specifies the local identity (usually 0 unless you use multiple identities).
- `BT_ADDR_LE_ANY` means "remove all bonded devices" (wildcard address).

If successful, all paired devices are forgotten, and the whitelist (if any) must be rebuilt.

After erasing bonds, you should also clear the FAL to ensure no stale entries remain. Also, you should go to the Bluetooth settings of your central device (e.g., phone) and remove the bond from there as well.