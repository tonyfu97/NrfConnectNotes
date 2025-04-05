# 04 - BLE Connection Parameters

## Core Connection Parameters  
These parameters were part of the **original Bluetooth LE specification**, and are exchanged during connection establishment. They define the timing and reliability of the connection.

### 1. Connection Interval
- Time between consecutive connection events (when devices wake up to communicate).
- **Typical range:** 7.5 ms to 4 s, in steps of 1.25 ms.
- Lower values = lower latency, higher power use.  
- Higher values = longer sleep time, lower power use.

### 2. Supervision Timeout
- Max time allowed without successful packet reception before the connection is considered lost.
- **Typical range:** 100 ms to 32 s, in steps of 10 ms.
- Must be > (1 + peripheral latency) × connection interval × 2.

### 3. Peripheral Latency
- Number of connection events the peripheral can skip if it has nothing to send.
- **Typical values:** 0–499 (unitless).
- Allows the peripheral to save power while remaining in the connection.
- **Note:** The name is misleading — this is **not a time duration**, but a **count** of skipped events.

### In Action
In the on_connect callback, we can print the 3 connection parameters:
```c
static void on_conn_established(struct bt_conn *conn, uint8_t err)
{
	// ... other code
	active_conn = bt_conn_ref(conn);

	struct bt_conn_info info;
	if (bt_conn_get_info(conn, &info) == 0) {
		double int_ms = info.le.interval * 1.25;
		uint16_t timeout_ms = info.le.timeout * 10;
		LOG_INF("Initial conn params: %.2f ms, latency %u, timeout %u ms", int_ms, info.le.latency, timeout_ms);
	}
    // ... other code
}
```

> **Note:** Floating point math is not enabled by default, so we need to add `CONFIG_FPU=y` to the `prj.conf` file.

Those parameters are typically first determined by the central device, and then the peripheral can request changes. To define our (i.e., the peripheral's) connection parameters, we can override the default values in the `prj.conf` file:

```Kconfig
# Set preferred connection parameters (units: 1.25ms for interval, 10ms for timeout)
CONFIG_BT_PERIPHERAL_PREF_MIN_INT=320
CONFIG_BT_PERIPHERAL_PREF_MAX_INT=400
CONFIG_BT_PERIPHERAL_PREF_LATENCY=3
CONFIG_BT_PERIPHERAL_PREF_TIMEOUT=500
CONFIG_BT_GAP_AUTO_UPDATE_CONN_PARAMS=y
```

The last line may be redundant, as it is enabled by default. It allows the peripheral to request a change in connection parameters after the initial connection. To get notified of the new parameters, we can implement the `on_conn_param_update` callback:

```c
static void handle_conn_param_change(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout)
{
	double interval_ms = interval * 1.25;
	uint16_t timeout_ms = timeout * 10;
	LOG_INF("Params changed: %.2f ms, latency %u, timeout %u ms", interval_ms, latency, timeout_ms);
}
```

---

## PHY Radio Modes  
- The default mode is **1M PHY** (1 Mbps), which is used for compatibility.
- **2M PHY** (2 Mbps): Doubles throughput, reduces time on air, but may shorten range.
- **Coded PHY**: Increases range using redundancy, but at a lower data rate (125 kbps or 500 kbps).


### In Action

We can set the PHY mode again in the on_connect callback:

```c
static void on_conn_established(struct bt_conn *conn, uint8_t err)
{
    // ... other code
    const struct bt_conn_le_phy_param phy_pref = {
		.options = BT_CONN_LE_PHY_OPT_NONE,
		.pref_rx_phy = BT_GAP_LE_PHY_2M,
		.pref_tx_phy = BT_GAP_LE_PHY_2M,
	};

	int err = bt_conn_le_phy_update(conn, &phy_pref);
	if (err) {
		LOG_ERR("PHY update failed (%d)", err);
	}
    // ... other code
}
```

To get notified of the PHY change, we can implement the `on_phy_update` callback:

```c
static void handle_phy_change(struct bt_conn *conn, struct bt_conn_le_phy_info *info)
{
	switch (info->tx_phy) {
	case BT_CONN_LE_TX_POWER_PHY_1M:
		LOG_INF("PHY switched to 1M");
		break;
	case BT_CONN_LE_TX_POWER_PHY_2M:
		LOG_INF("PHY switched to 2M");
		break;
	case BT_CONN_LE_TX_POWER_PHY_CODED_S8:
		LOG_INF("PHY switched to Long Range");
		break;
	default:
		LOG_INF("PHY changed to unknown mode");
		break;
	}
}
```

---

## Data Length vs MTU

To understand how these differ, it's helpful to recall the **Bluetooth LE stack layers**:
- **GATT** (Generic Attribute Profile) sits at the top — this is where most application developers work.
- Beneath that, **L2CAP** segments and reassembles data.
- At the base, **the Link Layer** is responsible for actual over-the-air packets.

### 4. MTU (Maximum Transmission Unit)
- Max number of bytes in a single **GATT operation** (e.g., read/write).
- **Default:** 23 bytes.
- Can be increased via **MTU exchange** after connection.
- Operates at the **GATT/L2CAP level**.

### 5. Data Length
- Max number of bytes that can be sent in a single **Link Layer PDU** (packet).
- **Default:** 27 bytes. Can be increased up to **251 bytes** (BLE 4.2+).
- Negotiated via **Data Length Extension (DLE)**.
- Even with a large MTU, if the data length is low, data is split into multiple packets.

### In Action

Again, we can set the data length and MTU in the on_connect callback:

```c
static void mtu_exchange_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
	if (!err) {
		uint16_t app_mtu = bt_gatt_get_mtu(conn) - 3;
		LOG_INF("MTU negotiated: %u bytes", app_mtu);
	} else {
		LOG_ERR("MTU exchange failed (ATT err %u)", err);
	}
}

static void on_conn_established(struct bt_conn *conn, uint8_t err)
{
    // ... other code
    // Update data length
    struct bt_conn_le_data_len_param len_params = {
		.tx_max_len = BT_GAP_DATA_LEN_MAX,
		.tx_max_time = BT_GAP_DATA_TIME_MAX,
	};

	int err = bt_conn_le_data_len_update(conn, &len_params);
	if (err) {
		LOG_ERR("Failed to update data length (%d)", err);
	}

    // Update MTU
    static struct bt_gatt_exchange_params params = {
		.func = mtu_exchange_cb
	};

	int err = bt_gatt_exchange_mtu(conn, &params);
	if (err) {
		LOG_ERR("MTU exchange failed (%d)", err);
	}
    // ... other code
}
```

And we can get notified of the data length change with the `le_data_len_updated` callback:

```c
static void handle_data_len_change(struct bt_conn *conn, struct bt_conn_le_data_len_info *info)
{
	LOG_INF("Data len: TX=%u (%uus), RX=%u (%uus)",
		info->tx_max_len, info->tx_max_time,
		info->rx_max_len, info->rx_max_time);
}
```
