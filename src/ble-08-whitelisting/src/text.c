/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>
#include <dk_buttons_and_leds.h>

#include "lbs.h"

LOG_MODULE_REGISTER(Lesson5_Exercise2, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define CON_STATUS_LED DK_LED2
#define RUN_LED_BLINK_INTERVAL 1000

#define USER_LED DK_LED3

#define USER_BUTTON DK_BTN1_MSK

/* STEP 1.2 - Add the header file for the Settings module */
#include <zephyr/settings/settings.h>

/* STEP 2.1 - Add extra button for bond deleting function */

#define BOND_DELETE_BUTTON DK_BTN2_MSK

/* STEP 4.2.1 - Add extra button and a static bool variable for enabling pairing mode */

#define PAIRING_BUTTON DK_BTN3_MSK
static bool pairing_mode = false;

/* STEP 3.2 - Define advertising parameter for when Accept List is used */
#define BT_LE_ADV_CONN_ACCEPT_LIST                                  \
    BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_FILTER_CONN, \
                    BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL)

static bool app_button_state;
static struct k_work adv_work;

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LBS_VAL),
};

/* STEP 3.3.1 - Define the callback to add addreses to the Accept List */
static void setup_accept_list_cb(const struct bt_bond_info *info, void *user_data)
{
    int *bond_cnt = user_data;

    if ((*bond_cnt) < 0)
    {
        return;
    }

    int err = bt_le_filter_accept_list_add(&info->addr);
    LOG_INF("Added following peer to accept list: %x %x\n", info->addr.a.val[0],
            info->addr.a.val[1]);
    if (err)
    {
        LOG_INF("Cannot add peer to filter accept list (err: %d)\n", err);
        (*bond_cnt) = -EIO;
    }
    else
    {
        (*bond_cnt)++;
    }
}

/* STEP 3.3.2 - Define the function to loop through the bond list */
static int setup_accept_list(uint8_t local_id)
{
    int err = bt_le_filter_accept_list_clear();

    if (err)
    {
        LOG_INF("Cannot clear accept list (err: %d)\n", err);
        return err;
    }

    int bond_cnt = 0;

    bt_foreach_bond(local_id, setup_accept_list_cb, &bond_cnt);

    return bond_cnt;
}

/* STEP 3.4.1 - Define the function to advertise with the Accept List */
static void adv_work_handler(struct k_work *work)
{
    int err = 0;
    /* STEP 4.2.3 Add extra code to advertise without using Accept List when pairing_mode is set to true */
    if (pairing_mode == true)
    {
        err = bt_le_filter_accept_list_clear();
        if (err)
        {
            LOG_INF("Cannot clear accept list (err: %d)\n", err);
        }
        else
        {
            LOG_INF("Accept list cleared succesfully");
        }
        pairing_mode = false;
        err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), sd,
                              ARRAY_SIZE(sd));
        if (err)
        {
            LOG_INF("Advertising failed to start (err %d)\n", err);
            return;
        }
        LOG_INF("Advertising successfully started\n");
        return;
    }
    /* STEP 3.4.1 - Remove the original code that does normal advertising */

    // err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    //  if (err) {
    //  	LOG_INF("Advertising failed to start (err %d)\n", err);
    //  	return;
    //  }
    // LOG_INF("Advertising successfully started\n");
    /* STEP 3.4.2 - Start advertising with the Accept List */
    int allowed_cnt = setup_accept_list(BT_ID_DEFAULT);
    if (allowed_cnt < 0)
    {
        LOG_INF("Acceptlist setup failed (err:%d)\n", allowed_cnt);
    }
    else
    {
        if (allowed_cnt == 0)
        {
            LOG_INF("Advertising with no Accept list \n");
            err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), sd,
                                  ARRAY_SIZE(sd));
        }
        else
        {
            LOG_INF("Advertising with Accept list \n");
            LOG_INF("Acceptlist setup number  = %d \n", allowed_cnt);
            err = bt_le_adv_start(BT_LE_ADV_CONN_ACCEPT_LIST, ad, ARRAY_SIZE(ad), sd,
                                  ARRAY_SIZE(sd));
        }
        if (err)
        {
            LOG_INF("Advertising failed to start (err %d)\n", err);
            return;
        }
        LOG_INF("Advertising successfully started\n");
    }
}

static void advertising_start(void)
{
    k_work_submit(&adv_work);
}

static void on_connected(struct bt_conn *conn, uint8_t err)
{
    if (err)
    {
        LOG_INF("Connection failed (err %u)\n", err);
        return;
    }

    LOG_INF("Connected\n");

    dk_set_led_on(CON_STATUS_LED);
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected (reason %u)\n", reason);
    dk_set_led_off(CON_STATUS_LED);
}

static void recycled_cb(void)
{
    printk("Connection object available from previous conn. Disconnect/stop advertising is completed!\n");
    advertising_start();
}

static void on_security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (!err)
    {
        LOG_INF("Security changed: %s level %u\n", addr, level);
    }
    else
    {
        LOG_INF("Security failed: %s level %u err %d\n", addr, level, err);
    }
}
struct bt_conn_cb connection_callbacks = {
    .connected = on_connected,
    .disconnected = on_disconnected,
    .recycled = recycled_cb,
    .security_changed = on_security_changed,
};
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .passkey_display = auth_passkey_display,
    .cancel = auth_cancel,
};

static void app_led_cb(bool led_state)
{
    dk_set_led(USER_LED, led_state);
}

static bool app_button_cb(void)
{
    return app_button_state;
}

static struct bt_lbs_cb lbs_callbacs = {
    .led_cb = app_led_cb,
    .button_cb = app_button_cb,
};

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
    if (has_changed & USER_BUTTON)
    {
        uint32_t user_button_state = button_state & USER_BUTTON;

        bt_lbs_send_button_state(user_button_state);
        app_button_state = user_button_state ? true : false;
    }
    /* STEP 2.2 - Add extra button handling to remove bond information */
    if (has_changed & BOND_DELETE_BUTTON)
    {
        uint32_t bond_delete_button_state = button_state & BOND_DELETE_BUTTON;
        if (bond_delete_button_state == 0)
        {
            int err = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
            if (err)
            {
                LOG_INF("Cannot delete bond (err: %d)\n", err);
            }
            else
            {
                LOG_INF("Bond deleted succesfully");
            }
        }
    }
    /* STEP 4.2.2 Add extra button handling pairing mode (advertise without using Accept List) */

    if (has_changed & PAIRING_BUTTON)
    {
        uint32_t pairing_button_state = button_state & PAIRING_BUTTON;
        if (pairing_button_state == 0)
        {
            pairing_mode = true;
            int err_code = bt_le_adv_stop();
            if (err_code)
            {
                LOG_INF("Cannot stop advertising err= %d \n", err_code);
                return;
            }
            // recycled_cb will be called after the advertising stopped we will continue advertise when we receive that callback
        }
    }
}

static int init_button(void)
{
    int err;

    err = dk_buttons_init(button_changed);
    if (err)
    {
        LOG_INF("Cannot init buttons (err: %d)\n", err);
    }

    return err;
}

int main(void)
{
    int blink_status = 0;
    int err;

    LOG_INF("Starting Lesson 5 - Exercise 2 \n");

    err = dk_leds_init();
    if (err)
    {
        LOG_INF("LEDs init failed (err %d)\n", err);
        return -1;
    }

    err = init_button();
    if (err)
    {
        LOG_INF("Button init failed (err %d)\n", err);
        return -1;
    }
    err = bt_conn_auth_cb_register(&conn_auth_callbacks);
    if (err)
    {
        LOG_INF("Failed to register authorization callbacks.\n");
        return -1;
    }
    bt_conn_cb_register(&connection_callbacks);

    err = bt_enable(NULL);
    if (err)
    {
        LOG_INF("Bluetooth init failed (err %d)\n", err);
        return -1;
    }

    LOG_INF("Bluetooth initialized\n");

    /* STEP 1.3 - Add setting load function */
    settings_load();

    err = bt_lbs_init(&lbs_callbacs);
    if (err)
    {
        LOG_INF("Failed to init LBS (err:%d)\n", err);
        return -1;
    }

    k_work_init(&adv_work, adv_work_handler);
    advertising_start();

    for (;;)
    {
        dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
        k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
    }
}