#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "my_service.h"

LOG_MODULE_REGISTER(my_service, LOG_LEVEL_INF);

static uint8_t stored_value = 0;
static struct my_service_cb service_cb;

static ssize_t on_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                       void *buf, uint16_t len, uint16_t offset)
{
    LOG_INF("Read request received");
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &stored_value, sizeof(stored_value));
}

static ssize_t on_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    if (offset != 0 || len != 1)
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    uint8_t val = *((uint8_t *)buf);
    stored_value = val;

    LOG_INF("New value written: %d", stored_value);

    if (service_cb.on_write)
    {
        service_cb.on_write(val);
    }

    return len;
}

/* GATT structure */
BT_GATT_SERVICE_DEFINE(my_svc,
                       BT_GATT_PRIMARY_SERVICE(BT_UUID_MY_SERVICE),

                       BT_GATT_CHARACTERISTIC(BT_UUID_MY_CHAR_READ,
                                              BT_GATT_CHRC_READ,
                                              BT_GATT_PERM_READ,
                                              on_read, NULL, &stored_value),

                       BT_GATT_CHARACTERISTIC(BT_UUID_MY_CHAR_WRITE,
                                              BT_GATT_CHRC_WRITE,
                                              BT_GATT_PERM_WRITE,
                                              NULL, on_write, NULL));

int my_service_init(struct my_service_cb *cb)
{
    if (cb)
    {
        service_cb = *cb;
    }
    LOG_INF("Custom service initialized");
    return 0;
}
