# 02 - BLE Advertising Simple

**Author:** Tony Fu  
**Date:** 2025/4/3  
**Device:** nRF52840 Dongle  
**Toolchain:** nRF Connect SDK v2.8.0

To advertise your device, you need to call `bt_le_adv_start()` with the right parameters.

```c
int bt_le_adv_start(const struct bt_le_adv_param *param,
		    const struct bt_data *ad, size_t ad_len,
		    const struct bt_data *sd, size_t sd_len);
```

- `param`: Advertising parameters (defines behavior)
- `ad`: Advertising data (broadcasted to all scanners)
- `sd`: Scan response data (optional, sent after scan request)

---

## Common Advertising Params

1. `BT_LE_ADV_CONN` - Connectable undirected advertising, but still advertises to all devices after connected to a peer. This could be bad for power consumption and security.
2. `BT_LE_ADV_CONN_ONE_TIME` - Connectable undirected advertising, but stops after the first connection. This is the **recommended default** for most applications.
3. `BT_LE_ADV_CONN_DIR` - Connectable directed advertising, which is used when you know the peer address. This is faster and less power-hungry than undirected advertising.
4. `BT_LE_ADV_NCONN` - Non-connectable undirected advertising, which is used when you don't want to be connected to. This is useful for beacons and other passive broadcasts.
5. `BT_LE_ADV_CONN_DIR_LOW_DUTY` - Connectable directed advertising with a lower duty cycle, which is used when you don't expect to be connected to immediately but still want to reconnect with a known peer.
6. `BT_LE_ADV_NCONN_IDENTITY` - Non-connectable advertising that uses the device's identity address, which is useful for testing and visibility.


**Note:** To prevent tracking, advertising modes like `BT_LE_ADV_CONN`, `BT_LE_ADV_CONN_ONE_TIME`, and `BT_LE_ADV_NCONN` (params 1, 2, and 4) use **Resolvable Private Addresses** (RPAs) by default. RPAs are a privacy feature introduced in Bluetooth 4.2. They are generated using the device’s identity address (public or static random) and a random value, and they typically change every 15 minutes.

This allows trusted, bonded peers to resolve your identity, while third parties cannot track the device over time. RPA behavior is enabled by default when `CONFIG_BT_PRIVACY=y`, but it can be disabled if needed. To explicitly advertise with your identity address (e.g., for testing or directed advertising), use the `BT_LE_ADV_OPT_USE_IDENTITY` option.


---


## Advertising Data and Scan Response Data

### Advertising Data (ad[])
```c
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};
```

This example shows how to define **advertising data** using `struct bt_data`.

```c
struct bt_data {
	uint8_t type;
	uint8_t data_len;
	const uint8_t *data;
};

However, we typically don't define `struct bt_data` directly. Instead, we use macros to simplify the process.

```c
BT_DATA(type, data, data_len)        // Use with a pointer and explicit size
BT_DATA_BYTES(type, byte1, byte2...) // Inline declaration with raw bytes
```

In advertising data, we must always include the following fields:

```c
BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR))
```

This is **always used on Nordic devices** because Nordic only supports BLE (not BR/EDR), and this is why we include `<zephyr/bluetooth/gap.h>` for the bitmasks.

and

```c
BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN)
```

### Scan Response Data (sd[])

Use scan response (sd) when you want to include optional, larger data like a full device name or a URL. Advertising data is always broadcast; scan response is only sent when a scanner asks for it, making it more power-efficient and less crowded.

```c
#define COMPANY_ID_CODE 0x0059 // Nordic Semiconductor
typedef struct
{
	uint16_t company_id; // Company ID
	uint8_t data[6];	 // Custom Data
} my_data_t;

static const my_data_t my_data = {
	.company_id = COMPANY_ID_CODE,
	.data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
```

When the advertising data is too large to fit in advertising packets, the scanner can request a scan response. This is not shown here.

---

## Common Advertising Data Types

### Common in `ad[]` (Advertising)

| Macro                  | Description                      |
|------------------------|----------------------------------|
| `BT_DATA_FLAGS`        | BLE flags (required)             |
| `BT_DATA_NAME_COMPLETE`| Complete device name             |
| `BT_DATA_UUID16_ALL`   | List of 16-bit service UUIDs     |
| `BT_DATA_UUID128_ALL`  | List of 128-bit service UUIDs    |
| `BT_DATA_MANUFACTURER_DATA` | Vendor-specific binary data |

### Common in `sd[]` (Scan Response)

| Macro                 | Description                       |
|-----------------------|-----------------------------------|
| `BT_DATA_URI`         | URI string (e.g., website)        |
| `BT_DATA_TX_POWER`    | Transmission power (in dBm)       |
| `BT_DATA_APPEARANCE`  | Device appearance (e.g., watch)   |
| `BT_DATA_NAME_SHORTENED` | Shortened name (if full name won't fit) |

More on service UUIDs later.