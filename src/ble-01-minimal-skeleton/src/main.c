#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>

LOG_MODULE_REGISTER(minimal_ble, LOG_LEVEL_INF);

int main(void)
{
	int err;

	LOG_INF("Minimal BLE Example Start");

	// Initialize the Bluetooth stack
	err = bt_enable(NULL);
	if (err)
	{
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return -1;
	}

	LOG_INF("Bluetooth initialized");

	// Idle loop
	while (1)
	{
		k_sleep(K_SECONDS(1));
	}
}
