/**
 * @file frame.c
 * @brief Frame parsing and building implementation
 */

#include "protocol/frame.h"
#include <string.h>

/*============================================================================*/
/* Frame Format Constants                                                     */
/*============================================================================*/

#define FRAME_OVERHEAD      4   /* STX + LEN + CRC + ETX */
#define FRAME_MIN_SIZE      4   /* Minimum valid frame: STX + LEN(0) + CRC + ETX */

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
    
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
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
