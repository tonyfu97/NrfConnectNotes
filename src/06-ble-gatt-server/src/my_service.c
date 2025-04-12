#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "my_service.h"

LOG_MODULE_REGISTER(my_service, LOG_LEVEL_INF);

static bool notify_enabled;
static bool indicate_enabled;
static struct bt_gatt_indicate_params ind_params;
static uint8_t dummy_cmd;

static ssize_t write_cmd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                         const void *buf, uint16_t len, uint16_t offset, uint8_t flags);

static void critical_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    indicate_enabled = (value == BT_GATT_CCC_INDICATE);
    LOG_INF("Indicate enabled: %s", indicate_enabled ? "true" : "false");
}

static void noncritical_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    notify_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("Notify enabled: %s", notify_enabled ? "true" : "false");
}

BT_GATT_SERVICE_DEFINE(test_svc,
                       BT_GATT_PRIMARY_SERVICE(BT_UUID_TEST_SERVICE),

                       // Command char (write)
                       BT_GATT_CHARACTERISTIC(BT_UUID_TEST_CMD, BT_GATT_CHRC_WRITE,
                                              BT_GATT_PERM_WRITE, NULL, write_cmd, NULL),

                       // Critical (indicate)
                       BT_GATT_CHARACTERISTIC(BT_UUID_TEST_CRITICAL, BT_GATT_CHRC_INDICATE,
                                              BT_GATT_PERM_NONE, NULL, NULL, NULL),
                       BT_GATT_CCC(critical_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

                       // Non-critical (notify)
                       BT_GATT_CHARACTERISTIC(BT_UUID_TEST_NONCRITICAL, BT_GATT_CHRC_NOTIFY,
                                              BT_GATT_PERM_NONE, NULL, NULL, NULL),
                       BT_GATT_CCC(noncritical_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

static void indicate_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
    LOG_DBG("Indication result: %s", err == 0U ? "success" : "fail");
}

static ssize_t write_cmd(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                         const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    if (len != sizeof(uint8_t))
    {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    dummy_cmd = *((uint8_t *)buf);

    const uint32_t dummy_data = 0xAABBCCDD;

    if (dummy_cmd)
    {
        // Indicate critical data
        if (!indicate_enabled)
        {
            LOG_WRN("Indications not enabled");
            return -EACCES;
        }
        LOG_INF("Indicating critical data: %x", dummy_data);
        ind_params.attr = &test_svc.attrs[3];
        ind_params.func = indicate_cb;
        ind_params.data = &dummy_data;
        ind_params.len = sizeof(dummy_data);
        return bt_gatt_indicate(NULL, &ind_params);
    }
    else
    {
        // Notify non-critical data
        if (!indicate_enabled)
        {
            LOG_WRN("Indications not enabled");
            return -EACCES;
        }
        LOG_INF("Notifying non-critical data: %x", dummy_data);

        return bt_gatt_notify(NULL, &test_svc.attrs[6], &dummy_data, sizeof(dummy_data));
    }
}