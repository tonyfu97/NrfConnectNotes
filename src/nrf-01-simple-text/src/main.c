#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

#include <nfc_t2t_lib.h>
#include <nfc/ndef/msg.h>
#include <nfc/ndef/text_rec.h>

#define NFC_BUFFER_SIZE 128

static uint8_t nfc_data_buf[NFC_BUFFER_SIZE];

/* Callback to handle NFC field events */
static void nfc_event_handler(void *ctx, nfc_t2t_event_t evt,
							  const uint8_t *data, size_t len)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(data);
	ARG_UNUSED(len);

	switch (evt)
	{
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

/* Generate a basic NDEF text message */
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

int main(void)
{
	uint32_t payload_len = sizeof(nfc_data_buf);

	printk("Starting minimal NFC demo\n");

	if (nfc_t2t_setup(nfc_event_handler, NULL) < 0)
	{
		printk("Failed to init NFC interface\n");
		goto error;
	}

	if (create_text_payload(nfc_data_buf, &payload_len) < 0)
	{
		printk("Payload creation failed\n");
		goto error;
	}

	if (nfc_t2t_payload_set(nfc_data_buf, payload_len) < 0)
	{
		printk("Unable to load NFC data\n");
		goto error;
	}

	if (nfc_t2t_emulation_start() < 0)
	{
		printk("Emulation start failed\n");
		goto error;
	}

	return 0;

error:
#if CONFIG_REBOOT
	sys_reboot(SYS_REBOOT_COLD);
#endif
	return -EIO;
}
