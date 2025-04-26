#include <dk_buttons_and_leds.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/settings/settings.h>
#include "my_service.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define BOND_ERASE_BUTTON_MASK DK_BTN1_MSK
#define PAIRING_MODE_BUTTON_MASK DK_BTN2_MSK

// ##################### Advertising ########################

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_TEST_SERVICE_VAL),
};

#define BT_LE_ADV_CONN_ACCEPT_LIST                                                                  \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_FILTER_CONN | BT_LE_ADV_OPT_FILTER_SCAN_REQ, \
					BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL)

static struct k_work adv_work;
static void start_advertising(void)
{
	LOG_INF("Starting advertising\n");
	k_work_submit(&adv_work);
}

// ##################### Connection Callbacks ########################

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

static void on_recycled(void)
{
	printk("Recycled callback called\n");
	start_advertising();
}

struct bt_conn_cb connection_callbacks = {
	.connected = on_connected,
	.disconnected = on_disconnected,
	.security_changed = on_security_changed,
	.recycled = on_recycled,
};

// ##################### Bonding Callbacks ########################

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

// ##################### Button Handling ########################

static bool pairing_mode_enabled = false;

static void handle_button_event(uint32_t current_state, uint32_t changed_mask)
{
	if (changed_mask & BOND_ERASE_BUTTON_MASK)
	{
		bool bond_erase_requested = !(current_state & BOND_ERASE_BUTTON_MASK);
		if (bond_erase_requested)
		{
			int ret = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
			if (ret)
			{
				LOG_INF("Failed to erase bonds (err: %d)", ret);
			}
			else
			{
				LOG_INF("Bond information erased");
			}
		}
	}

	if (changed_mask & PAIRING_MODE_BUTTON_MASK)
	{
		bool pairing_requested = !(current_state & PAIRING_MODE_BUTTON_MASK);
		if (pairing_requested)
		{
			pairing_mode_enabled = true;
			int ret = bt_le_adv_stop();
			if (ret)
			{
				LOG_INF("Failed to stop advertising (err: %d)", ret);
			}
			// Advertising restart will be triggered after recycle callback
			LOG_INF("Pairing mode enabled, advertising stopped");
		}
	}
}

// ##################### Whitelist Setup and Advertising ########################

static void add_bonded_device_to_whitelist(const struct bt_bond_info *bond, void *context)
{
	int *added_count = context;

	if (*added_count < 0)
	{
		return;
	}

	int ret = bt_le_filter_accept_list_add(&bond->addr);
	if (ret)
	{
		LOG_INF("Failed to add device to whitelist (err: %d)", ret);
		*added_count = -EIO;
	}
	else
	{
		(*added_count)++;
		LOG_INF("Device added to whitelist: %02X %02X", bond->addr.a.val[0], bond->addr.a.val[1]);
	}
}

static int configure_whitelist(uint8_t id)
{
	int ret = bt_le_filter_accept_list_clear();
	if (ret)
	{
		LOG_INF("Whitelist clear failed (err: %d)", ret);
		return ret;
	}

	int bonded_devices = 0;
	bt_foreach_bond(id, add_bonded_device_to_whitelist, &bonded_devices);

	return bonded_devices;
}

static void advertisement_handler(struct k_work *work_item)
{
	int ret;

	if (pairing_mode_enabled)
	{
		ret = bt_le_filter_accept_list_clear();
		if (ret)
		{
			LOG_INF("Whitelist clear failed (err: %d)", ret);
		}
		else
		{
			LOG_INF("Whitelist cleared for pairing");
		}

		pairing_mode_enabled = false;

		ret = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
		if (ret)
		{
			LOG_INF("Advertising start failed (err: %d)", ret);
		}
		else
		{
			LOG_INF("Advertising started for pairing");
		}
		return;
	}

	int whitelist_count = configure_whitelist(BT_ID_DEFAULT);
	if (whitelist_count < 0)
	{
		LOG_INF("Whitelist configuration failed (err: %d)", whitelist_count);
		return;
	}

	if (whitelist_count == 0)
	{
		LOG_INF("No bonded devices found, advertising openly");
		ret = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	}
	else
	{
		LOG_INF("Advertising with whitelist, bonded devices: %d", whitelist_count);
		ret = bt_le_adv_start(BT_LE_ADV_CONN_ACCEPT_LIST, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	}

	if (ret)
	{
		LOG_INF("Advertising start failed (err: %d)", ret);
	}
	else
	{
		LOG_INF("Advertising started successfully");
	}
}

// ##################### Main Function ########################

int main(void)
{
	k_work_init(&adv_work, advertisement_handler);

	int err;

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

	settings_load();
	start_advertising();

	err = dk_buttons_init(handle_button_event);
	if (err)
	{
		LOG_ERR("Failed to init buttons (err %d)", err);
		return -1;
	}
	LOG_INF("Buttons initialized");

	while (1)
	{
		k_sleep(K_SECONDS(1));
	}

	return 0; // Should never reach here
}
