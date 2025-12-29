/**
 * @file vl53l0x_platform.h
 * @brief VL53L0X Platform Abstraction Layer
 * 
 * Platform-specific I2C and timing implementation.
 */

#ifndef VL53L0X_PLATFORM_H
#define VL53L0X_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "vl53l0x_api.h"

/**
 * @brief Write single byte to register
 * @param Dev Device handle
 * @param index Register address
 * @param data Data to write
 * @return Error code
 */
VL53L0X_Error VL53L0X_WriteReg(VL53L0X_Dev_t *Dev, uint8_t index, uint8_t data);

/**
 * @brief Write 16-bit value to register
 * @param Dev Device handle
 * @param index Register address
 * @param data Data to write
 * @return Error code
 */
VL53L0X_Error VL53L0X_WriteReg16(VL53L0X_Dev_t *Dev, uint8_t index, uint16_t data);

/**
 * @brief Write 32-bit value to register
 * @param Dev Device handle
 * @param index Register address
 * @param data Data to write
 * @return Error code
 */
VL53L0X_Error VL53L0X_WriteReg32(VL53L0X_Dev_t *Dev, uint8_t index, uint32_t data);

/**
 * @brief Write multiple bytes
 * @param Dev Device handle
 * @param index Register address
 * @param pdata Data buffer
 * @param count Number of bytes
 * @return Error code
 */
VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_Dev_t *Dev, uint8_t index,
                                   uint8_t *pdata, uint32_t count);

/**
 * @brief Read single byte from register
 * @param Dev Device handle
 * @param index Register address
 * @param data Output data
 * @return Error code
 */
VL53L0X_Error VL53L0X_ReadReg(VL53L0X_Dev_t *Dev, uint8_t index, uint8_t *data);

/**
 * @brief Read 16-bit value from register
 * @param Dev Device handle
 * @param index Register address
 * @param data Output data
 * @return Error code
 */
VL53L0X_Error VL53L0X_ReadReg16(VL53L0X_Dev_t *Dev, uint8_t index, uint16_t *data);

/**
 * @brief Read 32-bit value from register
 * @param Dev Device handle
 * @param index Register address
 * @param data Output data
 * @return Error code
 */
VL53L0X_Error VL53L0X_ReadReg32(VL53L0X_Dev_t *Dev, uint8_t index, uint32_t *data);

/**
 * @brief Read multiple bytes
 * @param Dev Device handle
 * @param index Register address
 * @param pdata Output buffer
 * @param count Number of bytes
 * @return Error code
 */
VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_Dev_t *Dev, uint8_t index,
                                  uint8_t *pdata, uint32_t count);

/**
 * @brief Polling delay in milliseconds
 * @param Dev Device handle
 */
VL53L0X_Error VL53L0X_PollingDelay(VL53L0X_Dev_t *Dev);

#ifdef __cplusplus
}
#endif

#endif /* VL53L0X_PLATFORM_H */
