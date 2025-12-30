/**
 * @file frame.c
 * @brief Frame parsing and building implementation
 *
 * Uses CRC-8 CCITT (polynomial 0x07) for improved error detection.
 */

#include "protocol/frame.h"
#include <string.h>

/*============================================================================*/
/* Frame Format Constants                                                     */
/*============================================================================*/

#define FRAME_OVERHEAD      4   /* STX + LEN + CRC + ETX */
#define FRAME_MIN_SIZE      4   /* Minimum valid frame: STX + LEN(0) + CRC + ETX */

/*============================================================================*/
/* CRC-8 CCITT Lookup Table (Polynomial 0x07)                                 */
/*============================================================================*/

/**
 * @brief CRC-8 CCITT lookup table
 *
 * Generated with polynomial 0x07 (x^8 + x^2 + x + 1)
 * Initial value: 0x00
 * This provides much better error detection than XOR checksum:
 * - Detects all single-bit errors
 * - Detects all burst errors up to 8 bits
 * - Detects most multi-bit errors
 */
static const uint8_t crc8_table[256] = {
    0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
    0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
    0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
    0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
    0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5,
    0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
    0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85,
    0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
    0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
    0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
    0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2,
    0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
    0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32,
    0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
    0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
    0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
    0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C,
    0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
    0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC,
    0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
    0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
    0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
    0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C,
    0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
    0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B,
    0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
    0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
    0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
    0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB,
    0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
    0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB,
    0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
};

/*============================================================================*/
/* Public Functions                                                           */
/*============================================================================*/

FrameParseResult_t Frame_Parse(const uint8_t* buffer, uint16_t len,
                                Frame_t* frame, uint16_t* consumed)
{
    if (buffer == NULL || frame == NULL || consumed == NULL) {
        return FRAME_PARSE_FORMAT_ERR;
    }
    
    *consumed = 0;
    
    /* Search for STX */
    uint16_t start_idx = 0;
    while (start_idx < len && buffer[start_idx] != PROTOCOL_STX) {
        start_idx++;
    }
    
    /* Discard bytes before STX */
    *consumed = start_idx;
    
    if (start_idx >= len) {
        return FRAME_PARSE_INCOMPLETE;
    }
    
    /* Need at least STX + LEN to continue */
    if (len - start_idx < 2) {
        return FRAME_PARSE_INCOMPLETE;
    }
    
    /* Get payload length */
    uint8_t payload_len = buffer[start_idx + 1];
    
    /* Validate payload length */
    if (payload_len > PROTOCOL_MAX_PAYLOAD) {
        /* Invalid length, skip this STX and try again */
        *consumed = start_idx + 1;
        return FRAME_PARSE_FORMAT_ERR;
    }
    
    /* Calculate expected frame size: STX + LEN + CMD + PAYLOAD + CRC + ETX */
    uint16_t expected_size = 1 + 1 + 1 + payload_len + 1 + 1;
    
    /* Check if we have complete frame */
    if (len - start_idx < expected_size) {
        return FRAME_PARSE_INCOMPLETE;
    }
    
    /* Verify ETX */
    uint16_t etx_idx = start_idx + expected_size - 1;
    if (buffer[etx_idx] != PROTOCOL_ETX) {
        /* Invalid frame, skip this STX */
        *consumed = start_idx + 1;
        return FRAME_PARSE_FORMAT_ERR;
    }
    
    /* Calculate and verify CRC */
    /* CRC covers: LEN + CMD + PAYLOAD */
    uint8_t crc_data_len = 1 + 1 + payload_len;  /* LEN + CMD + PAYLOAD */
    uint8_t calc_crc = Frame_CalculateCRC(&buffer[start_idx + 1], crc_data_len);
    uint8_t recv_crc = buffer[etx_idx - 1];
    
    if (calc_crc != recv_crc) {
        *consumed = start_idx + expected_size;
        return FRAME_PARSE_CRC_ERROR;
    }
    
    /* Parse frame data */
    frame->cmd = buffer[start_idx + 2];
    frame->payload_len = payload_len;
    
    if (payload_len > 0) {
        memcpy(frame->payload, &buffer[start_idx + 3], payload_len);
    }
    
    *consumed = start_idx + expected_size;
    return FRAME_PARSE_OK;
}

uint16_t Frame_Build(const Frame_t* frame, uint8_t* buffer)
{
    if (frame == NULL || buffer == NULL) {
        return 0;
    }
    
    uint16_t idx = 0;
    
    /* STX */
    buffer[idx++] = PROTOCOL_STX;
    
    /* LEN (includes CMD in some interpretations, here it's just payload) */
    /* Based on architecture doc: LEN = payload length only */
    buffer[idx++] = frame->payload_len;
    
    /* CMD */
    buffer[idx++] = frame->cmd;
    
    /* PAYLOAD */
    if (frame->payload_len > 0) {
        memcpy(&buffer[idx], frame->payload, frame->payload_len);
        idx += frame->payload_len;
    }
    
    /* CRC: covers LEN + CMD + PAYLOAD */
    uint8_t crc_start = 1;  /* After STX */
    uint8_t crc_len = 1 + 1 + frame->payload_len;  /* LEN + CMD + PAYLOAD */
    buffer[idx++] = Frame_CalculateCRC(&buffer[crc_start], crc_len);
    
    /* ETX */
    buffer[idx++] = PROTOCOL_ETX;
    
    return idx;
}

uint8_t Frame_CalculateCRC(const uint8_t* data, uint8_t len)
{
    if (data == NULL || len == 0) {
        return 0;
    }

    uint8_t crc = 0x00;  /* CRC-8 CCITT initial value */
    for (uint8_t i = 0; i < len; i++) {
        crc = crc8_table[crc ^ data[i]];
    }
    return crc;
}

void Frame_Init(Frame_t* frame, uint8_t cmd)
{
    if (frame != NULL) {
        frame->cmd = cmd;
        frame->payload_len = 0;
        memset(frame->payload, 0, PROTOCOL_MAX_PAYLOAD);
    }
}

bool Frame_AddByte(Frame_t* frame, uint8_t byte)
{
    if (frame == NULL || frame->payload_len >= PROTOCOL_MAX_PAYLOAD) {
        return false;
    }
    
    frame->payload[frame->payload_len++] = byte;
    return true;
}

bool Frame_AddU16(Frame_t* frame, uint16_t value)
{
    if (frame == NULL || frame->payload_len + 2 > PROTOCOL_MAX_PAYLOAD) {
        return false;
    }
    
    /* Big-endian */
    frame->payload[frame->payload_len++] = (uint8_t)(value >> 8);
    frame->payload[frame->payload_len++] = (uint8_t)(value & 0xFF);
    return true;
}

bool Frame_AddS16(Frame_t* frame, int16_t value)
{
    return Frame_AddU16(frame, (uint16_t)value);
}

bool Frame_AddBytes(Frame_t* frame, const uint8_t* data, uint8_t len)
{
    if (frame == NULL || data == NULL) {
        return false;
    }
    
    if (frame->payload_len + len > PROTOCOL_MAX_PAYLOAD) {
        return false;
    }
    
    memcpy(&frame->payload[frame->payload_len], data, len);
    frame->payload_len += len;
    return true;
}
