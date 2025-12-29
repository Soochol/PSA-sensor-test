/**
 * @file MLX90640_I2C_Driver.h
 * @brief MLX90640 I2C Driver Interface
 * 
 * Platform-specific I2C implementation for MLX90640.
 * This implementation uses the I2C handler abstraction layer.
 */

#ifndef MLX90640_I2C_DRIVER_H
#define MLX90640_I2C_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Initialize I2C for MLX90640
 */
void MLX90640_I2CInit(void);

/**
 * @brief Read data from MLX90640
 * @param slaveAddr 7-bit I2C address
 * @param startAddress Register start address
 * @param nMemAddressRead Number of 16-bit words to read
 * @param data Output buffer
 * @return 0 on success, -1 on error
 */
int MLX90640_I2CRead(uint8_t slaveAddr, uint16_t startAddress,
                      uint16_t nMemAddressRead, uint16_t *data);

/**
 * @brief Write data to MLX90640
 * @param slaveAddr 7-bit I2C address
 * @param writeAddress Register address
 * @param data Data to write
 * @return 0 on success, -1 on error
 */
int MLX90640_I2CWrite(uint8_t slaveAddr, uint16_t writeAddress, uint16_t data);

/**
 * @brief Set I2C bus frequency
 * @param freq Frequency in kHz
 */
void MLX90640_I2CFreqSet(int freq);

#ifdef __cplusplus
}
#endif

#endif /* MLX90640_I2C_DRIVER_H */
