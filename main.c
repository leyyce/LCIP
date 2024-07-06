#include <stdio.h>
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

void print_frame_header_raw(const lcip_device_frame_t *device_frame) {
    const uint8_t *header = (uint16_t[]) {device_frame->destination, device_frame->ctrl_satus, device_frame->lenght};
    const size_t header_size = sizeof(device_frame->destination) + sizeof(device_frame->ctrl_satus) + sizeof(device_frame->lenght);
    printf("Header Raw:\n\t");
    for (int i = 0; i < header_size; i++) {
        printf("0x%02X ", *(header + i));
    }
    printf("\n\tdfCRC: 0x%04X [%d]\n", device_frame->df_crc, device_frame->df_crc);
    printf("\tdfCRC check %s!\n", verify_crc16_hd6(header, header_size, device_frame->df_crc) ? "succesful" : "failed");
}

void print_frame_tlvs_raw(const lcip_device_frame_t *device_frame) {
    printf("Frame TLVs Raw:\n\t");
    for (int i = 0; i < device_frame->lenght; i++) {
        printf("0x%02X ", *(device_frame->tlv_frames + i));
    }
    if (!device_frame->lenght) printf("<EMPTY>");
    printf("\n\tdCRC: 0x%04X [%d]\n", device_frame->d_crc, device_frame->d_crc);
    printf("\tdCRC check %s!\n", verify_crc16_hd4(device_frame->tlv_frames, device_frame->lenght, device_frame->d_crc) ? "succesful" : "failed");
}

void print_frame_tlvs(const lcip_device_frame_t *device_frame) {
    printf("Frame TLVs:\n");
    for (int i = 0; i < device_frame->lenght;) {
        const uint16_t t = bytes_to_int(device_frame->tlv_frames + i, 2);
        i += 2;
        const uint8_t l = bytes_to_int(device_frame->tlv_frames + i, 1);
        i+= 1;
        const uint32_t v = bytes_to_int(device_frame->tlv_frames + i, l);
        i+= l;

        printf("\tT: 0x%04X, L: %d, V: 0x%04X\n", t, l, v);
    }
    if (!device_frame->lenght) puts("\t<EMPTY>");
}

void print_frame(const lcip_device_frame_t *device_frame) {
    puts("Header Data:");
    printf("\tDestination: 0x%04X, CTRL_STATUS: 0x%04X, Length: %d\n", device_frame->destination, device_frame->ctrl_satus, device_frame->lenght);
    print_frame_header_raw(device_frame);
    print_frame_tlvs(device_frame);
    print_frame_tlvs_raw(device_frame);
    puts("");
}

int main(void) {
    lcip_device_frame_t device_frame = new_lcip_device_frame(123, 456);
    print_frame(&device_frame);

    lcip_device_frame_add_tlv(&device_frame, 0x0004, 2, (uint16_t[]){0x0320});
    device_frame.destination = 0;
    print_frame(&device_frame);

    lcip_device_frame_add_tlv(&device_frame, 0x0005, 2, (uint16_t[]){0xFF2E});
    print_frame(&device_frame);

    lcip_device_frame_add_tlv(&device_frame, 0x0006, 1, (uint16_t[]){0xAB});
    print_frame(&device_frame);

    lcip_device_frame_delete(&device_frame);
    return 0;
}
