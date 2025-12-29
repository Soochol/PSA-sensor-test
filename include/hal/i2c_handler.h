/**
 * @file i2c_handler.h
 * @brief I2C communication handler abstraction layer
 *
 * Provides a unified interface for I2C communication across multiple buses.
 * 
 * Hardware Configuration:
 *   - I2C1: VL53L0X ToF sensor (PB6: SCL, PB7: SDA)
 *   - I2C4: MLX90640 IR sensor (PB8: SCL, PB9: SDA)
 */

#ifndef I2C_HANDLER_H
#define I2C_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "stm32h7xx_hal.h"
#include "config.h"

/*============================================================================*/
/* Functions                                                                  */
/*============================================================================*/

/**
 * @brief Initialize I2C handler for a specific bus
 * @param bus_id Bus identifier (I2C_BUS_1 or I2C_BUS_4)
 * @param hi2c Pointer to HAL I2C handle
 * @return HAL_OK on success, HAL_ERROR on failure
 */
HAL_StatusTypeDef I2C_Handler_Init(I2C_BusID_t bus_id, I2C_HandleTypeDef* hi2c);

/**
 * @brief Check if a device is ready on the I2C bus
 * @param bus_id Bus identifier
 * @param dev_addr 7-bit device address
 * @param timeout_ms Timeout in milliseconds
 * @return HAL_OK if device responds, HAL_ERROR/HAL_TIMEOUT otherwise
 */
HAL_StatusTypeDef I2C_Handler_IsDeviceReady(I2C_BusID_t bus_id, uint8_t dev_addr,
                                             uint32_t timeout_ms);

/**
 * @brief Read data from a device register (16-bit register address)
 * @param bus_id Bus identifier
 * @param dev_addr 7-bit device address
 * @param reg_addr 16-bit register address
 * @param data Output buffer
 * @param len Number of bytes to read
 * @param timeout_ms Timeout in milliseconds
 * @return HAL_OK on success
 */
HAL_StatusTypeDef I2C_Handler_Read16(I2C_BusID_t bus_id, uint8_t dev_addr,
                                      uint16_t reg_addr, uint8_t* data,
                                      uint16_t len, uint32_t timeout_ms);

/**
 * @brief Write data to a device register (16-bit register address)
 * @param bus_id Bus identifier
 * @param dev_addr 7-bit device address
 * @param reg_addr 16-bit register address
 * @param data Input buffer
 * @param len Number of bytes to write
 * @param timeout_ms Timeout in milliseconds
 * @return HAL_OK on success
 */
HAL_StatusTypeDef I2C_Handler_Write16(I2C_BusID_t bus_id, uint8_t dev_addr,
                                       uint16_t reg_addr, const uint8_t* data,
                                       uint16_t len, uint32_t timeout_ms);

/**
 * @brief Read data from a device register (8-bit register address)
 * @param bus_id Bus identifier
 * @param dev_addr 7-bit device address
 * @param reg_addr 8-bit register address
 * @param data Output buffer
 * @param len Number of bytes to read
 * @param timeout_ms Timeout in milliseconds
 * @return HAL_OK on success
 */
HAL_StatusTypeDef I2C_Handler_Read8(I2C_BusID_t bus_id, uint8_t dev_addr,
                                     uint8_t reg_addr, uint8_t* data,
                                     uint16_t len, uint32_t timeout_ms);

/**
 * @brief Write data to a device register (8-bit register address)
 * @param bus_id Bus identifier
 * @param dev_addr 7-bit device address
 * @param reg_addr 8-bit register address
 * @param data Input buffer
 * @param len Number of bytes to write
 * @param timeout_ms Timeout in milliseconds
 * @return HAL_OK on success
 */
HAL_StatusTypeDef I2C_Handler_Write8(I2C_BusID_t bus_id, uint8_t dev_addr,
                                      uint8_t reg_addr, const uint8_t* data,
                                      uint16_t len, uint32_t timeout_ms);

/**
 * @brief Get the HAL I2C handle for a bus
 * @param bus_id Bus identifier
 * @return Pointer to I2C handle or NULL if not initialized
 */
I2C_HandleTypeDef* I2C_Handler_GetHandle(I2C_BusID_t bus_id);

#ifdef __cplusplus
}
#endif

#endif /* I2C_HANDLER_H */
