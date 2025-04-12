# 05 - BLE GATT Client-Initiated Operations

In the GATT protocol, the **server** holds the data. The **client** can request the server to perform operations such as **read**, **write**, or **write without response** — these are known as *client-initiated operations*. Alternatively, the client can *subscribe* to **notifications** or **indications**, which are *server-initiated operations*.

This note focuses on **client-initiated operations**. The server-side implementation will be covered in a later note. We will walk through how to define a custom 128-bit UUID GATT service with both readable and writable characteristics in Zephyr, using the Nordic SDK style.


---

## 1. Create a File: `my_service.h`

This header will define the UUIDs, callback types, and initialization function needed for the custom service.

---

## 2. Encode the UUIDs Using `BT_UUID_128_ENCODE`

Zephyr provides a helper macro to define a 128-bit UUID:

```c
BT_UUID_128_ENCODE(w32, w1, w2, w3, w48)
```

This macro converts your UUID into little-endian byte order suitable for Zephyr’s internal structures. It's commonly used when defining:
- Service UUIDs
- Characteristic UUIDs
- Advertising UUIDs

### Parameters:
| Param | Size | Description                      |
|-------|------|----------------------------------|
| w32   | 32b  | First field of the UUID          |
| w1    | 16b  | Second field                     |
| w2    | 16b  | Third field                      |
| w3    | 16b  | Fourth field                     |
| w48   | 48b  | Final field (usually vendor part)|

> Just take your UUID, replace the dashes with commas, and prefix each value with `0x`.

### Example:

If your 128-bit UUID is:
```
12345678-9abc-def0-1234-56789abcdef0
```

You can encode it like:

```c
#define BT_UUID_MY_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x9abc, 0xdef0, 0x1234, 0x56789abcdef0)
```

And then define characteristics with similar base UUIDs:

```c
#define BT_UUID_MY_CHAR_READ_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x9abc, 0xdef0, 0x1234, 0x56789abcdef1)

#define BT_UUID_MY_CHAR_WRITE_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x9abc, 0xdef0, 0x1234, 0x56789abcdef2)
```

> 💡 **Naming convention (rule of thumb):**
> - The first few fields can vary by purpose (e.g., 1 service, multiple characteristics).
> - The final 48 bits are often treated as the vendor-defined base.
> - No strict rules — just make sure they’re unique.

---

## 3. Declare the UUIDs

Encoding a UUID gives you a byte array. To use them in APIs like `BT_GATT_PRIMARY_SERVICE()` or `BT_GATT_CHARACTERISTIC()`, you must wrap them with `BT_UUID_DECLARE_128()`:

```c
#define BT_UUID_MY_SERVICE     BT_UUID_DECLARE_128(BT_UUID_MY_SERVICE_VAL)
#define BT_UUID_MY_CHAR_READ   BT_UUID_DECLARE_128(BT_UUID_MY_CHAR_READ_VAL)
#define BT_UUID_MY_CHAR_WRITE  BT_UUID_DECLARE_128(BT_UUID_MY_CHAR_WRITE_VAL)
```

This declares each UUID as a `const struct bt_uuid *` that can be used with Zephyr’s GATT API.

---

## 4. Create a File: `my_service.c`

This file implements the callbacks, the internal state, and defines the GATT service.

---

## 5. Implement the Read Callback

Include the needed headers:

```c
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
```

Implement the read callback:

```c
static uint8_t stored_value;

static ssize_t on_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                       void *buf, uint16_t len, uint16_t offset)
{
    LOG_INF("Read request received");
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &stored_value, sizeof(stored_value));
}
```

> `bt_gatt_attr_read()` is a helper that reads from a memory buffer and does bounds checking for you.

---

## 6. Implement the Write Callback

```c
static struct my_service_cb service_cb;

static ssize_t on_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                        const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    if (offset != 0 || len != 1) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    uint8_t val = *((uint8_t *)buf);
    stored_value = val;

    LOG_INF("New value written: %d", stored_value);

    if (service_cb.on_write) {
        service_cb.on_write(val);
    }

    return len;
}
```

> You can choose to act immediately on the written value, or store it for later use.

---

## 7. Define a Callback Struct and Init Function

Define the callback type and storage:

```c
typedef void (*my_write_cb_t)(uint8_t new_value);

struct my_service_cb {
    my_write_cb_t on_write;
};
```

Then implement a simple init function:

```c
int my_service_init(struct my_service_cb *cb)
{
    if (cb) {
        service_cb = *cb;
    }
    LOG_INF("Custom service initialized");
    return 0;
}
```

This lets users register custom application logic on writes.

---

## 8. Define the GATT Service

Declare your service and characteristics using `BT_GATT_SERVICE_DEFINE`:

```c
BT_GATT_SERVICE_DEFINE(my_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_MY_SERVICE),

    BT_GATT_CHARACTERISTIC(BT_UUID_MY_CHAR_READ,
                           BT_GATT_CHRC_READ,
                           BT_GATT_PERM_READ,
                           on_read, NULL, &stored_value),

    BT_GATT_CHARACTERISTIC(BT_UUID_MY_CHAR_WRITE,
                           BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE,
                           NULL, on_write, NULL)
);
```

---

## 9. Use the Service in `main.c`

Register your service in `main()`:

```c
#include "my_service.h"

void my_write_handler(uint8_t value)
{
    LOG_INF("Value changed by client to %d", value);
}

void main(void)
{
    bt_enable(NULL);

    struct my_service_cb cb = {
        .on_write = my_write_handler,
    };

    my_service_init(&cb);

    bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
}
```
