# NFC 01: Introduction to NFC

**Author:** Tony Fu  
**Date:** 2025/04/27  
**Device:** nRF52840 DK  
**Toolchain:** nRF Connect SDK v3.0.0  


---


## What is NFC?

NFC stands for Near Field Communication. It allows two devices to communicate when they are very close, usually within a few centimeters. It is based on magnetic induction, not normal radio waves like Wi-Fi or Bluetooth. It is designed for very short-range and fast interactions. It does not need pairing or long setup.

One device usually acts as a passive tag (it does not need power), and the other acts as an active reader (it powers the communication). Some devices, like phones, can do both.


---


## NFC Device Roles

In NFC communication, devices have specific roles:

### Tag (Passive)  
A tag stores data. It does not create its own signal. It waits to be powered by a reader. Example: an NFC sticker, a smart card, or an nRF board acting as a tag.

### Reader/Writer (Active)  
A reader generates a magnetic field. It reads data from or writes data to a tag. Example: a smartphone reading a poster or tapping to pay.

### Peer-to-Peer (Both Active)  
Two devices can talk to each other by taking turns. Both generate fields and exchange data. Example: two phones sharing a contact by tapping together.

Most nRF boards can only act as a tag.


---


## NFC Tag Types

When using NFC, devices can either act as a polling device (reader) or a tag (listener). We will focus only on the tag role in this subsection.

A tag is a passive NFC device. It does not generate its own electromagnetic field but waits for a polling device like a smartphone to scan it. When scanned, the tag responds by modulating the field. The tag contains simple data, such as a URL or text.

Most of Nordic Semiconductor's nRF5 Series devices have a built-in NFC Tag peripheral. This peripheral operates using NFC-A technology (i.e., following the ISO/IEC 14443 Type A standard). NFC-A is the most widely used NFC signaling method and is compatible with most smartphones.

The nRF Connect SDK provides libraries to emulate two kinds of NFC tag protocols:

### Type 2 Tag  
Based on the NFC Forum Type 2 specification. These tags are simple, lightweight, and fast enough for basic NFC applications like sharing URLs, text messages, or launching apps.

### Type 4 Tag  
Based on the NFC Forum Type 4 specification. These tags can store more data, support security features like authentication, and can be used for advanced tasks such as emulating access cards or payment cards.

The nRF5 SDK or nRF Connect SDK includes precompiled libraries for both Type 2 and Type 4 Tag emulation. These libraries communicate with the hardware through the NFCT driver inside the nrfxlib module.

Other NFC tag types defined by the NFC Forum include:

### Type 1 Tag  
The earliest NFC tag type. Very simple. Low speed and small memory. Not common today.

### Type 3 Tag  
More complex. Supports higher data rates. Originally designed for specific applications like transit cards in Japan.

### Type 5 Tag  
Newer standard. Often used for long-range RFID and NFC applications. Less common in consumer devices.

Table comparing the NFC Tag Types:

| Tag Type | Memory Size | Speed | Special Features | Common Use |
|:---|:---|:---|:---|:---|
| Type 1 | Small | Slow | Basic functionality, simple locking | Rare today, early posters |
| Type 2 | Small to Medium | Medium | Lightweight, flexible, widely supported | Stickers, marketing, simple IoT devices |
| Type 3 | Medium to Large | Medium to High | Complex, peer-to-peer possible | Transit systems (e.g., FeliCa cards in Japan) |
| Type 4 | Medium to Large | High | Secure messaging, encryption, smartcard support | Access cards, payment prototypes |
| Type 5 | Variable | Medium to High | Long-range NFC and RFID integration | Industrial applications, asset tracking |


---


## NDEF: NFC Data Exchange Format

* **Purpose**: A standardized data format to encapsulate typed data in NFC communications.
* **Defined by**: NFC Forum.
* **Usage**: For reading/writing data on NFC tags and peer-to-peer communication.

### Structure:

* **NDEF Message**: A container that holds one or more NDEF Records.
* **NDEF Record**: Basic unit of NDEF data, with a header and a payload.

### Record Header Fields:

* **MB (Message Begin)**: Set in the first record.
* **ME (Message End)**: Set in the last record.
* **CF (Chunk Flag)**: For chunked payloads.
* **SR (Short Record)**: Indicates 1-byte payload length (payload < 256 bytes).
* **IL (ID Length Present)**: Indicates presence of an ID field.
* **TNF (Type Name Format)**: 3-bit field indicating record type.

### Record Fields:

* **Type**: Describes payload type (e.g., text, URI).
* **ID**: Optional identifier for the record.
* **Payload**: Actual data (e.g., text string, URL).

### Common TNF Values:

* `0x01` – Well-known type (e.g., Text, URI)
* `0x02` – MIME media type
* `0x04` – External type

### Common Record Types:

* **Text**: Type = "T", Payload = language code + UTF text.
* **URI**: Type = "U", Payload = prefix byte + URI string.

### Encoding Example (Text):

Payload = `[lang code len][lang code][UTF-8 text]`


---


## In Action

This code sets up NFC Type 2 Tag emulation on the nRF52840 DK using Nordic's NFC stack. It builds an NDEF message containing a single well-known Text record with the payload "Hello World!" encoded in UTF-8. When a phone or NFC reader enters the field, the tag is activated and the message is transmitted.

---

### Step 1: Configure the NFC Stack

```kconfig
# NFC Type 2 Tag support via Nordic's nrfxlib backend
CONFIG_NFC_T2T_NRFXLIB=y

# Enable core NDEF functionality
CONFIG_NFC_NDEF=y

# Enable NDEF message handling
CONFIG_NFC_NDEF_MSG=y

# Enable support for NDEF record structures
CONFIG_NFC_NDEF_RECORD=y

# Enable helper macros/functions for text records
CONFIG_NFC_NDEF_TEXT_RECORD=y
```

These options enable both the low-level Type 2 Tag emulation (`nfc_t2t_*` APIs) and high-level NDEF message construction utilities (such as text and URI record encoding).

---

### Step 2: Handle NFC Events

```c
#include <nfc_t2t_lib.h>

static void nfc_event_handler(void *ctx, nfc_t2t_event_t evt,
                              const uint8_t *data, size_t len)
{
    switch (evt) {
    case NFC_T2T_EVENT_FIELD_ON:
        printk("Phone detected\n");
        break;
    case NFC_T2T_EVENT_FIELD_OFF:
        printk("Phone removed\n");
        break;
    default:
        break;
    }
}
```

This callback receives NFC field events.

* `NFC_T2T_EVENT_FIELD_ON`: Indicates that a reader (e.g., a phone) is nearby and the tag is powered.
* `NFC_T2T_EVENT_FIELD_OFF`: The reader has moved away; the field is gone.

Other events like `NFC_T2T_EVENT_DATA_READ` (when a reader completes reading) and `NFC_T2T_EVENT_STOPPED` (when the emulation is explicitly stopped) exist but are optional for basic use. You can handle these for more advanced flows like logging reads or cleaning up.

and later in main we can register this handler with the NFC stack:

```c
int main(void)
{
    int err = nfc_t2t_setup(nfc_event_handler, NULL);

    // More code not shown here...
}
```

Here, the second argument is a user-defined context pointer. It can be used to pass additional data to the event handler if needed. In this case, we set it to `NULL` since we don't need any extra context.

---

### Step 3: Encode NDEF Message

To broadcast data over NFC, we must encode it into an **NDEF (NFC Data Exchange Format)** message. This step shows how to build a basic NDEF message containing a single **Text Record** using Nordic’s NFC library.

#### 0. Include Required Headers

```c
#include <nfc/ndef/msg.h>
#include <nfc/ndef/text_rec.h>
```

#### 1. Define a Text Record

```c
NFC_NDEF_TEXT_RECORD_DESC_DEF(text_rec,
    UTF_8,
    "en", 2,
    "Hello World!", 12);
```

This macro:

* Creates a `nfc_ndef_record_desc` (record descriptor) and its payload.
* Parameters:

  * `UTF_8`: Specifies text encoding.
  * `"en", 2`: Language code ("en" for English) and its length.
  * `"Hello World!", 12`: Text string and its length.

Use `NFC_NDEF_TEXT_RECORD_DESC(text_rec)` to access the descriptor later.

#### 2. Define the NDEF Message

```c
NFC_NDEF_MSG_DEF(ndef_msg, 1);
```

This macro:

* Declares an NDEF message descriptor with capacity for 1 record.
* Creates an internal array to hold pointers to record descriptors.

Use `NFC_NDEF_MSG(ndef_msg)` to refer to the message instance.

#### 3. Add the Record to the Message

```c
nfc_ndef_msg_record_add(&NFC_NDEF_MSG(ndef_msg),
                        &NFC_NDEF_TEXT_RECORD_DESC(text_rec));
```

This function:

* Appends the previously defined text record to the message.
* Returns `0` on success or a negative error code.

#### 4. Encode the Message to a Buffer

```c
nfc_ndef_msg_encode(&NFC_NDEF_MSG(ndef_msg), buf, buf_len);
```

This function:

* Serializes the message into a raw binary format for NFC transmission.
* `buf` is the output buffer; `buf_len` is input/output (available space / encoded length).
* The buffer must be large enough to hold the message (128 bytes is usually enough for simple records).


#### Total Code Snippet

```c
#include <nfc/ndef/msg.h>
#include <nfc/ndef/text_rec.h>

static int create_text_payload(uint8_t *buf, uint32_t *buf_len)
{
	NFC_NDEF_TEXT_RECORD_DESC_DEF(text_rec,
								  UTF_8,
								  "en", 2,
								  "Hello World!", 12);

	NFC_NDEF_MSG_DEF(ndef_msg, 1);

	int ret = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(ndef_msg),
									  &NFC_NDEF_TEXT_RECORD_DESC(text_rec));
	if (ret < 0)
	{
		return ret;
	}

	return nfc_ndef_msg_encode(&NFC_NDEF_MSG(ndef_msg), buf, buf_len);
}
```


---


### Step 4: Register Payload for Read

Once the NDEF message is encoded, it must be registered with the NFC stack so that it can be sent to a reader during a READ operation.

```c
if (nfc_t2t_payload_set(nfc_data_buf, payload_len) < 0) {
    printk("Unable to load NFC data\n");
    goto error;
}
```

* `nfc_t2t_payload_set()` tells the NFC Type 2 Tag library which data to serve to the reader.
* `nfc_data_buf` is a pointer to the encoded NDEF message in RAM.
* `payload_len` is the size of that buffer in bytes.

The function wraps the raw NDEF data with the necessary Type-Length-Value (TLV) headers internally. The pointer must remain valid as long as NFC is active.


---


### Step 5: Start NFC Emulation

After configuring the message and registering the payload, NFC emulation must be activated to begin responding to NFC readers.

```c
if (nfc_t2t_emulation_start() < 0) {
    printk("Emulation start failed\n");
    goto error;
}
```

* `nfc_t2t_emulation_start()` enables the NFC hardware and starts field sensing.
* Once started, events like `FIELD_ON`, `FIELD_OFF`, and `DATA_READ` will be delivered to the user-defined callback.
* Returns `0` on success; returns an error if already started or hardware is busy.

This call marks the final step to make the tag discoverable and readable by external devices.
