/**
 * @file frame.h
 * @brief Frame parsing and building utilities
 * 
 * Provides functions to parse incoming byte streams into frames
 * and build outgoing frames from structured data.
 */

#ifndef FRAME_H
#define FRAME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "protocol/protocol.h"

/*============================================================================*/
/* Types                                                                      */
/*============================================================================*/

/**
 * @brief Frame structure
 */
typedef struct {
    uint8_t     cmd;                            /* Command code */
    uint8_t     payload[PROTOCOL_MAX_PAYLOAD];  /* Payload data */
    uint8_t     payload_len;                    /* Payload length */
} Frame_t;

/**
 * @brief Frame parsing result
 */
typedef enum {
    FRAME_PARSE_OK          = 0,    /* Frame parsed successfully */
    FRAME_PARSE_INCOMPLETE  = 1,    /* More data needed */
    FRAME_PARSE_CRC_ERROR   = 2,    /* CRC verification failed */
    FRAME_PARSE_FORMAT_ERR  = 3,    /* Frame format error */
} FrameParseResult_t;

/*============================================================================*/
/* Functions                                                                  */
/*============================================================================*/

/**
 * @brief Parse a frame from byte buffer
 * @param buffer Input byte buffer
 * @param len Buffer length
 * @param frame Output frame structure
 * @param consumed Number of bytes consumed (output)
 * @return Parse result status
 */
FrameParseResult_t Frame_Parse(const uint8_t* buffer, uint16_t len,
                                Frame_t* frame, uint16_t* consumed);

/**
 * @brief Build a frame into byte buffer
 * @param frame Input frame structure
 * @param buffer Output byte buffer (must be at least PROTOCOL_MAX_PAYLOAD + 5 bytes)
 * @return Number of bytes written
 */
uint16_t Frame_Build(const Frame_t* frame, uint8_t* buffer);

/**
 * @brief Calculate CRC (XOR checksum)
 * @param data Data buffer
 * @param len Data length
 * @return CRC value
 */
uint8_t Frame_CalculateCRC(const uint8_t* data, uint8_t len);

/**
 * @brief Initialize a frame structure
 * @param frame Frame to initialize
 * @param cmd Command code
 */
void Frame_Init(Frame_t* frame, uint8_t cmd);

/**
 * @brief Add a byte to frame payload
 * @param frame Frame structure
 * @param byte Byte to add
 * @return true if added, false if payload full
 */
bool Frame_AddByte(Frame_t* frame, uint8_t byte);

/**
 * @brief Add a 16-bit value to frame payload (big-endian)
 * @param frame Frame structure
 * @param value 16-bit value
 * @return true if added, false if insufficient space
 */
bool Frame_AddU16(Frame_t* frame, uint16_t value);

/**
 * @brief Add a signed 16-bit value to frame payload (big-endian)
 * @param frame Frame structure
 * @param value Signed 16-bit value
 * @return true if added, false if insufficient space
 */
bool Frame_AddS16(Frame_t* frame, int16_t value);

/**
 * @brief Add multiple bytes to frame payload
 * @param frame Frame structure
 * @param data Data to add
 * @param len Number of bytes
 * @return true if added, false if insufficient space
 */
bool Frame_AddBytes(Frame_t* frame, const uint8_t* data, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* FRAME_H */
