/**
 * @file mlx90640.c
 * @brief MLX90640 IR thermal camera driver implementation
 */

#include "sensors/mlx90640.h"
#include "hal/i2c_handler.h"
#include "config.h"
#include "MLX90640_API.h"
#include <string.h>
#include <stdlib.h>

/*============================================================================*/
/* Private Defines                                                            */
/*============================================================================*/

#define MLX90640_EEPROM_SIZE        832
#define MLX90640_FRAME_SIZE         834

/*============================================================================*/
/* Private Variables                                                          */
/*============================================================================*/

static SensorSpec_t current_spec;
static bool spec_set = false;
static bool initialized = false;

/* MLX90640 calibration data */
static paramsMLX90640 mlx_params;
static uint16_t eeprom_data[MLX90640_EEPROM_SIZE];
static uint16_t frame_data[MLX90640_FRAME_SIZE];
static float mlx_image[MLX90640_PIXEL_COUNT];

/*============================================================================*/
/* Private Function Prototypes                                                */
/*============================================================================*/

static HAL_StatusTypeDef MLX90640_Init(void);
static void MLX90640_Deinit(void);
static void MLX90640_SetSpec(const SensorSpec_t* spec);
static void MLX90640_GetSpec(SensorSpec_t* spec);
static bool MLX90640_HasSpec(void);
static TestStatus_t MLX90640_RunTest(SensorResult_t* result);
static uint8_t MLX90640_SerializeSpec(const SensorSpec_t* spec, uint8_t* buffer);
static uint8_t MLX90640_ParseSpec(const uint8_t* buffer, SensorSpec_t* spec);
static uint8_t MLX90640_SerializeResult(const SensorResult_t* result, uint8_t* buffer);

/*============================================================================*/
/* Driver Instance                                                            */
/*============================================================================*/

const SensorDriver_t MLX90640_Driver = {
    .id             = SENSOR_ID_MLX90640,
    .name           = "MLX90640",
    .init           = MLX90640_Init,
    .deinit         = MLX90640_Deinit,
    .set_spec       = MLX90640_SetSpec,
    .get_spec       = MLX90640_GetSpec,
    .has_spec       = MLX90640_HasSpec,
    .run_test       = MLX90640_RunTest,
    .serialize_spec = MLX90640_SerializeSpec,
    .parse_spec     = MLX90640_ParseSpec,
    .serialize_result = MLX90640_SerializeResult,
};

/*============================================================================*/
/* Private Functions                                                          */
/*============================================================================*/

static HAL_StatusTypeDef MLX90640_Init(void)
{
    if (initialized) {
        return HAL_OK;
    }
    
    /* Check device presence */
    if (I2C_Handler_IsDeviceReady(MLX90640_I2C_BUS, MLX90640_I2C_ADDR, 
                                   TIMEOUT_I2C_MS) != HAL_OK) {
        return HAL_ERROR;
    }
    
    /* Read EEPROM data */
    int status = MLX90640_DumpEE(MLX90640_I2C_ADDR, eeprom_data);
    if (status != 0) {
        return HAL_ERROR;
    }
    
    /* Extract parameters from EEPROM */
    status = MLX90640_ExtractParameters(eeprom_data, &mlx_params);
    if (status != 0) {
        return HAL_ERROR;
    }
    
    /* Set refresh rate (4Hz by default) */
    status = MLX90640_SetRefreshRate(MLX90640_I2C_ADDR, MLX90640_REFRESH_RATE);
    if (status != 0) {
        return HAL_ERROR;
    }
    
    /* Set resolution (19-bit by default) */
    status = MLX90640_SetResolution(MLX90640_I2C_ADDR, MLX90640_ADC_RESOLUTION);
    if (status != 0) {
        return HAL_ERROR;
    }
    
    initialized = true;
    return HAL_OK;
}

static void MLX90640_Deinit(void)
{
    initialized = false;
}

static void MLX90640_SetSpec(const SensorSpec_t* spec)
{
    if (spec != NULL) {
        current_spec = *spec;
        spec_set = true;
    }
}

static void MLX90640_GetSpec(SensorSpec_t* spec)
{
    if (spec != NULL) {
        *spec = current_spec;
    }
}

static bool MLX90640_HasSpec(void)
{
    return spec_set;
}

static TestStatus_t MLX90640_RunTest(SensorResult_t* result)
{
    if (result == NULL) {
        return STATUS_FAIL_INVALID;
    }
    
    /* Check if specification is set */
    if (!spec_set) {
        return STATUS_FAIL_NO_SPEC;
    }
    
    /* Initialize if needed */
    if (!initialized) {
        if (MLX90640_Init() != HAL_OK) {
            /* Check if it's a communication issue */
            if (I2C_Handler_IsDeviceReady(MLX90640_I2C_BUS, MLX90640_I2C_ADDR,
                                           TIMEOUT_I2C_MS) != HAL_OK) {
                return STATUS_FAIL_NO_ACK;
            }
            return STATUS_FAIL_INIT;
        }
    }
    
    /* Read both subpages for complete frame */
    int status;
    float emissivity = 0.95f;  /* Default emissivity for most surfaces */
    float tr = 23.0f;          /* Reflected temperature (ambient) */
    
    /* Read subpage 0 */
    status = MLX90640_GetFrameData(MLX90640_I2C_ADDR, frame_data);
    if (status < 0) {
        return STATUS_FAIL_TIMEOUT;
    }
    
    int subpage = MLX90640_GetSubPageNumber(frame_data);
    float vdd = MLX90640_GetVdd(frame_data, &mlx_params);
    float ta = MLX90640_GetTa(frame_data, &mlx_params);
    
    MLX90640_CalculateTo(frame_data, &mlx_params, emissivity, tr, mlx_image);
    
    /* Read subpage 1 for complete image */
    status = MLX90640_GetFrameData(MLX90640_I2C_ADDR, frame_data);
    if (status < 0) {
        return STATUS_FAIL_TIMEOUT;
    }
    
    MLX90640_CalculateTo(frame_data, &mlx_params, emissivity, tr, mlx_image);
    
    /* Find maximum temperature in image */
    float max_temp = mlx_image[0];
    for (int i = 1; i < MLX90640_PIXEL_COUNT; i++) {
        if (mlx_image[i] > max_temp) {
            max_temp = mlx_image[i];
        }
    }
    
    /* Convert to fixed-point (×100) */
    int16_t max_temp_x100 = (int16_t)(max_temp * 100.0f);
    
    /* Fill result structure */
    result->mlx90640.max_temp = max_temp_x100;
    result->mlx90640.target = current_spec.mlx90640.target_temp;
    result->mlx90640.tolerance = current_spec.mlx90640.tolerance;
    
    /* Calculate difference */
    int16_t diff = max_temp_x100 - current_spec.mlx90640.target_temp;
    if (diff < 0) diff = -diff;
    result->mlx90640.diff = (uint16_t)diff;
    
    /* Check against tolerance */
    if (result->mlx90640.diff > current_spec.mlx90640.tolerance) {
        return STATUS_FAIL_INVALID;
    }
    
    return STATUS_PASS;
}

static uint8_t MLX90640_SerializeSpec(const SensorSpec_t* spec, uint8_t* buffer)
{
    if (spec == NULL || buffer == NULL) {
        return 0;
    }
    
    /* Format: [target_temp_hi][target_temp_lo][tolerance_hi][tolerance_lo] */
    buffer[0] = (uint8_t)(spec->mlx90640.target_temp >> 8);
    buffer[1] = (uint8_t)(spec->mlx90640.target_temp & 0xFF);
    buffer[2] = (uint8_t)(spec->mlx90640.tolerance >> 8);
    buffer[3] = (uint8_t)(spec->mlx90640.tolerance & 0xFF);
    
    return 4;
}

static uint8_t MLX90640_ParseSpec(const uint8_t* buffer, SensorSpec_t* spec)
{
    if (buffer == NULL || spec == NULL) {
        return 0;
    }
    
    /* Parse big-endian format */
    spec->mlx90640.target_temp = (int16_t)((buffer[0] << 8) | buffer[1]);
    spec->mlx90640.tolerance = (uint16_t)((buffer[2] << 8) | buffer[3]);
    
    return 4;
}

static uint8_t MLX90640_SerializeResult(const SensorResult_t* result, uint8_t* buffer)
{
    if (result == NULL || buffer == NULL) {
        return 0;
    }
    
    /* Format: [max_temp][target][tolerance][diff] - all 16-bit big-endian */
    buffer[0] = (uint8_t)(result->mlx90640.max_temp >> 8);
    buffer[1] = (uint8_t)(result->mlx90640.max_temp & 0xFF);
    buffer[2] = (uint8_t)(result->mlx90640.target >> 8);
    buffer[3] = (uint8_t)(result->mlx90640.target & 0xFF);
    buffer[4] = (uint8_t)(result->mlx90640.tolerance >> 8);
    buffer[5] = (uint8_t)(result->mlx90640.tolerance & 0xFF);
    buffer[6] = (uint8_t)(result->mlx90640.diff >> 8);
    buffer[7] = (uint8_t)(result->mlx90640.diff & 0xFF);
    
    return 8;
}
