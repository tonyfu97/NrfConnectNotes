# 07 - BLE Security Modes

**Author:** Tony Fu  
**Date:** 2025/4/13
**Device:** nRF52840 Dongle
**Toolchain:** nRF Connect SDK v2.8.0

## Common BLE Security Threats

Here are some common threats to BLE security:

### Identity Tracking (**ID**)
Tracking a device over time using its Bluetooth address. Prevented by using **resolvable private addresses** with the **IRK (Identity Resolving Key)** to obscure the device identity.

### Passive Eavesdropping (**PE**)
An attacker silently listens to data over the air. Prevented by **encrypting** the connection.

### Man-in-the-Middle (**MITM**)
An attacker impersonates both peers, relaying and potentially modifying messages. Prevented by **authenticated pairing** methods that confirm peer identity (e.g., Passkey Entry, Numeric Comparison).

---

## BLE Security Levels

| **Security Level** | **Description**                                                                 | **Protects Against**        |
|--------------------|----------------------------------------------------------------------------------|-----------------------------|
| **Level 1**        | No security: no encryption, no authentication.                                   | _None_                      |
| **Level 2**        | Encrypted link using **unauthenticated pairing** (e.g., Just Works).             | PE                          |
| **Level 3**        | Encrypted link with **authenticated pairing** (e.g., Passkey, OOB with Legacy).  | PE, basic MITM              |
| **Level 4**        | Encrypted link with **LE Secure Connections + authentication**.                  | PE, MITM, ID (with privacy) |

### Notes:
- All connections start at **Level 1**, then upgrade during pairing.
- **Just Works** leads to **Level 2** — encrypted but not authenticated.
- **Passkey Entry** or **OOB** leads to **Level 3**.
- **Level 4** requires both devices to support **LE Secure Connections** and use authenticated pairing.
- **Privacy (ID protection)** is separate from these levels but essential to combat **identity tracking**.

---

## Connecting vs Pairing vs Bonding

Before diving into BLE security modes, it's important to understand three foundational terms: **connecting**, **pairing**, and **bonding**. These steps form the basis of how two Bluetooth devices establish trust. While connecting simply creates a communication link, **pairing** is the beginning of any real security—it’s when encryption keys are generated and exchanged. **Bonding** builds on pairing by storing that trust for future sessions.

- **Connecting**
  - Establishes a temporary link between two BLE devices.
  - No security by default.

- **Pairing**
  - Establishes **trusted relationship** for the current session.
  - Negotiates and exchanges encryption keys.
  - Enables secure data transfer (encryption, authentication).

- **Bonding**
  - Saves pairing information (keys) to **non-volatile memory**.
  - Allows devices to reconnect securely **without re-pairing**.
  - Typically happens automatically after successful pairing if both devices support it.

> In short: **Connect → Pair → Bond (optional but persistent)**.


---


## More on Pairing

The **pairing process** can be further divided into three distinct phases:

### Phase 1: Pairing Feature Exchange
- Initiated by the **central** device via a **Pairing Request**.
- The **peripheral** responds with a **Pairing Response**.
- Devices exchange their:
  - **I/O capabilities** (e.g., keyboard, display, none).
  - **OOB and MITM flags** (to determine required security).
  - Supported **authentication and encryption options**.
  - Whether they wish to bond or just pair temporarily.
- Based on this information, the devices will choose an appropriate **pairing method** in Phase 2.

### Phase 2: Key Generation and Authentication
- Devices perform cryptographic operations to generate encryption keys.
- Depending on the pairing mode:
  - **Legacy Pairing**: Generates a **Short Term Key (STK)** from a **Temporary Key (TK)**.
  - **LE Secure Connections**: Uses Elliptic Curve Diffie-Hellman (ECDH) to generate a **Long Term Key (LTK)** directly and securely.
- The actual **pairing method** used depends on capabilities and flags:
  - **Just Works** (unauthenticated)
  - **Passkey Entry** (user types in/display 6-digit code)
  - **Out-of-Band (OOB)** (e.g., via NFC)
  - **Numeric Comparison** (LE Secure Connections only)
- This phase determines **how secure** the pairing is — especially whether it's protected against MITM attacks.

### Phase 3: Key Distribution (only if bonding)
- If bonding is agreed upon, devices exchange and store keys:
  - **LTK** (used to encrypt future connections)
  - **IRK** (Identity Resolving Key for address privacy)
  - **CSRK** (for signed data, optional)
- These keys are saved to non-volatile memory so that future reconnections can skip pairing and resume secure communication immediately.

---

## More on Phase 2: Pairing Methods

The method used to perform pairing directly affects the **security level**.


| **Method**            | **Authentication** | **Requires User Interaction** | **MITM Protection** | **Available In**               |
|-----------------------|--------------------|-------------------------------|----------------------|--------------------------------|
| **Just Works**        | ❌ No               | Minimal (accept prompt)       | ❌ No                | Legacy & LE Secure Connections |
| **Passkey Entry**     | ✅ Yes              | Yes (input or display)        | ✅ Yes               | Legacy & LE Secure Connections |
| **Out of Band (OOB)** | ✅ Yes              | External channel (e.g., NFC)  | ✅ Yes               | Legacy & LE Secure Connections |
| **Numeric Comparison**| ✅ Yes              | Yes (compare 6-digit number)  | ✅ Yes               | **LE Secure Connections only** |

### Details

#### **Just Works**
- The simplest pairing method.
- Devices derive encryption keys without authentication.
- Used when neither device has a display or keyboard (e.g., most wearables).
- Susceptible to **MITM attacks**, as the user cannot verify peer identity.
- Results in **Security Level 2** (unauthenticated encryption).

#### **Passkey Entry**
- A **6-digit passkey** is shown on one device and entered on the other.
- Which device displays or inputs depends on I/O capabilities.
- Provides **authenticated pairing**, resistant to MITM.
- Yields **Security Level 3** (authenticated encryption).

#### **Out of Band (OOB)**
- Key material is exchanged over an external medium (e.g., **NFC**, QR codes).
- Ideal for devices with limited BLE I/O but secure secondary channels.
- Only **one** device needs to support OOB for it to be used (in Secure Connections).
- Provides MITM protection, assuming the OOB channel is trusted.

#### **Numeric Comparison**
- Both devices display the same 6-digit number.
- The user confirms if the numbers match (presses “Yes”).
- Ensures that no attacker is relaying the connection.
- Only available in **LE Secure Connections**.
- Offers strong MITM protection and **Security Level 4**.

### How Pairing Method is Selected

Pairing method selection is automatic and based on:
- Devices' **I/O capabilities**
- **OOB** and **MITM** flags exchanged in Phase 1
- Support for **LE Secure Connections**

If **OOB is supported**, it is chosen.  
Otherwise, if **MITM is required**, Passkey Entry or Numeric Comparison is used.  
If neither applies, **Just Works** is used by default.

---

# In Action

In this section, we’ll define two writable characteristics:  
- One that **requires encryption** to access  
- Another that **requires authentication**  

When you try to write to either from your phone app, the device will prompt you to pair, depending on the required security level.

---

### 1. Enable pairing support

Enable the Security Manager Protocol (SMP) in your `prj.conf`:

```kconfig
CONFIG_BT_SMP=y
```

---

### 2. Declare the characteristic UUIDs

Just like in previous examples, define custom UUIDs for the service and its characteristics:

```c
#define BT_UUID_TEST_SERVICE_VAL BT_UUID_128_ENCODE(0x12345678, 0x9abc, 0xdef0, 0x1234, 0x56789abcdef0)
#define BT_UUID_TEST_ENCRYPT_VAL BT_UUID_128_ENCODE(0x12345678, 0x9abc, 0xdef0, 0x1234, 0x56789abcdef1)

#define BT_UUID_TEST_SERVICE BT_UUID_DECLARE_128(BT_UUID_TEST_SERVICE_VAL)
#define BT_UUID_TEST_ENCRYPT BT_UUID_DECLARE_128(BT_UUID_TEST_ENCRYPT_VAL)
```

---

### 3. Define the characteristics

We use `BT_GATT_PERM_WRITE_ENCRYPT` to require link encryption for writing:

```c
BT_GATT_SERVICE_DEFINE(my_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_TEST_SERVICE),

    // Encrypted write characteristic
    BT_GATT_CHARACTERISTIC(BT_UUID_TEST_ENCRYPT, BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE_ENCRYPT, NULL, encrypt_write, NULL),
);
```

---

### 4. Implement the write handler

Same as earlier examples (not shown here). It will be called once security is sufficient.

---

### 5. Track security level changes

Use the `security_changed` callback to monitor when the connection is encrypted or authenticated:

```c
static void on_security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char peer_addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), peer_addr, sizeof(peer_addr));

	if (err == 0) {
		LOG_INF("Link secured with %s (level %u)", peer_addr, level);
	} else {
		LOG_WRN("Security setup failed with %s (level %u, err %d)", peer_addr, level, err);
	}
}

struct bt_conn_cb connection_callbacks = {
	.connected = on_connected,
	.disconnected = on_disconnected,
	.security_changed = on_security_changed,
};
```

---

### 6. Attempt a write

When you try to write to the encrypted characteristic, the phone will prompt you to pair.  
Once encrypted, the connection will be promoted to **Security Level 2**.

---

### 7. Unpair before moving on

To start fresh:
- On your phone, unpair the device via Bluetooth settings.
- In firmware, clear bonding info with:

```c
bt_unpair(BT_ID_DEFAULT, NULL);
```

---

### 8. Add an authenticated characteristic

Now define a second characteristic that requires **authentication**:

```c
#define BT_UUID_TEST_AUTHEN_VAL BT_UUID_128_ENCODE(0x12345678, 0x9abc, 0xdef0, 0x1234, 0x56789abcdef2)
#define BT_UUID_TEST_AUTHEN BT_UUID_DECLARE_128(BT_UUID_TEST_AUTHEN_VAL)

BT_GATT_SERVICE_DEFINE(my_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_TEST_SERVICE),

    // Encrypted write characteristic
    BT_GATT_CHARACTERISTIC(BT_UUID_TEST_ENCRYPT, BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE_ENCRYPT, NULL, encrypt_write, NULL),

    // Authenticated write characteristic
    BT_GATT_CHARACTERISTIC(BT_UUID_TEST_AUTHEN, BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_WRITE_AUTHEN, NULL, authen_write, NULL),
);
```

---

### 9. Display the passkey

To complete an authenticated pairing, we need to show the user a passkey. This is done using the authentication callbacks:

```c
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
```

---

### 10. Register authentication handlers

Don't forget to register your callbacks during initialization:

```c
int main(void)
{
    bt_unpair(BT_ID_DEFAULT, NULL);
    bt_conn_auth_cb_register(&auth_callbacks);
    bt_conn_cb_register(&connection_callbacks);

    bt_enable(NULL);
    // Other setup...
}
```
