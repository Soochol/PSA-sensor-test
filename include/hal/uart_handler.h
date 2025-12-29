/**
 * @file uart_handler.h
 * @brief UART communication handler with interrupt-based reception
 *
 * Provides buffered UART communication with ring buffer for reception.
 * 
 * Hardware Configuration:
 *   - UART4: Host communication (PA11: RX, PA12: TX)
 *   - Baud: 115200, 8-N-1
 */

#ifndef UART_HANDLER_H
#define UART_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "stm32h7xx_hal.h"
#include "config.h"

/*============================================================================*/
/* Types                                                                      */
/*============================================================================*/

/**
 * @brief Callback function type for received data
 * @param data Pointer to received data
 * @param len Length of received data
 */
typedef void (*UART_RxCallback_t)(const uint8_t* data, uint16_t len);

/*============================================================================*/
/* Functions                                                                  */
/*============================================================================*/

/**
 * @brief Initialize UART handler
 * @param huart Pointer to HAL UART handle
 * @return HAL_OK on success
 */
HAL_StatusTypeDef UART_Handler_Init(UART_HandleTypeDef* huart);

/**
 * @brief Set callback for received data
 * @param callback Function to call when data is available
 */
void UART_Handler_SetRxCallback(UART_RxCallback_t callback);

/**
 * @brief Send data (blocking)
 * @param data Data to send
 * @param len Length of data
 * @param timeout_ms Timeout in milliseconds
 * @return HAL_OK on success
 */
HAL_StatusTypeDef UART_Handler_Send(const uint8_t* data, uint16_t len, uint32_t timeout_ms);

/**
 * @brief Read available data from receive buffer
 * @param buffer Output buffer
 * @param max_len Maximum bytes to read
 * @return Number of bytes read
 */
uint16_t UART_Handler_Read(uint8_t* buffer, uint16_t max_len);

/**
 * @brief Get number of bytes available in receive buffer
 * @return Number of available bytes
 */
uint16_t UART_Handler_Available(void);

/**
 * @brief Process received data (call from main loop)
 * 
 * This function processes the ring buffer and calls the registered callback
 * with any available data.
 */
void UART_Handler_Process(void);

/**
 * @brief UART receive complete callback (called from HAL)
 * @param huart UART handle
 */
void UART_Handler_RxCpltCallback(UART_HandleTypeDef* huart);

/**
 * @brief Clear receive buffer
 */
void UART_Handler_ClearRxBuffer(void);

#ifdef __cplusplus
}
#endif

#endif /* UART_HANDLER_H */
