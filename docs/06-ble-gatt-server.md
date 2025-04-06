# 06 - BLE GATT Server-Initiated Operations

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


