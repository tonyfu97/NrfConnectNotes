#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>

LOG_MODULE_REGISTER(simple_adv, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

// Advertising data
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

// Scan response data
#define COMPANY_ID_CODE 0x0059 // Nordic Semiconductor
typedef struct
{
	uint16_t company_id; // Company ID
	uint8_t data[6];	 // Custom Data
} my_data_t;

static const my_data_t my_data = {
	.company_id = COMPANY_ID_CODE,
	.data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, (const uint8_t *)&my_data, sizeof(my_data))};

void main(void)
{
	int err;

	LOG_INF("Starting example with scan response");

	err = bt_enable(NULL);
	if (err)
	{
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}

	LOG_INF("Bluetooth initialized");

	err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err)
	{
		LOG_ERR("Advertising failed to start (err %d)", err);
		return;
	}

	LOG_INF("Advertising with scan response started");

	// Idle loop
	while (1)
	{
		k_sleep(K_SECONDS(1));
	}
}
