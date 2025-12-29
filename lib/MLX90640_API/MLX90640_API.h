/**
 * @file MLX90640_API.h
 * @brief MLX90640 IR Array Sensor API
 * 
 * Based on Melexis MLX90640 Driver Library.
 * Adapted for STM32H7 platform with custom I2C handler.
 */

#ifndef MLX90640_API_H
#define MLX90640_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*============================================================================*/
/* Constants                                                                  */
/*============================================================================*/

#define MLX90640_NO_ERROR           0
#define MLX90640_I2C_ERROR          -1
#define MLX90640_EEPROM_ERROR       -2
#define MLX90640_BROKEN_PIXEL_ERROR -3

/*============================================================================*/
/* Types                                                                      */
/*============================================================================*/

typedef struct {
    int16_t kVdd;
    int16_t vdd25;
    float KvPTAT;
    float KtPTAT;
    uint16_t vPTAT25;
    float alphaPTAT;
    int16_t gainEE;
    float tgc;
    float cpKv;
    float cpKta;
    uint8_t resolutionEE;
    uint8_t calibrationModeEE;
    float KsTa;
    float ksTo[5];
    int16_t ct[5];
    uint16_t alpha[768];
    uint8_t alphaScale;
    int16_t offset[768];
    int8_t kta[768];
    uint8_t ktaScale;
    int8_t kv[768];
    uint8_t kvScale;
    float cpAlpha[2];
    int16_t cpOffset[2];
    float ilChessC[3];
    uint16_t brokenPixels[5];
    uint16_t outlierPixels[5];
} paramsMLX90640;

/*============================================================================*/
/* API Functions                                                              */
/*============================================================================*/

/**
 * @brief Set I2C bus speed
 * @param freq Bus frequency in kHz
 */
void MLX90640_I2CFreqSet(int freq);

/**
 * @brief Read EEPROM data
 * @param slaveAddr I2C slave address (7-bit)
 * @param eeData Output buffer (832 words)
 * @return 0 on success, negative on error
 */
int MLX90640_DumpEE(uint8_t slaveAddr, uint16_t *eeData);

/**
 * @brief Extract calibration parameters from EEPROM
 * @param eeData EEPROM data
 * @param params Output parameters structure
 * @return 0 on success, negative on error
 */
int MLX90640_ExtractParameters(uint16_t *eeData, paramsMLX90640 *params);

/**
 * @brief Set refresh rate
 * @param slaveAddr I2C slave address
 * @param rate Rate setting (0-7)
 * @return 0 on success
 */
int MLX90640_SetRefreshRate(uint8_t slaveAddr, uint8_t rate);

/**
 * @brief Get current refresh rate
 * @param slaveAddr I2C slave address
 * @return Rate setting
 */
int MLX90640_GetRefreshRate(uint8_t slaveAddr);

/**
 * @brief Set ADC resolution
 * @param slaveAddr I2C slave address
 * @param resolution Resolution (16-19)
 * @return 0 on success
 */
int MLX90640_SetResolution(uint8_t slaveAddr, uint8_t resolution);

/**
 * @brief Get current resolution
 * @param slaveAddr I2C slave address
 * @return Resolution setting
 */
int MLX90640_GetCurResolution(uint8_t slaveAddr);

/**
 * @brief Read frame data from sensor
 * @param slaveAddr I2C slave address
 * @param frameData Output buffer (834 words)
 * @return 0 on success, negative on error
 */
int MLX90640_GetFrameData(uint8_t slaveAddr, uint16_t *frameData);

/**
 * @brief Get subpage number from frame data
 * @param frameData Frame data
 * @return Subpage number (0 or 1)
 */
int MLX90640_GetSubPageNumber(uint16_t *frameData);

/**
 * @brief Calculate Vdd from frame data
 * @param frameData Frame data
 * @param params Calibration parameters
 * @return Vdd voltage
 */
float MLX90640_GetVdd(uint16_t *frameData, const paramsMLX90640 *params);

/**
 * @brief Calculate ambient temperature
 * @param frameData Frame data
 * @param params Calibration parameters
 * @return Ambient temperature in °C
 */
float MLX90640_GetTa(uint16_t *frameData, const paramsMLX90640 *params);

/**
 * @brief Calculate object temperatures
 * @param frameData Frame data
 * @param params Calibration parameters
 * @param emissivity Surface emissivity
 * @param tr Reflected temperature
 * @param result Output temperature array (768 pixels)
 */
void MLX90640_CalculateTo(uint16_t *frameData, const paramsMLX90640 *params,
                          float emissivity, float tr, float *result);

/**
 * @brief Set interleaved mode
 * @param slaveAddr I2C slave address
 * @return 0 on success
 */
int MLX90640_SetInterleavedMode(uint8_t slaveAddr);

/**
 * @brief Set chess mode
 * @param slaveAddr I2C slave address
 * @return 0 on success
 */
int MLX90640_SetChessMode(uint8_t slaveAddr);

/**
 * @brief Get current mode
 * @param slaveAddr I2C slave address
 * @return Mode (0=interleaved, 1=chess)
 */
int MLX90640_GetCurMode(uint8_t slaveAddr);

/**
 * @brief Correct bad pixels
 * @param pixels Broken/outlier pixel indices
 * @param to Temperature array
 * @param mode Current mode
 * @param params Calibration parameters
 */
void MLX90640_BadPixelsCorrection(uint16_t *pixels, float *to, int mode,
                                   const paramsMLX90640 *params);

#ifdef __cplusplus
}
#endif

#endif /* MLX90640_API_H */
