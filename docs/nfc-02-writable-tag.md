# NFC 02: Writable Tag

**Author:** Tony Fu  
**Date:** 2025/05/04   
**Device:** nRF52840 DK  
**Toolchain:** nRF Connect SDK v3.0.0  

---

## NFC Tag Type Comparison (Type 2 vs Type 4)

In this example, we will use our phone to write data to the NFC tag. We need to use type 4 tag for this purpose. The nRF Connect SDK allows us to use both type 2 and type 4 tags. The table below summarizes the differences between the two types of tags:

| Feature                    | **Type 2 Tag**                                      | **Type 4 Tag**                                       |
| -------------------------- | --------------------------------------------------- | ---------------------------------------------------- |
| **Writability by phone**   | ❌ *Not supported (in Nordic SDK)*                   | ✅ *Fully supported*                                  |
| **Read support**           | ✅ Yes                                               | ✅ Yes                                                |
| **Data update method**     | Static; updated via firmware or internal logic only | Dynamic; data written via NFC (e.g., from phone)     |
| **Storage backend**        | No file system (raw TLV structure in RAM)           | NDEF file with internal length and control structure |
| **File system support**    | ❌ None                                              | ✅ Uses NDEF file abstraction                         |
| **Authentication**         | ❌ Not supported                                     | ✅ Optional (ISO 7816-style command sets)             |
| **Tag activation**         | Type 2 Tag protocol                                 | ISO/IEC 14443-4 (APDU protocol)                      |
| **Library used (nRF SDK)** | `nfc_t2t_lib`                                       | `nfc_t4t_lib`                                        |
| **Use case**               | Simple, static read-only tags (e.g., fixed URLs)    | Smart cards, writable business cards, secure access  |


---


## In Action

This example demonstrates how to create and emulate a writable NFC Type 4 Tag using the nRF Connect SDK. It builds a simple NDEF text message with the payload `"42"`, sets up an emulation buffer, starts NFC communication, and handles basic tag events such as read, write, and field detection.

---

### Step 1: Configure the NFC Stack

Just like the previous example, we need to enable the NFC stack in the `prj.conf` file. The only difference is that we need to enable the `CONFIG_NFC_T4T_NRFXLIB` option to use the type 4 tag.

```kconfig
# NFC Type 4 Support
CONFIG_NFC_T4T_NRFXLIB=y

# NFC NDEF (High-Level Message Encoding)
CONFIG_NFC_NDEF=y
CONFIG_NFC_NDEF_MSG=y
CONFIG_NFC_NDEF_RECORD=y
CONFIG_NFC_NDEF_TEXT_RECORD=y
```

---

### Step 2: Handle NFC Events

Just like the previous example, we need to handle NFC events in the main loop

```c
#include <nfc_t4t_lib.h>

static void tag_event_handler(void *ctx, nfc_t4t_event_t evt,
							  const uint8_t *data, size_t len, uint32_t flags)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(data);
	ARG_UNUSED(flags);

	switch (evt)
	{
	case NFC_T4T_EVENT_FIELD_ON:
		printk("Reader present\n");
		break;
	case NFC_T4T_EVENT_FIELD_OFF:
		printk("Reader removed\n");
		break;
	case NFC_T4T_EVENT_NDEF_READ:
		printk("Message read\n");
		break;
	case NFC_T4T_EVENT_NDEF_UPDATED:
		printk("Message updated (%d bytes)\n", len);
		break;
	default:
		break;
	}
}
```

and later in main we can register this handler with the NFC stack:

```c
int main(void)
{
	// More code not shown here...
	if (nfc_t4t_setup(tag_event_handler, NULL) < 0)
	{
		printk("Error: NFC setup failed\n");
		return -1;
	}
	// More code not shown here...
}
```

---

### Step 3: Encode NDEF Message

Next, we encode the NDEF message. The example below creates a text record with the payload `"42"` and language code `"en"`:

```c
static int create_text_payload(uint8_t *buf, uint32_t *len)
{
	/* Define a text record with language code "en" and payload "42" */
	NFC_NDEF_TEXT_RECORD_DESC_DEF(txt_rec,
			UTF_8,
			"en", 2,
			"42", 2);

	NFC_NDEF_MSG_DEF(msg, 1);

	int err = nfc_ndef_msg_record_add(&NFC_NDEF_MSG(msg),
			&NFC_NDEF_TEXT_RECORD_DESC(txt_rec));
	if (err < 0)
	{
		return err;
	}

	uint32_t used = *len;
	err = nfc_ndef_msg_encode(&NFC_NDEF_MSG(msg),
			nfc_t4t_ndef_file_msg_get(buf),
			&used);
	if (err < 0)
	{
		return err;
	}

	err = nfc_t4t_ndef_file_encode(buf, &used);
	if (err < 0)
	{
		return err;
	}

	*len = used;
	return 0;
}
```

This is similar to the earlier example using T2T, but here in additionally we use `nfc_t4t_ndef_file_msg_get` to retrieve the buffer where the encoded NDEF message should be stored. This function provides access to the internal buffer structure used by T4T formatting.

The final step uses `nfc_t4t_ndef_file_encode`, which wraps the encoded message in the full T4T NDEF file format. This is required because a Type 4 Tag expects the NDEF data to follow a specific structure.

* `file_buf`: A pointer to the output buffer that will hold the fully formatted NDEF file.
* `size`: A pointer to a variable that initially holds the buffer size and returns the actual size used after encoding.

This function is essential to ensure the message conforms to the T4T specification and can be read by NFC readers that support this format.

Later on, we will call this function to encode the NDEF message and write it to the NFC tag:

```c
#define NFC_MEM_SIZE 256
static uint8_t nfc_mem[NFC_MEM_SIZE];

main()
{
	uint32_t ndef_len = NFC_MEM_SIZE;
	if (create_text_payload(nfc_mem, &ndef_len) < 0)
	{
		printk("Error: failed to build initial message\n");
		return -1;
	}

	if (nfc_t4t_setup(tag_event_handler, NULL) < 0)
	{
		printk("Error: NFC setup failed\n");
		return -1;
	}

	// More code not shown here...
}
```

---

### Step 4: Set Emulation Buffer and Start NFC

Before starting NFC communication, we must configure the buffer that holds the NDEF data and then activate the NFC emulation.

```c
if (nfc_t4t_ndef_rwpayload_set(nfc_mem, NFC_MEM_SIZE) < 0)
{
	printk("Error: cannot set buffer\n");
	return -1;
}

if (nfc_t4t_emulation_start() < 0)
{
	printk("Error: emulation failed\n");
	return -1;
}
```

The first function, `nfc_t4t_ndef_rwpayload_set`, sets the buffer used for emulating a read/write NDEF tag. This buffer must remain accessible for as long as the emulation is active. It will be updated if an external NFC reader modifies the content. You must ensure the buffer length is consistent across updates, and it should not be the same memory currently in use.

* `nfc_mem`: A pointer to the buffer that stores the NDEF content.
* `NFC_MEM_SIZE`: The size of this buffer in bytes.

The second function, `nfc_t4t_emulation_start`, activates the NFC frontend. This must be called after the payload buffer is set. Once started, NFC events (like reads or writes) will be handled by the NFC stack and delivered to the application callback.

These calls are essential to make the device appear as a writable NFC Type 4 Tag to external readers.

In the end, the main function looks like this:

```c
int main(void)
{
	printk("NFC tag init\n");

	uint32_t ndef_len = NFC_MEM_SIZE;
	if (create_text_payload(nfc_mem, &ndef_len) < 0)
	{
		printk("Error: failed to build initial message\n");
		return -1;
	}

	if (nfc_t4t_setup(tag_event_handler, NULL) < 0)
	{
		printk("Error: NFC setup failed\n");
		return -1;
	}

	if (nfc_t4t_ndef_rwpayload_set(nfc_mem, NFC_MEM_SIZE) < 0)
	{
		printk("Error: cannot set buffer\n");
		return -1;
	}

	if (nfc_t4t_emulation_start() < 0)
	{
		printk("Error: emulation failed\n");
		return -1;
	}

	printk("Tag is writable. Waiting...\n");
	while (1)
	{
		k_sleep(K_FOREVER);
	}
}
```
