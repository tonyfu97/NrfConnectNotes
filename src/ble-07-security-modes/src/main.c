#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include "my_service.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_TEST_SERVICE_VAL),
};

static void on_connected(struct bt_conn *conn, uint8_t err)
{
	if (err)
	{
		LOG_ERR("Connection failed (err %u)\n", err);
		return;
	}

	LOG_INF("Connected\n");
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason %u)\n", reason);
}

static void on_security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char peer_addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), peer_addr, sizeof(peer_addr));

	if (err == 0)
	{
		LOG_INF("Link secured with %s (level %u)", peer_addr, level);
	}
	else
	{
		LOG_WRN("Security setup failed with %s (level %u, err %d)", peer_addr, level, err);
	}
}

struct bt_conn_cb connection_callbacks = {
	.connected = on_connected,
	.disconnected = on_disconnected,
	.security_changed = on_security_changed,
};

static void display_passkey(struct bt_conn *conn, unsigned int passkey)
{
	char peer_addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), peer_addr, sizeof(peer_addr));

	LOG_INF("Enter passkey on %s: %06u", peer_addr, passkey);
}

static void cancel_authentication(struct bt_conn *conn)
{
	char peer_addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), peer_addr, sizeof(peer_addr));

	LOG_INF("Pairing canceled by remote: %s", peer_addr);
}

static const struct bt_conn_auth_cb auth_callbacks = {
	.passkey_display = display_passkey,
	.cancel = cancel_authentication,
};

int main(void)
{
	int err = bt_unpair(BT_ID_DEFAULT, NULL);
	if (err)
	{
		LOG_ERR("Failed to unpair devices (err %d)\n", err);
		return -1;
	}
	LOG_INF("Unpaired all devices");

	err = bt_conn_auth_cb_register(&auth_callbacks);
	if (err)
	{
		LOG_INF("Failed to register authorization callbacks\n");
		return -1;
	}
	LOG_INF("Authorization callbacks registered");

	err = bt_conn_cb_register(&connection_callbacks);
	if (err)
	{
		LOG_ERR("Failed to register connection callbacks (err %d)", err);
		return -1;
	}
	LOG_INF("Connection callbacks registered");

	err = bt_enable(NULL);
	if (err)
	{
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return -1;
	}
	LOG_INF("Bluetooth initialized");

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err)
	{
		LOG_ERR("Advertising failed to start (err %d)", err);
		return -1;
	}

	LOG_INF("Advertising started");

	while (1)
	{
		k_sleep(K_SECONDS(1));
	}

	return 0; // Should never reach here
}
