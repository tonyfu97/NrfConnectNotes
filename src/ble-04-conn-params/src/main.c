#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>

LOG_MODULE_REGISTER(conn_params, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static const struct bt_data adv_payload[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static struct bt_conn *active_conn = NULL;

/* Connection Parameter Update Callback */
static void handle_conn_param_change(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout)
{
	double interval_ms = interval * 1.25;
	uint16_t timeout_ms = timeout * 10;
	LOG_INF("Params changed: %.2f ms, latency %u, timeout %u ms", interval_ms, latency, timeout_ms);
}

/* PHY Request */
static void request_phy_update(struct bt_conn *conn)
{
	const struct bt_conn_le_phy_param phy_pref = {
		.options = BT_CONN_LE_PHY_OPT_NONE,
		.pref_rx_phy = BT_GAP_LE_PHY_2M,
		.pref_tx_phy = BT_GAP_LE_PHY_2M,
	};

	int err = bt_conn_le_phy_update(conn, &phy_pref);
	if (err) {
		LOG_ERR("PHY update failed (%d)", err);
	}
}

/* PHY Change Notification */
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

/* Data Length Request */
static void request_data_len_update(struct bt_conn *conn)
{
	struct bt_conn_le_data_len_param len_params = {
		.tx_max_len = BT_GAP_DATA_LEN_MAX,
		.tx_max_time = BT_GAP_DATA_TIME_MAX,
	};

	int err = bt_conn_le_data_len_update(conn, &len_params);
	if (err) {
		LOG_ERR("Failed to update data length (%d)", err);
	}
}

/* Data Length Change Notification */
static void handle_data_len_change(struct bt_conn *conn, struct bt_conn_le_data_len_info *info)
{
	LOG_INF("Data len: TX=%u (%uus), RX=%u (%uus)",
		info->tx_max_len, info->tx_max_time,
		info->rx_max_len, info->rx_max_time);
}

/* MTU Exchange Callback */
static void mtu_exchange_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
	if (!err) {
		uint16_t app_mtu = bt_gatt_get_mtu(conn) - 3;
		LOG_INF("MTU negotiated: %u bytes", app_mtu);
	} else {
		LOG_ERR("MTU exchange failed (ATT err %u)", err);
	}
}

/* MTU Exchange Trigger */
static void trigger_mtu_exchange(struct bt_conn *conn)
{
	static struct bt_gatt_exchange_params params = {
		.func = mtu_exchange_cb
	};

	int err = bt_gatt_exchange_mtu(conn, &params);
	if (err) {
		LOG_ERR("MTU exchange failed (%d)", err);
	}
}

/* Connection Event */
static void on_conn_established(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Failed to connect (err %u)", err);
		return;
	}

	active_conn = bt_conn_ref(conn);
	LOG_INF("Device connected");

	struct bt_conn_info info;
	if (bt_conn_get_info(conn, &info) == 0) {
		double int_ms = info.le.interval * 1.25;
		uint16_t timeout_ms = info.le.timeout * 10;
		LOG_INF("Initial conn params: %.2f ms, latency %u, timeout %u ms", int_ms, info.le.latency, timeout_ms);
	}

	request_phy_update(conn);
	request_data_len_update(conn);
	trigger_mtu_exchange(conn);
}

/* Disconnection Event */
static void on_conn_terminated(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason: 0x%02x)", reason);

	if (active_conn) {
		bt_conn_unref(active_conn);
		active_conn = NULL;
	}
}

/* Register callbacks */
static struct bt_conn_cb conn_callbacks = {
	.connected = on_conn_established,
	.disconnected = on_conn_terminated,
	.le_param_updated = handle_conn_param_change,
	.le_phy_updated = handle_phy_change,
	.le_data_len_updated = handle_data_len_change,
};

void main(void)
{
	if (bt_conn_cb_register(&conn_callbacks)) {
		LOG_ERR("Failed to register BLE callbacks");
		return;
	}

	if (bt_enable(NULL)) {
		LOG_ERR("Bluetooth init failed");
		return;
	}

	int err = bt_le_adv_start(BT_LE_ADV_CONN_ONE_TIME, adv_payload,
				  ARRAY_SIZE(adv_payload), NULL, 0);
	if (err) {
		LOG_ERR("Adv start failed (%d)", err);
		return;
	}

	LOG_INF("Advertising (connectable) started");

	while (1) {
		k_sleep(K_SECONDS(1));
	}
}
