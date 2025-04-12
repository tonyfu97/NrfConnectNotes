#ifndef MY_SERVICE_H_
#define MY_SERVICE_H_

#include <zephyr/types.h>
#include <zephyr/bluetooth/uuid.h>

/* Custom 128-bit UUIDs */
#define BT_UUID_MY_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x9abc, 0xdef0, 0x1234, 0x56789abcdef0)

#define BT_UUID_MY_CHAR_READ_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x9abc, 0xdef0, 0x1234, 0x56789abcdef1)

#define BT_UUID_MY_CHAR_WRITE_VAL \
    BT_UUID_128_ENCODE(0x12345678, 0x9abc, 0xdef0, 0x1234, 0x56789abcdef2)

#define BT_UUID_MY_SERVICE BT_UUID_DECLARE_128(BT_UUID_MY_SERVICE_VAL)
#define BT_UUID_MY_CHAR_READ BT_UUID_DECLARE_128(BT_UUID_MY_CHAR_READ_VAL)
#define BT_UUID_MY_CHAR_WRITE BT_UUID_DECLARE_128(BT_UUID_MY_CHAR_WRITE_VAL)

/* Optional callback for when value is written */
typedef void (*my_write_cb_t)(uint8_t new_value);

struct my_service_cb
{
    my_write_cb_t on_write;
};

/* Initialize the service with optional callback */
int my_service_init(struct my_service_cb *cb);

#endif /* MY_SERVICE_H_ */
