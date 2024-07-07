//
// Created by Leya Wehner on 17.06.2024.
//

#ifndef LCIP_H
#define LCIP_H

#include <stddef.h>
#include <stdint.h>

typedef struct proto_header proto_header_t;
typedef struct lcip_frame lcip_frame_t;
typedef struct lcip_device_frame lcip_device_frame_t;
typedef struct tlv_block tlv_block_t;

struct proto_header {
    uint16_t id:8;
    uint16_t version:8;
    lcip_frame_t *lcip_frame;
    uint32_t crc;
};

struct lcip_frame {
    uint8_t msg_count;
    uint8_t group_id;
    uint16_t source_id;
    uint8_t msg_type;
    uint16_t df_size;
    uint8_t h_crc;
    uint8_t *lcip_device_frames;
    uint8_t *padding;
};

struct lcip_device_frame {
    uint16_t destination;
    uint16_t ctrl_satus;
    uint16_t lenght;
    uint16_t df_crc;
    size_t tlv_block_count;
    tlv_block_t *tlv_blocks;
    uint16_t d_crc;
};

struct tlv_block {
    uint16_t type;
    uint8_t length;
    uint8_t *value;
};

tlv_block_t new_tlv_block(const uint16_t type,const  uint8_t length, uint8_t *value);
uint32_t tlv_block_get_value(const tlv_block_t *tlv_block);

int8_t verify_crc16_hd4(const uint8_t *data, const size_t size, const uint16_t received_crc);
int8_t verify_crc16_hd6(const uint8_t *data, const size_t size, const uint16_t received_crc);

lcip_device_frame_t new_lcip_device_frame(const uint16_t destination, const uint16_t ctrl_status);
void lcip_device_frame_free(lcip_device_frame_t *device_frame);
int8_t lcip_device_frame_add_tlv(lcip_device_frame_t *device_frame, const uint16_t type, const uint8_t length, uint8_t *value);
// size_t lcip_device_frame_get_tlv(const lcip_device_frame_t *device_frame, const size_t offset, tlv_block_t *tlv_block);
void print_lcip_device_frame(const lcip_device_frame_t *device_frame);

uint8_t *serialize_uint8(uint8_t *buffer, uint8_t value);
uint8_t *serialize_uint16(uint8_t *buffer, uint16_t value);
uint8_t *serialize_uint32(uint8_t *buffer, uint32_t value);

uint32_t bytes_to_int(const uint8_t *buffer, const size_t length);
// uint8_t *serialize_tlv_data(uint8_t *buffer, uint8_t *value);
// uint8_t *serialize_tlv_block(uint8_t *buffer, const tlv_block_t *tlv_block);

#endif //LCIP_H
