#include "lib/lcip.h"

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
((byte) & 0x80 ? '1' : '0'), \
((byte) & 0x40 ? '1' : '0'), \
((byte) & 0x20 ? '1' : '0'), \
((byte) & 0x10 ? '1' : '0'), \
((byte) & 0x08 ? '1' : '0'), \
((byte) & 0x04 ? '1' : '0'), \
((byte) & 0x02 ? '1' : '0'), \
((byte) & 0x01 ? '1' : '0')

int main(void) {
    lcip_device_frame_t device_frame = new_lcip_device_frame(0xABC, 0xDEF);
    print_lcip_device_frame(&device_frame);

    lcip_device_frame_add_tlv(&device_frame, 0x0004, 2, (uint8_t *)(uint16_t[]){0x0320});
    print_lcip_device_frame(&device_frame);

    lcip_device_frame_add_tlv(&device_frame, 0x0005, 2, (uint8_t *) (uint16_t[]){0xFF2E});
    print_lcip_device_frame(&device_frame);

    lcip_device_frame_add_tlv(&device_frame, 0x0006, 1, (uint8_t *) (uint16_t[]){0xAB});
    print_lcip_device_frame(&device_frame);

    device_frame.destination = 0;
    device_frame.tlv_blocks[0] = new_tlv_block(0xDEAD, 4, (uint8_t *) (uint32_t[]){0xBEEF});
    print_lcip_device_frame(&device_frame);

    lcip_device_frame_free(&device_frame);
    return 0;
}
