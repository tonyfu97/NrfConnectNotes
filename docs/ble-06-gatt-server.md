# 06 - BLE GATT Server-Initiated Operations

**Author:** Tony Fu  
**Date:** 2025/4/6
**Device:** nRF52840 Dongle
**Toolchain:** nRF Connect SDK v2.8.0

Before diving into server-initiated operations like **notifications** and **indications**, it’s important to understand the layers beneath them: **ATT** and **GATT**. These layers form the foundation of BLE’s server-client communication model.

We want to get comfortable with **GATT**, as it’s the layer most application developers interact with. But beneath GATT lies **ATT**, which provides the raw data transport mechanism.

---

## ATT (Attribute Protocol)

ATT defines a minimal protocol for exposing data as a list of **attributes** on the server. Each attribute is a generic container that GATT builds upon (services, characteristics, descriptors).

An ATT attribute consists of:

| Field      | Size     | Description                                                                 |
|------------|----------|-----------------------------------------------------------------------------|
| **Handle** | 2 bytes  | A unique ID for the attribute on the server. Used by clients to reference it. |
| **Type**   | 2 or 16 bytes | A UUID that indicates what kind of attribute this is (e.g., service, characteristic). |
| **Permission** | —   | Access control: read, write, notify, etc.                                    |
| **Value**  | Variable | Actual data content (e.g., the characteristic's value).                     |

### Common Attribute Types

These are 16-bit **standardized UUIDs** defined by the Bluetooth SIG. They are used to define structure within a GATT service:

| Type (UUID) | Description                              | Meaning                                                                 |
|-------------|------------------------------------------|-------------------------------------------------------------------------|
| `0x2800`    | Primary Service                          | Marks the beginning of a primary service declaration.                   |
| `0x2801`    | Secondary Service                        | Used to define a secondary (helper) service that is referenced by another. |
| `0x2803`    | Characteristic Declaration               | Describes a characteristic: includes properties, handle, and UUID.      |
| `0x2901`    | Characteristic User Description          | Human-readable label (e.g., `"Heart Rate"`). Shown in GUIs.             |
| `0x2902`    | Client Characteristic Configuration (CCCD) | Lets clients enable notifications or indications for a characteristic.  |

- GATT uses these generic ATT attributes to structure and organize the data.
- The type of a characteristic is defined by its UUID, which can be a 16-bit or 128-bit value.

### Attribute Permissions

Each attribute has a **set of permissions** that control how clients are allowed to interact with it — such as whether it can be read, written, or requires a secure connection.

These permissions are enforced by the **ATT server**, regardless of what the characteristic *claims* to support (via its properties). Here's a mapping of common permissions:

| Permission               | Description                                                                 | Zephyr Macro              |
|--------------------------|-----------------------------------------------------------------------------|---------------------------|
| **Plain Read**           | Client can read the attribute value.                                        | `BT_GATT_PERM_READ`       |
| **Plain Write**          | Client can write a new value.                                               | `BT_GATT_PERM_WRITE`      |
| **Encrypted Read**       | Read only allowed over an encrypted connection.                             | `BT_GATT_PERM_READ_ENCRYPT` |
| **Encrypted Write**      | Write only allowed over an encrypted connection.                            | `BT_GATT_PERM_WRITE_ENCRYPT` |
| **Authenticated Read**   | Read allowed only after authentication (e.g., MITM pairing).                | `BT_GATT_PERM_READ_AUTHEN` |
| **Authenticated Write**  | Write allowed only after authentication (e.g., MITM pairing).               | `BT_GATT_PERM_WRITE_AUTHEN` |
| **LESC Read**            | Read requires LE Secure Connections.                                        | `BT_GATT_PERM_READ_LESC`  |
| **LESC Write**           | Write requires LE Secure Connections.                                       | `BT_GATT_PERM_WRITE_LESC` |
| **Prepare Write**        | Attribute supports queued writes (long/atomic writes).                      | `BT_GATT_PERM_PREPARE_WRITE` |

> 🔐 When no appropriate permissions are granted (e.g., no `BT_GATT_PERM_READ`), the server will reject the client’s operation with an ATT error like `Read Not Permitted`.

This permission layer acts independently from characteristic **properties**, which simply advertise what *can* be done — permissions control what is actually *allowed* at runtime.

Also, here is a quick note on the different security levels (will be covered in more detail later):

- **Encrypted** access means the connection must be encrypted (e.g., after pairing). This protects data from passive eavesdropping.
- **Authenticated** access goes a step further — the connection must be encrypted *and* use an authenticated key (typically generated with MITM protection, like passkey entry). This defends against impersonation attacks.
- **LESC (LE Secure Connections)** is a newer pairing method introduced in Bluetooth 4.2. It uses Elliptic Curve Diffie-Hellman (ECDH) for key exchange and offers stronger protection against passive and active attacks compared to older methods.


---


## GATT (Generic Attribute Profile)

The **Generic Attribute Profile (GATT)** defines how the low-level attributes defined by the **ATT protocol** are grouped and interpreted to represent meaningful data. While **ATT** is concerned purely with the format and transport of attributes (each with a handle, type, permissions, and value), **GATT** gives structure to those attributes by organizing them into logical groupings like **services**, **characteristics**, and **descriptors**.

At its core, GATT uses multiple ATT attributes to represent each component:

- A **Service** is a collection of related characteristics (e.g., the Heart Rate Service).
- A **Characteristic** represents a data item (e.g., current heart rate) and is made up of three attributes: a **declaration**, a **value**, and optional **descriptors**.
- A **Descriptor** is additional metadata about a characteristic, such as a human-readable label or client configuration (e.g., enabling notifications).

Each attribute is given a handle, type, permssions, and value. Handles are assgined by the server and can vary between different BLE stacks, but it is worth discussing the other three fields in more detail:

### 1. **Service Declaration**

| Type     | Permission    | Value                |
|----------|---------------|----------------------|
| `0x2800` (Primary) or `0x2801` (Secondary) | Read | UUID of the service (e.g., `0x180F` for Battery Service) |

### 2. **Characteristic Declaration**

| Type     | Permission    | Value                                |
|----------|---------------|--------------------------------------|
| `0x2803` (Characteristic Declaration) | `Read` | - **Properties** (1 byte): Bitfield indicating allowed operations (e.g., Read, Write, Notify).<br>- **Value Handle** (2 bytes): Handle pointing to the **Characteristic Value** attribute.<br>- **Characteristic UUID** (2 or 16 bytes): UUID of the characteristic. |

### 3. **Characteristic Value**

| Type (UUID of Characteristic) | Permission | Value |
|-------------------------------|------------|-------|
| The UUID of the characteristic (e.g., `0x2A19` for Battery Level) | Depends on characteristic (e.g., Read, Write, Notify) | Raw data (e.g., sensor reading, config byte). This is the actual **payload** a client interacts with. |

### 4. **Descriptor Attribute** (optional)

| Type       | Permission       | Value                          |
|------------|------------------|--------------------------------|
| e.g., `0x2901` (User Description), `0x2902` (CCCD) | Depends on descriptor:<br>- `0x2901`: Read<br>- `0x2902`: Read/Write | - **User Description (0x2901)**: Human-readable name of the characteristic (e.g., "Temperature").<br>- **CCCD (0x2902)**: 2-byte bitfield that enables notifications and/or indications.<br>- Others: May define triggers, ranges, or valid formats. |


---


## Service-Initiated Operations: Indication and Notification

**Indication** and **notification** are operations used to push data from a GATT server  to a client.

- **Notification** is a lightweight, unacknowledged data push. It is suitable for non-critical or frequently changing data, such as sensor streams or status updates. Notifications are fast but not guaranteed to arrive.

- **Indication** is a reliable, acknowledged mechanism. Each indication must be confirmed by the client before the next can be sent. This makes it ideal for critical data or state changes that must be received.

Use **notification** for speed and simplicity. Use **indication** when reliability is essential.


---


## In Action

### Purpose of this example

This example demonstrates how to create a custom GATT service with three characteristics:

1. **Command characteristic** (Write): receives a control byte from a client (e.g., phone).
2. **Critical characteristic** (Indicate): used to send important data that requires acknowledgment.
3. **Non-critical characteristic** (Notify): used to send less important, best-effort data.

The purpose is to test how a peripheral (nRF52840 dongle) can push different types of data to a central (like a phone), depending on a command it receives. Writing `0x00` triggers a notification, while any non-zero value triggers an indication.

---

### Steps to implement

---

#### Step 1. Declare UUIDs

Custom 128-bit UUIDs are declared for the service and each of the three characteristics. This is just like what we have done in the previous example.

```c
#include <zephyr/types.h>
#include <zephyr/bluetooth/uuid.h>

#define BT_UUID_TEST_SERVICE_VAL BT_UUID_128_ENCODE(0x12345678, 0x9abc, 0xdef0, 0x1234, 0x56789abcdef0)
#define BT_UUID_TEST_CMD_VAL BT_UUID_128_ENCODE(0x12345678, 0x9abc, 0xdef0, 0x1234, 0x56789abcdef1)
#define BT_UUID_TEST_CRITICAL_VAL BT_UUID_128_ENCODE(0x12345678, 0x9abc, 0xdef0, 0x1234, 0x56789abcdef2)
#define BT_UUID_TEST_NONCRITICAL_VAL BT_UUID_128_ENCODE(0x12345678, 0x9abc, 0xdef0, 0x1234, 0x56789abcdef3)

#define BT_UUID_TEST_SERVICE BT_UUID_DECLARE_128(BT_UUID_TEST_SERVICE_VAL)
#define BT_UUID_TEST_CMD BT_UUID_DECLARE_128(BT_UUID_TEST_CMD_VAL)
#define BT_UUID_TEST_CRITICAL BT_UUID_DECLARE_128(BT_UUID_TEST_CRITICAL_VAL)
#define BT_UUID_TEST_NONCRITICAL BT_UUID_DECLARE_128(BT_UUID_TEST_NONCRITICAL_VAL)
```

---

#### Step 2. Define the service and characteristics

Here we define the service and its characteristics using `BT_GATT_SERVICE_DEFINE`. Each characteristic is assigned appropriate properties (`WRITE`, `INDICATE`, `NOTIFY`) and permissions.

```c
BT_GATT_SERVICE_DEFINE(test_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_TEST_SERVICE),

    BT_GATT_CHARACTERISTIC(BT_UUID_TEST_CMD, BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE, NULL, write_cmd, NULL),

    BT_GATT_CHARACTERISTIC(BT_UUID_TEST_CRITICAL, BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(critical_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    BT_GATT_CHARACTERISTIC(BT_UUID_TEST_NONCRITICAL, BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(noncritical_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
);
```

> **Note**: `BT_GATT_CCC` adds a Client Characteristic Configuration Descriptor, which allows the client to enable or disable notifications/indications.

---

#### Step 3. Define the CCCD callback functions

These callbacks are used in the previous step to handle changes in the CCCD values.

```c
static bool notify_enabled;
static bool indicate_enabled;

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
```

---

#### Step 4. Define the write callback function

The write callback interprets the value written by the client and sends either a notification or an indication.

```c
static struct bt_gatt_indicate_params ind_params;
static uint8_t dummy_cmd;

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
        if (!notify_enabled)
        {
            LOG_WRN("Notifications not enabled");
            return -EACCES;
        }
        LOG_INF("Notifying non-critical data: %x", dummy_data);
        return bt_gatt_notify(NULL, &test_svc.attrs[6], &dummy_data, sizeof(dummy_data));
    }
}
```

> **Note**:
- `bt_gatt_indicate()` uses a parameter structure because it handles asynchronous acknowledgment.  
- `bt_gatt_notify()` is simpler—no acknowledgment, no callback, just send.
- The `attr` indices for `.attrs[3]` and `.attrs[6]` refer to the **characteristic declaration attribute**, not the value or CCCD. Remember:  
  - Service declaration = 1 attribute  
  - Each characteristic = 2 attributes (declaration + value)  
  - Optional CCCD = 1 attribute  
  - So manually counting the offsets is required unless dynamic lookup is used.

---

#### Step 5. Define the indication callback

This is called after the client acknowledges the indication.

```c
static void indicate_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
    LOG_DBG("Indication result: %s", err == 0U ? "success" : "fail");
}
```
