/**
 * @file vl53l0x.h
 * @brief VL53L0X ToF distance sensor driver
 *
 * Hardware Configuration:
 *   - I2C1 (PB6: SCL, PB7: SDA)
 *   - I2C Address: 0x29 (7-bit)
 *   - Range: 30mm to 2000mm
 */

#ifndef VL53L0X_H
#define VL53L0X_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sensors/sensor_manager.h"

/*============================================================================*/
/* Exported Driver Instance                                                   */
/*============================================================================*/

/**
 * @brief VL53L0X sensor driver instance
 */
extern const SensorDriver_t VL53L0X_Driver;

#ifdef __cplusplus
}
#endif

#endif /* VL53L0X_H */
