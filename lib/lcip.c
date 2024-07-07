//
// Created by Leya Wehner on 17.06.2024.
//

#include "lcip.h"
#include <stdlib.h>
#include <stdio.h>

#define CRC16_HD4_POLY 0xBAAD
#define CRC16_HD6_POLY 0xC86C

uint8_t *tlv_blocks_to_bytes(const lcip_device_frame_t *device_frame) {
    uint8_t *tlv_blocks_raw = malloc(device_frame->lenght);
    if (tlv_blocks_raw) {
        uint8_t *ptr = tlv_blocks_raw;
        for (int i = 0; i < device_frame->tlv_block_count; i++) {
            tlv_block_t current_frame = device_frame->tlv_blocks[i];

            *(uint16_t *) ptr = current_frame.type;
            ptr += sizeof(current_frame.type);
            *ptr = current_frame.length;
            ptr += sizeof(current_frame.length);
            for (int j = 0; j < current_frame.length; j++) {
                ptr[j] = current_frame.value[j];
            }
            ptr += current_frame.length;
        }
    }
    return tlv_blocks_raw;
}

uint32_t bytes_to_int(const uint8_t *buffer, const size_t length) {
    uint32_t result = buffer[0];
    for (size_t i = 0; i < length; ++i) {
        result = result | (buffer[i] << (8 * i));
    }
    return result;
}

uint16_t crc16(const uint8_t *data, const size_t size, const uint16_t poly) {
    uint16_t crc = 0xFFFF; // Initial value

    for (size_t i = 0; i < size; i++) {
        crc ^= (uint16_t)data[i] << 8; // Move byte into MSB of crc

        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ poly;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

// Function to verify data with the given checksum
int8_t verify_crc16(const uint8_t *data, const size_t size, const uint16_t received_crc, const uint16_t poly) {
    // Create a buffer to hold data + received CRC
    uint8_t *buffer = malloc(size + sizeof(received_crc));
    if (!buffer) {
        return -1;
    }

    // Copy data to buffer
    for (size_t i = 0; i < size; i++) {
        buffer[i] = data[i];
    }

    // Append the received CRC to the buffer
    buffer[size] = (received_crc >> 8) & 0xFF; // High byte of CRC
    buffer[size + 1] = received_crc & 0xFF;    // Low byte of CRC

    // Recalculate the CRC over the combined data (original data + received CRC)
    const uint16_t calculated_crc = crc16(buffer, size + 2, poly);

    // Free the buffer
    free(buffer);

    // Check if the recalculated CRC is zero
    return calculated_crc == 0;
}

int8_t verify_crc16_hd4(const uint8_t *data, const size_t size, const uint16_t received_crc) {
    return verify_crc16(data, size, received_crc, CRC16_HD4_POLY);
}

int8_t verify_crc16_hd6(const uint8_t *data, const size_t size, const uint16_t received_crc) {
    return verify_crc16(data, size, received_crc, CRC16_HD6_POLY);
}

uint16_t crc16_hd4(const uint8_t *data, const size_t size) {
    return crc16(data, size, CRC16_HD4_POLY);
}

uint16_t crc16_hd6(const uint8_t *data, const size_t size) {
    return crc16(data, size, CRC16_HD6_POLY);
}

void calculate_lcip_device_frame_crc(lcip_device_frame_t *device_frame) {
    const uint8_t *header_raw = (uint8_t *)(uint16_t[]){device_frame->destination, device_frame->ctrl_satus, device_frame->lenght};
    device_frame->df_crc = crc16_hd6(header_raw, sizeof(device_frame->destination) + sizeof(device_frame->ctrl_satus) + sizeof(device_frame->lenght));

    uint8_t *tlv_blocks_raw = tlv_blocks_to_bytes(device_frame);

    device_frame->d_crc = crc16_hd4(tlv_blocks_raw, device_frame->lenght);
    free(tlv_blocks_raw);
}

tlv_block_t new_tlv_block(const uint16_t type, const uint8_t length, uint8_t *value) {
    tlv_block_t tlv;
    tlv.type = type;
    tlv.length = length;
    tlv.value = value;

    return tlv;
}

lcip_device_frame_t new_lcip_device_frame(const uint16_t destination, const uint16_t ctrl_status) {
    lcip_device_frame_t device_frame = {
        .destination = destination,
        .ctrl_satus = ctrl_status,
        .lenght = 0,
        .tlv_block_count = 0,
        .tlv_blocks = NULL,
    };
    calculate_lcip_device_frame_crc(&device_frame);
    return device_frame;
}

void lcip_device_frame_free(lcip_device_frame_t *device_frame) {
    free(device_frame->tlv_blocks);
    device_frame->lenght = 0;
    device_frame->tlv_block_count = 0;
}

int8_t lcip_device_frame_add_tlv(lcip_device_frame_t *device_frame, const uint16_t type, const uint8_t length, uint8_t *value) {
    size_t new_count = device_frame->tlv_block_count + 1;
    tlv_block_t *temp = realloc(device_frame->tlv_blocks, sizeof(tlv_block_t) * new_count);

    if (temp) {
        device_frame->tlv_blocks = temp;
        tlv_block_t tlv_block = new_tlv_block(type, length, value);
        device_frame->tlv_blocks[device_frame->tlv_block_count] = tlv_block;
        device_frame->tlv_block_count = new_count;
        device_frame->lenght += sizeof(type) + sizeof(length) + length;
        calculate_lcip_device_frame_crc(device_frame);
        return 0;
    }

    return -1;
}

uint8_t *serialize_uint8(uint8_t *buffer, const uint8_t value) {
    buffer[0] = value;

    return buffer + 1;
}

uint8_t *serialize_uint16(uint8_t *buffer, const uint16_t value) {
    buffer[0] = value >> 8;
    buffer[1] = value;

    return buffer + 2;
}

uint8_t *serialize_uint32(uint8_t *buffer, const uint32_t value) {
    buffer[0] = value >> 24;
    buffer[1] = value >> 16;
    buffer[2] = value >> 8;
    buffer[3] = value;

    return buffer + 4;
}

uint32_t tlv_block_get_value(const tlv_block_t *tlv_block) {
   return bytes_to_int(tlv_block->value, tlv_block->length);
}

/*uint8_t *serialize_tlv_data(uint8_t *buffer, const uint8_t *value, const uint8_t len) {
    for (int i = 0; i < len; i++) {
        buffer[i] = value[i];
    }

    return buffer + len;
}

uint8_t *serialize_tlv_block(uint8_t *buffer, const tlv_block_t *tlv_block) {
    buffer = serialize_uint16(buffer, tlv_block->type);
    buffer = serialize_uint8(buffer, tlv_block->length);
    buffer = serialize_tlv_data(buffer, tlv_block->value, tlv_block->length);

    return buffer;
}*/

void print_lcip_device_frame_header_raw(const lcip_device_frame_t *device_frame) {
    const uint8_t *header = (uint8_t *)(uint16_t[]){device_frame->destination, device_frame->ctrl_satus, device_frame->lenght};
    const size_t header_size = sizeof(device_frame->destination) + sizeof(device_frame->ctrl_satus) + sizeof(device_frame->lenght);
    printf("Header Raw:\n\t");
    for (int i = 0; i < header_size; i++) {
        printf("0x%02X ", *(header + i));
    }
    printf("\n\tdfCRC: 0x%04X\n", device_frame->df_crc);
    printf("\tdfCRC check %s!\n", verify_crc16_hd6(header, header_size, device_frame->df_crc) ? "succesful" : "failed");
}

void print_lcip_device_frame_tlvs_raw(const lcip_device_frame_t *device_frame) {
    uint8_t *tlv_blocks_raw = tlv_blocks_to_bytes(device_frame);
    printf("Frame TLVs Raw:\n\t");
    if (!device_frame->lenght) printf("<EMPTY>");
    else {

        for (int i = 0; i < device_frame->lenght; i++) {
            printf("0x%02X ", tlv_blocks_raw[i]);
        }
    }
    printf("\n\tdCRC: 0x%04X\n", device_frame->d_crc);
    printf("\tdCRC check %s!\n", verify_crc16_hd4(tlv_blocks_raw, device_frame->lenght, device_frame->d_crc) ? "succesful" : "failed");
    free(tlv_blocks_raw);
}

void print_lcip_device_frame_tlvs(const lcip_device_frame_t *device_frame) {
    printf("Frame TLVs:\n");
    if (!device_frame->tlv_block_count) {
        puts("\t<EMPTY>");
    } else {
        for (int i = 0; i < device_frame->tlv_block_count; i++) {
            tlv_block_t current = device_frame->tlv_blocks[i];
            printf("\tT: 0x%04X; L: %d; V: 0x%04X\n", current.type, current.length, tlv_block_get_value(&current));
        }
    }
}

void print_lcip_device_frame(const lcip_device_frame_t *device_frame) {
    puts("Header Data:");
    printf("\tDestination: 0x%04X, CTRL_STATUS: 0x%04X, Length: %d\n", device_frame->destination, device_frame->ctrl_satus, device_frame->lenght);
    print_lcip_device_frame_header_raw(device_frame);
    print_lcip_device_frame_tlvs(device_frame);
    print_lcip_device_frame_tlvs_raw(device_frame);
    puts("");
}