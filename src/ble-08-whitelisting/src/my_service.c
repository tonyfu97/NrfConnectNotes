#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "my_service.h"

LOG_MODULE_REGISTER(my_service, LOG_LEVEL_INF);

static ssize_t encrypt_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                             const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    if (len != sizeof(uint8_t))
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    uint8_t value = *(uint8_t *)buf;
    LOG_INF("Encrypted write: %u", value);

    return len;
}

static ssize_t authen_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    if (len != sizeof(uint8_t))
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    uint8_t value = *(uint8_t *)buf;
    LOG_INF("Authenticated write: %u", value);

    return len;
}

BT_GATT_SERVICE_DEFINE(my_svc,
                       BT_GATT_PRIMARY_SERVICE(BT_UUID_TEST_SERVICE),

                       // Encrypted characteristic
                       BT_GATT_CHARACTERISTIC(BT_UUID_TEST_ENCRYPT, BT_GATT_CHRC_WRITE,
                                              BT_GATT_PERM_WRITE_ENCRYPT, NULL, encrypt_write, NULL),

                       // Authenticated characteristic
                       BT_GATT_CHARACTERISTIC(BT_UUID_TEST_AUTHEN, BT_GATT_CHRC_WRITE,
                                              BT_GATT_PERM_WRITE_AUTHEN, NULL, authen_write, NULL),

);
