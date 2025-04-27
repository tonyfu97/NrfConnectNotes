# 03 - BLE Advertising: Connectable

**Author:** Tony Fu  
**Date:** 2025/4/5  
**Device:** nRF52840 Dongle  
**Toolchain:** nRF Connect SDK v2.8.0

To make a device connectable, we need to enable the Bluetooth peripheral role and set the advertising parameters accordingly.

Add this to your `prj.conf` file:

```Kconfig
CONFIG_BT_PERIPHERAL=y
```

In previous examples, we used the predefined macro `BT_LE_ADV_NCONN` for non-connectable advertising. To allow connections, we need different parameters like `BT_LE_ADV_CONN_ONE_TIME`. We can also define our own advertising parameters with the help of the `BT_LE_ADV_PARAM()` macro:

```c
BT_LE_ADV_PARAM(_options, _int_min, _int_max, _peer)
```

- `_options`: A bitmask of advertising options (e.g., connectable, use identity, etc.)
- `_int_min`, `_int_max`: Advertising interval (units of 0.625 ms)
- `_peer`: Peer address; set to `NULL` for undirected advertising

Example parameters for connectable advertising:

```c
static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
	(BT_LE_ADV_OPT_CONNECTABLE |
	 BT_LE_ADV_OPT_USE_IDENTITY), /* Connectable advertising and use identity address */
	BT_GAP_ADV_FAST_INT_MIN_2, /* Min Advertising Interval 100 ms */
	BT_GAP_ADV_FAST_INT_MAX_2, /* Max Advertising Interval 150 ms */
	NULL); /* Set to NULL for undirected advertising */
```

Common `options` flags include:

- `BT_LE_ADV_OPT_CONNECTABLE`: Allow devices to connect
- `BT_LE_ADV_OPT_ONE_TIME`: Stop advertising after one connection
- `BT_LE_ADV_OPT_USE_IDENTITY`: Use the device's identity address (not RPA)
- `BT_LE_ADV_OPT_USE_NAME`: Automatically include the GAP device name
- `BT_LE_ADV_OPT_SCANNABLE`: Enable scan response data
- `BT_LE_ADV_OPT_EXT_ADV`: Use extended advertising features (for longer range or larger data)
- `BT_LE_ADV_OPT_FILTER_SCAN_REQ`: Only respond to scanners in the filter list
- `BT_LE_ADV_OPT_FILTER_CONN`: Only allow connections from filtered peers

This setup allows your device to be discoverable and accept connections from central devices like phones or PCs. However, it will continue to advertise even after a connection is established, which may not be ideal for power consumption or security. To stop advertising after the first connection, add the `BT_LE_ADV_OPT_ONE_TIME` option.

---

## Advertising Intervals

The **advertising interval** controls how often your device broadcasts its advertising packets. It is set using `_int_min` and `_int_max` in `BT_LE_ADV_PARAM()` and is specified in units of 0.625 ms.

- **Valid range**: 20 ms to 10.24 seconds
- **Resolution**: 0.625 ms steps

You can set:
- `min == max` → Fixed interval (allowed)
- `min < max` → Interval chosen randomly within that range (typical)

A small random delay (0–10 ms) is added automatically to help avoid collisions between devices using similar intervals.

### Guidelines

- **Short intervals (~20–100 ms)**: Fast discovery, higher power consumption
- **Medium intervals (~150–500 ms)**: Balanced power and discovery time (good default)
- **Long intervals (>1 s)**: Low power, slower to be discovered

Choose an interval based on your application:
- For **wearables** or **low-power sensors**, favor longer intervals
- For **pairing mode** or **quick reconnection**, use shorter intervals

Some constants are defined in `zephyr/bluetooth/gap.h` for common intervals:

#### GAP Advertising Parameters

These are predefined intervals for **legacy advertisement** (connectable or scannable), used by many macros like `BT_LE_ADV_CONN`.

| Macro                        | Hex     | Time (ms) |
|-----------------------------|---------|-----------|
| BT_GAP_ADV_FAST_INT_MIN_1   | 0x0030  | 30        |
| BT_GAP_ADV_FAST_INT_MAX_1   | 0x0060  | 60        |
| BT_GAP_ADV_FAST_INT_MIN_2   | 0x00a0  | 100       |
| BT_GAP_ADV_FAST_INT_MAX_2   | 0x00f0  | 150       |
| BT_GAP_ADV_SLOW_INT_MIN     | 0x0640  | 1000      |
| BT_GAP_ADV_SLOW_INT_MAX     | 0x0780  | 1200      |

#### GAP Periodic Advertising Parameters

**Periodic advertising** is a Bluetooth Low Energy feature introduced in Bluetooth 5 that allows a device to broadcast data at fixed, predictable intervals without requiring a connection. Unlike legacy advertising, which is sent on the primary channels and may be missed if the scanner isn't listening at the right time, periodic advertising uses a **synchronizable schedule** and is transmitted on secondary channels. This allows scanners to **synchronize** with the advertiser and receive updates reliably with lower power consumption. It's ideal for broadcasting **sensor data**, **location beacons**, or **status updates** where frequent connections aren't needed but consistent updates are.

| Macro                           | Hex     | Time (ms) |
|--------------------------------|---------|-----------|
| BT_GAP_PER_ADV_FAST_INT_MIN_1  | 0x0018  | 30        |
| BT_GAP_PER_ADV_FAST_INT_MAX_1  | 0x0030  | 60        |
| BT_GAP_PER_ADV_FAST_INT_MIN_2  | 0x0050  | 100       |
| BT_GAP_PER_ADV_FAST_INT_MAX_2  | 0x0078  | 150       |
| BT_GAP_PER_ADV_SLOW_INT_MIN    | 0x0320  | 1000      |
| BT_GAP_PER_ADV_SLOW_INT_MAX    | 0x03C0  | 1200      |


#### GAP Scan Parameters

These define standard scan intervals and windows. The **interval** is how often scanning starts, and the **window** is how long each scan lasts.

| Macro                          | Hex     | Time (ms) |
|-------------------------------|---------|-----------|
| BT_GAP_SCAN_FAST_INTERVAL_MIN | 0x0030  | 30        |
| BT_GAP_SCAN_FAST_INTERVAL     | 0x0060  | 60        |
| BT_GAP_SCAN_FAST_WINDOW       | 0x0030  | 30        |
| BT_GAP_SCAN_SLOW_INTERVAL_1   | 0x0800  | 1280      |
| BT_GAP_SCAN_SLOW_WINDOW_1     | 0x0012  | 11.25     |
| BT_GAP_SCAN_SLOW_INTERVAL_2   | 0x1000  | 2560      |
| BT_GAP_SCAN_SLOW_WINDOW_2     | 0x0012  | 11.25     |

#### GAP Initial Connection Parameters

These define intervals used when **initiating a connection** (i.e., when a central connects to a peripheral).

| Macro                       | Hex     | Time (ms) |
|----------------------------|---------|-----------|
| BT_GAP_INIT_CONN_INT_MIN   | 0x0018  | 30        |
| BT_GAP_INIT_CONN_INT_MAX   | 0x0028  | 50        |

---

## Additional Notes on Timing Coordination

When both the **advertiser** and **scanner** are under your control, you can optimize their timing parameters to strike a balance between **discovery speed**, **power consumption**, and **reliability**. This is especially useful in scenarios like proprietary ecosystems, closed systems, or when designing both ends of a BLE link (e.g., a wearable + smartphone app).

### Understanding Scan Interval and Scan Window

- **Scan interval** is how often the scanner initiates a scan cycle.
- **Scan window** is how long the scanner listens during each scan interval.
- Both range from **2.5 ms to 10.24 seconds** in 0.625 ms steps.
- In each scan interval, the scanner will scan one of the three primary advertising channels. After the interval ends, it switches to the next channel.
  
If the scan window is **equal to** the scan interval, the scanner is scanning **continuously** (100% duty cycle). If the window is **shorter than** the interval, it means the scanner is off part of the time — which saves power.

> For example, if scan interval = 100 ms and window = 20 ms, the scanner is listening only 20% of the time.

### Coordinating With Advertising Intervals

To improve the chances of the scanner receiving advertising packets:

- The **advertising interval** should be **shorter than or equal to** the scan interval. This increases the probability that an advertisement will occur **during** the scan window.
- A good rule of thumb is:  
  **advertising interval ≤ scan interval - scan window**

This ensures that at least one advertisement falls within an active scan window over multiple cycles.

You can also align timing using multiples, such as:
- Scan interval = 3 × advertising interval
- Scan window = 50–80% of scan interval (for reasonable duty cycle)

This is not a strict rule, but it helps **statistically reduce missed advertisements** due to phase misalignment.

### Example Timing Coordination

| Parameter           | Typical Value      |
|--------------------|--------------------|
| Advertising interval | 100 ms (`0x00A0`)  |
| Scan interval        | 300 ms (`0x018C`)  |
| Scan window          | 150 ms (`0x00F0`)  |

This setup provides good discovery speed and balances scanner energy usage.

### Notes on Scan Response Timing

- **Scan response packets** are only sent if the advertiser receives a **scan request**.
- This scan request must be sent **while the advertiser is listening**, and **only during active scanning.**
- The **non-scanning time (between windows)** is **not used for scan response** — in fact, **nothing happens** during those gaps.
- This means **larger scan windows** increase the chance of triggering a scan response successfully.

### Power Considerations

- Scanning uses **more power** than advertising — so the scanner (usually the central) should be the device with more available energy (e.g., a phone).
- Peripheral devices (e.g., sensors, wearables) can advertise less frequently to save power and rely on **burst scanning** from the central.

### Summary Guidelines

- Ensure `advertising interval ≤ scan interval`
- Keep `scan window` large enough to catch at least one advertisement
- Use scan window ≈ 50–100% of scan interval for faster discovery
- For low power devices, increase advertising interval and let central scan more aggressively


---


## Aside: BLE Address Types

Before we continue, let's take a moment to explore the different types of Bluetooth LE addresses. Every BLE device is identified by a **48-bit address**, and they are grouped into two main categories: **public** and **random**. Random addresses are further split into **static**, **resolvable private**, and **non-resolvable private** types. The type used affects how your device is identified, whether it can be tracked, and whether connections or bonding are possible.

### 1. Public Address

- A globally unique address assigned by the manufacturer and stored in the device's hardware (e.g., in FICR on nRF chips).
- Used automatically unless you override it with a random address or enable privacy.

#### Get the default public address:
```c
bt_addr_le_t addr;
bt_id_get(&addr, NULL);
```

### 2. Random Static Address

- Does not change across boots — stable identity.
- Must follow BLE spec: the most significant two bits of the first byte must be `11` (so the first byte must be between `0xC0` and `0xFF`).
- Commonly used when you don't have a public address block but need a consistent identity.

#### Set a static random address manually:
```c
#include <zephyr/bluetooth/addr.h>

bt_addr_le_t addr;
bt_addr_le_from_str("FF:00:11:22:33:44", "random", &addr);
addr.a.val[5] |= 0xC0; // Make sure MSBs are '11' to mark as static random

int id = bt_id_create(&addr, NULL); // Must be called before bt_enable()
```

> In production, you might derive a unique static random address from the device’s public address or store a pre-generated one in flash/UICR.

### 3. Resolvable Private Address (RPA)

- Changes periodically (by default every 15 minutes), but can be **resolved by bonded peers** using an Identity Resolving Key (IRK).
- Enables **privacy** without sacrificing the ability to reconnect or bond.

I am currently not sure how to set the RPA manually, but it should be automatically generated by the stack when you enable privacy (`CONFIG_BT_PRIVACY=y`).

### 4. Non-Resolvable Private Address (NRPA)

- Also changes periodically, but **cannot be resolved** — not bondable or connectable.
- Useful for anonymous broadcasting (e.g., a privacy-focused beacon).
- A new NRPA is generated every time you start advertising.

#### Set NRPA explicitly:
```c
static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
    BT_LE_ADV_OPT_USE_NRPA,
    160, 160, NULL);
```


---


## Connection Callbacks

To handle Bluetooth connections in Zephyr, we use the `bt_conn_cb` structure. This lets us register callback functions to track connection events like when a device connects or disconnects.

First, include the necessary header:

```c
#include <zephyr/bluetooth/conn.h>
```

Then, declare a connection reference to track the active connection:

```c
static struct bt_conn *my_conn = NULL;
```

### Basic Callbacks

```c
void on_connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection failed (err %u)", err);
		return;
	}

	my_conn = bt_conn_ref(conn);
}

void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason 0x%02x)", reason);

	if (my_conn) {
		bt_conn_unref(my_conn);
		my_conn = NULL;
	}
}
```

- `bt_conn_ref()` increases the reference count so the connection stays valid.
- `bt_conn_unref()` is called on disconnection to release the reference.
- `bt_conn_get_dst()` gets the peer device’s address.

Although tracking `bt_conn` isn't strictly necessary in this example, it becomes important when supporting **multiple simultaneous connections**. Keeping a reference to each connection allows you to target specific peers—for example, sending data only to one device or applying per-connection security settings. Most Zephyr Bluetooth APIs accept `NULL` as the `bt_conn` argument, which simply means “send to all connected peers.” But for more advanced use cases, managing and using specific `bt_conn` pointers is essential for precise control.

### Registering Callbacks

```c
static struct bt_conn_cb connection_callbacks = {
	.connected = on_connected,
	.disconnected = on_disconnected,
};

bt_conn_cb_register(&connection_callbacks);
```

You must register your callback structure before or after enabling Bluetooth, and **before** expecting any connection events.


### Other Available Callbacks in `bt_conn_cb`

You can optionally implement more callbacks:

- `recycled`: Called when a connection object is returned to the pool.
- `le_param_req`: Called when the peer requests to update connection parameters. Return `true` to accept or `false` to reject.
- `le_param_updated`: Notifies when connection parameters are updated (interval, latency, timeout).
- `identity_resolved`: (If SMP enabled) Notifies when a peer's identity address is resolved from an RPA.
- `security_changed`: (If SMP or Classic enabled) Called when connection security changes.
- `remote_info_available`: Called when info about the peer (features, roles) is available.
- `le_phy_updated`: Notifies when PHY is changed (1M, 2M, coded).
- `le_data_len_updated`: Called when the maximum payload size changes.
- `tx_power_report`: Reports transmit power changes.
- `subrate_changed`: Called when connection subrate settings change.

These are mostly optional and only needed for advanced use cases.


---


### Connection and Disconnection Error Codes

Both `on_connected()` and `on_disconnected()` callbacks report errors using **HCI error codes** defined by the Bluetooth Core Specification. These are standard codes used across the Bluetooth stack to indicate why a connection failed or was terminated.

- In `on_connected()`, the `err` parameter is `0` on success, or an HCI error (e.g., `0x3E` for "Connection Failed to be Established").
- In `on_disconnected()`, the `reason` parameter is also an HCI error code (e.g., `0x13` for "Remote User Terminated Connection").

These codes help identify common issues such as timeouts, user-initiated disconnects, or parameter mismatches.

You can refer to the official list of controller error codes here:  
🔗 [Bluetooth Core Spec v5.4 – Controller Error Codes](https://www.bluetooth.com/wp-content/uploads/Files/Specification/HTML/Core-54/out/en/architecture,-mixing,-and-conventions/controller-error-codes.html)
