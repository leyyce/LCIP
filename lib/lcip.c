//
// Created by Leya Wehner on 17.06.2024.
//

#include "lcip.h"
#include <stdlib.h>

#define CRC16_HD4_POLY 0xBAAD
#define CRC16_HD6_POLY 0xC86C


/*tlv_block_t new_tlv_block(const uint16_t type, const uint8_t length, const uint8_t *value) {
    tlv_block_t tlv;
    tlv.type = type;
    tlv.length = length;

    tlv.value = malloc(length);
    for (int i = 0; i < length; i++) {
        tlv.value[i] = value[i];
    }

    return tlv;
}*/

uint16_t _crc16(const uint8_t *data, const size_t size, const uint16_t poly) {
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
uint8_t verify_crc16(const uint8_t *data, const size_t size, const uint16_t received_crc, const uint16_t poly) {
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
    const uint16_t calculated_crc = _crc16(buffer, size + 2, poly);

    // Free the buffer
    free(buffer);

    // Check if the recalculated CRC is zero
    return calculated_crc == 0;
}

uint8_t verify_crc16_hd4(const uint8_t *data, const size_t size, const uint16_t received_crc) {
    return verify_crc16(data, size, received_crc, CRC16_HD4_POLY);
}

uint8_t verify_crc16_hd6(const uint8_t *data, const size_t size, const uint16_t received_crc) {
    return verify_crc16(data, size, received_crc, CRC16_HD6_POLY);
}

uint16_t _crc16_hd4(const uint8_t *data, const size_t size) {
    return _crc16(data, size, CRC16_HD4_POLY);
}

uint16_t _crc16_hd6(const uint8_t *data, const size_t size) {
    return _crc16(data, size, CRC16_HD6_POLY);
}

void _calculate_lcip_device_frame_crc(lcip_device_frame_t *device_frame) {
    const uint8_t *header_raw = (uint16_t[]) {device_frame->destination, device_frame->ctrl_satus, device_frame->lenght};
    device_frame->df_crc = _crc16_hd6(header_raw, sizeof(device_frame->destination) + sizeof(device_frame->ctrl_satus) + sizeof(device_frame->lenght));
    device_frame->d_crc = _crc16_hd4(device_frame->tlv_frames, device_frame->lenght);
}

lcip_device_frame_t new_lcip_device_frame(const uint16_t destionation, const uint16_t ctrl_status) {
    lcip_device_frame_t device_frame = {
        .destination = destionation,
        .ctrl_satus = ctrl_status,
        .lenght = 0,
        .tlv_frames = NULL,
    };
    _calculate_lcip_device_frame_crc(&device_frame);
    return device_frame;
}

void lcip_device_frame_delete(const lcip_device_frame_t *device_frame) {
    free(device_frame->tlv_frames);
}

int lcip_device_frame_add_tlv(lcip_device_frame_t *device_frame, const uint16_t type, const uint8_t length, const uint8_t *value) {
    const uint16_t new_len = device_frame->lenght + sizeof(type) + sizeof(length) + length;
    uint8_t *temp = realloc(device_frame->tlv_frames, new_len);
    if (temp) {
        device_frame->tlv_frames = temp;
        temp += device_frame->lenght;
        device_frame->lenght = new_len;

        *(uint16_t *) temp = type;
        temp += sizeof(type);
        *temp = length;
        temp += sizeof(length);
        for (int i = 0; i < length; i++) {
            temp[i] = value[i];
        }
        _calculate_lcip_device_frame_crc(device_frame);
        return 0;
    }

    return -1;
}

void tlv_block_delete(const tlv_block_t *tlv_block) {
    free(tlv_block->value);
}

size_t lcip_device_frame_get_tlv(const lcip_device_frame_t *device_frame, const size_t offset, tlv_block_t *tlv_block) {
    if (device_frame->lenght <= 0) return -1;

    uint8_t *start = device_frame->tlv_frames + offset;
    const uint16_t t = bytes_to_int(start, 2);
    const uint8_t l = bytes_to_int(start += sizeof(tlv_block->type), 1);
    tlv_block->type = t;
    tlv_block->length = l;

    tlv_block->value = malloc(l);
    start += sizeof(tlv_block->length);
    for (int i = 0; i < l; i++) {
        tlv_block->value[i] = start[i];
    }

    return start+l - device_frame->tlv_frames;
}

/*uint16_t crc16_hd6_2(const uint8_t *data, size_t size) {
    uint16_t out = 0;
    int bits_read = 0, bit_flag;

    /* Sanity check: #1#
    if(data == NULL)
        return 0;

    while(size > 0)
    {
        bit_flag = out >> 15;

        /* Get next bit: #1#
        out <<= 1;
        out |= (*data >> bits_read) & 1; // item a) work from the least significant bits

        /* Increment bit counter: #1#
        bits_read++;
        if(bits_read > 7)
        {
            bits_read = 0;
            data++;
            size--;
        }

        /* Cycle check: #1#
        if(bit_flag)
            out ^= CRC16_HD6_POLY;

    }

    // "push out" the last 16 bits
    int i;
    for (i = 0; i < 16; ++i) {
        bit_flag = out >> 15;
        out <<= 1;
        if(bit_flag)
            out ^= CRC16_HD6_POLY;
    }

    // reverse the bits
    uint16_t crc = 0;
    i = 0x8000;
    int j = 0x0001;
    for (; i != 0; i >>=1, j <<= 1) {
        if (i & out) crc |= j;
    }

    return crc;
}*/

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

uint32_t bytes_to_int(const uint8_t *buffer, const size_t length) {
    uint32_t result = buffer[0];
    for (size_t i = 0; i < length; ++i) {
        result = result | (buffer[i] << (8 * i));
    }
    return result;
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
