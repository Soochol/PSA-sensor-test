/**
 * @file commands.h
 * @brief Command handler and dispatcher
 * 
 * Provides command processing and response generation for all
 * supported protocol commands.
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "protocol/frame.h"

/*============================================================================*/
/* Types                                                                      */
/*============================================================================*/

/**
 * @brief Command handler function type
 * @param request Received frame
 * @param response Response frame (output)
 */
typedef void (*CommandHandler_t)(const Frame_t* request, Frame_t* response);

/*============================================================================*/
/* Functions                                                                  */
/*============================================================================*/

/**
 * @brief Initialize command handlers
 */
void Commands_Init(void);

/**
 * @brief Process a received frame and generate response
 * @param request Received frame
 * @param response Response frame (output)
 * @return true if response should be sent, false otherwise
 */
bool Commands_Process(const Frame_t* request, Frame_t* response);

/**
 * @brief Build NAK response with error code
 * @param response Response frame (output)
 * @param error_code Error code
 */
void Commands_BuildNAK(Frame_t* response, ErrorCode_t error_code);

#ifdef __cplusplus
}
#endif

#endif /* COMMANDS_H */
