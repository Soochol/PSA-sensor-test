/**
 * @file mlx90640.h
 * @brief MLX90640 IR thermal camera sensor driver
 *
 * Hardware Configuration:
 *   - I2C4 (PB8: SCL, PB9: SDA)
 *   - I2C Address: 0x33 (7-bit)
 *   - Resolution: 32x24 pixels
 */

#ifndef MLX90640_H
#define MLX90640_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sensors/sensor_manager.h"

/*============================================================================*/
/* Exported Driver Instance                                                   */
/*============================================================================*/

/**
 * @brief MLX90640 sensor driver instance
 */
extern const SensorDriver_t MLX90640_Driver;

#ifdef __cplusplus
}
#endif

#endif /* MLX90640_H */
