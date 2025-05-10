#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

#include <nfc_t4t_lib.h>
#include <nfc/t4t/ndef_file.h>
#include <nfc/ndef/msg.h>
#include <nfc/ndef/text_rec.h>

#include <string.h>

#define NFC_MEM_SIZE 256
static uint8_t nfc_mem[NFC_MEM_SIZE];

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
