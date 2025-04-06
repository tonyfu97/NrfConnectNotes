#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/addr.h>

LOG_MODULE_REGISTER(addr_test, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

// Advertising data
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

// Connection reference
static struct bt_conn *my_conn = NULL;

void on_connected(struct bt_conn *conn, uint8_t err)
{
	if (err)
	{
		LOG_ERR("Connection failed (err %u)", err);
		return;
	}

	my_conn = bt_conn_ref(conn);
}

void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason 0x%02x)", reason);

	if (my_conn)
	{
		bt_conn_unref(my_conn);
		my_conn = NULL;
	}
}

static struct bt_conn_cb connection_callbacks = {
	.connected = on_connected,
	.disconnected = on_disconnected,
};

void print_local_addresses(void)
{
	bt_addr_le_t addrs[CONFIG_BT_ID_MAX];
	size_t count = CONFIG_BT_ID_MAX;

	bt_id_get(addrs, &count);

	if (count == 0)
	{
		LOG_INF("No identity addresses configured");
		return;
	}

	for (size_t i = 0; i < count; i++)
	{
		char addr_str[BT_ADDR_LE_STR_LEN];
		bt_addr_le_to_str(&addrs[i], addr_str, sizeof(addr_str));
		LOG_INF("Identity address [%zu]: %s", i, addr_str);
	}
}

static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
	(BT_LE_ADV_OPT_CONNECTABLE |
	 BT_LE_ADV_OPT_USE_IDENTITY), /* Connectable advertising and use identity address */
	BT_GAP_ADV_FAST_INT_MIN_1,	  /* 0x30 units, 48 units, 30ms */
	BT_GAP_ADV_FAST_INT_MAX_1,	  /* 0x60 units, 96 units, 60ms */
	NULL);						  /* Set to NULL for undirected advertising */

void main(void)
{
	k_sleep(K_SECONDS(1));

	bt_addr_le_t addr;
	int err = bt_addr_le_from_str("FD:EE:DD:CC:BB:AA", "random", &addr);
	if (err)
	{
		printk("Invalid BT address (err %d)\n", err);
	}

	int id = bt_id_create(&addr, NULL);
	if (id < 0)
	{
		LOG_ERR("bt_id_create() failed: %d", id);
		return;
	}

	err = bt_conn_cb_register(&connection_callbacks);
	if (err)
	{
		LOG_ERR("Failed to register connection callbacks (err %d)", err);
		return;
	}

	err = bt_enable(NULL);
	if (err)
	{
		LOG_ERR("bt_enable() failed: %d", err);
		return;
	}

	print_local_addresses();

	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err)
	{
		LOG_ERR("Advertising failed to start (err %d)", err);
		return;
	}

	LOG_INF("Advertising started (connectable)");

	while (1)
	{
		k_sleep(K_SECONDS(1));
	}
}
