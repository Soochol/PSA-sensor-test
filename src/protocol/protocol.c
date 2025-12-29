/**
 * @file protocol.c
 * @brief Protocol module main implementation
 */

#include "protocol/protocol.h"
#include "protocol/frame.h"
#include "protocol/commands.h"
#include "hal/uart_handler.h"
#include <string.h>

/*============================================================================*/
/* Private Variables                                                          */
/*============================================================================*/

static uint8_t rx_buffer[PROTOCOL_RX_BUFFER_SIZE];
static uint16_t rx_buffer_len = 0;
static volatile bool busy = false;

/*============================================================================*/
/* Private Function Prototypes                                                */
/*============================================================================*/

static void Protocol_RxCallback(const uint8_t* data, uint16_t len);

/*============================================================================*/
/* Public Functions                                                           */
/*============================================================================*/

void Protocol_Init(void)
{
    rx_buffer_len = 0;
    busy = false;
    
    /* Initialize command handlers */
    Commands_Init();
    
    /* Set UART receive callback */
    UART_Handler_SetRxCallback(Protocol_RxCallback);
}

void Protocol_Process(void)
{
    /* Process any pending UART data */
    UART_Handler_Process();
    
    /* Try to parse frames from buffer */
    while (rx_buffer_len > 0) {
        Frame_t request;
        uint16_t consumed = 0;
        
        FrameParseResult_t result = Frame_Parse(rx_buffer, rx_buffer_len,
                                                 &request, &consumed);
        
        if (result == FRAME_PARSE_INCOMPLETE) {
            /* Need more data */
            break;
        }
        
        /* Remove consumed bytes from buffer */
        if (consumed > 0) {
            if (consumed < rx_buffer_len) {
                memmove(rx_buffer, &rx_buffer[consumed], rx_buffer_len - consumed);
            }
            rx_buffer_len -= consumed;
        }
        
        if (result == FRAME_PARSE_OK) {
            /* Process command and send response */
            Frame_t response;
            bool send_response = Commands_Process(&request, &response);
            
            if (send_response) {
                uint8_t tx_buffer[PROTOCOL_MAX_PAYLOAD + 5];
                uint16_t tx_len = Frame_Build(&response, tx_buffer);
                
                if (tx_len > 0) {
                    UART_Handler_Send(tx_buffer, tx_len, TIMEOUT_UART_TX_MS);
                }
            }
        } else if (result == FRAME_PARSE_CRC_ERROR) {
            /* Send NAK for CRC error */
            Frame_t response;
            Commands_BuildNAK(&response, ERR_CRC_FAIL);
            
            uint8_t tx_buffer[PROTOCOL_MAX_PAYLOAD + 5];
            uint16_t tx_len = Frame_Build(&response, tx_buffer);
            
            if (tx_len > 0) {
                UART_Handler_Send(tx_buffer, tx_len, TIMEOUT_UART_TX_MS);
            }
        }
        /* FRAME_PARSE_FORMAT_ERR: silently discard and continue */
    }
}

bool Protocol_IsBusy(void)
{
    return busy;
}

/*============================================================================*/
/* Private Functions                                                          */
/*============================================================================*/

static void Protocol_RxCallback(const uint8_t* data, uint16_t len)
{
    /* Append received data to buffer */
    uint16_t space = PROTOCOL_RX_BUFFER_SIZE - rx_buffer_len;
    uint16_t copy_len = (len > space) ? space : len;
    
    if (copy_len > 0) {
        memcpy(&rx_buffer[rx_buffer_len], data, copy_len);
        rx_buffer_len += copy_len;
    }
}
