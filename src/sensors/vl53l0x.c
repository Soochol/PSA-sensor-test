/**
 * @file vl53l0x.c
 * @brief VL53L0X ToF distance sensor driver implementation
 */

#include "sensors/vl53l0x.h"
#include "hal/i2c_handler.h"
#include "config.h"
#include "vl53l0x_api.h"
#include <string.h>
#include <stdlib.h>

/*============================================================================*/
/* Private Variables                                                          */
/*============================================================================*/

static SensorSpec_t current_spec;
static bool spec_set = false;
static bool initialized = false;

/* VL53L0X device handle */
static VL53L0X_Dev_t vl53l0x_dev;

/*============================================================================*/
/* Private Function Prototypes                                                */
/*============================================================================*/

static HAL_StatusTypeDef VL53L0X_Init_Driver(void);
static void VL53L0X_Deinit(void);
static void VL53L0X_SetSpec(const SensorSpec_t* spec);
static void VL53L0X_GetSpec(SensorSpec_t* spec);
static bool VL53L0X_HasSpec(void);
static TestStatus_t VL53L0X_RunTest(SensorResult_t* result);
static uint8_t VL53L0X_SerializeSpec(const SensorSpec_t* spec, uint8_t* buffer);
static uint8_t VL53L0X_ParseSpec(const uint8_t* buffer, SensorSpec_t* spec);
static uint8_t VL53L0X_SerializeResult(const SensorResult_t* result, uint8_t* buffer);

/*============================================================================*/
/* Driver Instance                                                            */
/*============================================================================*/

const SensorDriver_t VL53L0X_Driver = {
    .id             = SENSOR_ID_VL53L0X,
    .name           = "VL53L0X",
    .init           = VL53L0X_Init_Driver,
    .deinit         = VL53L0X_Deinit,
    .set_spec       = VL53L0X_SetSpec,
    .get_spec       = VL53L0X_GetSpec,
    .has_spec       = VL53L0X_HasSpec,
    .run_test       = VL53L0X_RunTest,
    .serialize_spec = VL53L0X_SerializeSpec,
    .parse_spec     = VL53L0X_ParseSpec,
    .serialize_result = VL53L0X_SerializeResult,
};

/*============================================================================*/
/* Private Functions                                                          */
/*============================================================================*/

static HAL_StatusTypeDef VL53L0X_Init_Driver(void)
{
    VL53L0X_Error status;
    
    if (initialized) {
        return HAL_OK;
    }
    
    /* Check device presence */
    if (I2C_Handler_IsDeviceReady(VL53L0X_I2C_BUS, VL53L0X_I2C_ADDR, 
                                   TIMEOUT_I2C_MS) != HAL_OK) {
        return HAL_ERROR;
    }
    
    /* Initialize device structure */
    vl53l0x_dev.I2cDevAddr = VL53L0X_I2C_ADDR;
    vl53l0x_dev.comms_type = 1;  /* I2C */
    vl53l0x_dev.comms_speed_khz = 400;
    
    /* Data initialization */
    status = VL53L0X_DataInit(&vl53l0x_dev);
    if (status != VL53L0X_ERROR_NONE) {
        return HAL_ERROR;
    }
    
    /* Static initialization */
    status = VL53L0X_StaticInit(&vl53l0x_dev);
    if (status != VL53L0X_ERROR_NONE) {
        return HAL_ERROR;
    }
    
    /* Reference SPAD calibration */
    uint32_t refSpadCount;
    uint8_t isApertureSpads;
    status = VL53L0X_PerformRefSpadManagement(&vl53l0x_dev, &refSpadCount, &isApertureSpads);
    if (status != VL53L0X_ERROR_NONE) {
        return HAL_ERROR;
    }
    
    /* Reference calibration */
    uint8_t VhvSettings, PhaseCal;
    status = VL53L0X_PerformRefCalibration(&vl53l0x_dev, &VhvSettings, &PhaseCal);
    if (status != VL53L0X_ERROR_NONE) {
        return HAL_ERROR;
    }
    
    /* Set measurement mode */
    status = VL53L0X_SetDeviceMode(&vl53l0x_dev, VL53L0X_DEVICEMODE_SINGLE_RANGING);
    if (status != VL53L0X_ERROR_NONE) {
        return HAL_ERROR;
    }
    
    /* Set timing budget */
    status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&vl53l0x_dev, VL53L0X_TIMING_BUDGET_US);
    if (status != VL53L0X_ERROR_NONE) {
        return HAL_ERROR;
    }
    
    initialized = true;
    return HAL_OK;
}

static void VL53L0X_Deinit(void)
{
    initialized = false;
}

static void VL53L0X_SetSpec(const SensorSpec_t* spec)
{
    if (spec != NULL) {
        current_spec = *spec;
        spec_set = true;
    }
}

static void VL53L0X_GetSpec(SensorSpec_t* spec)
{
    if (spec != NULL) {
        *spec = current_spec;
    }
}

static bool VL53L0X_HasSpec(void)
{
    return spec_set;
}

static TestStatus_t VL53L0X_RunTest(SensorResult_t* result)
{
    VL53L0X_Error status;
    VL53L0X_RangingMeasurementData_t measurementData;
    
    if (result == NULL) {
        return STATUS_FAIL_INVALID;
    }
    
    /* Check if specification is set */
    if (!spec_set) {
        return STATUS_FAIL_NO_SPEC;
    }
    
    /* Initialize if needed */
    if (!initialized) {
        if (VL53L0X_Init_Driver() != HAL_OK) {
            /* Check if it's a communication issue */
            if (I2C_Handler_IsDeviceReady(VL53L0X_I2C_BUS, VL53L0X_I2C_ADDR,
                                           TIMEOUT_I2C_MS) != HAL_OK) {
                return STATUS_FAIL_NO_ACK;
            }
            return STATUS_FAIL_INIT;
        }
    }
    
    /* Perform single measurement */
    status = VL53L0X_PerformSingleRangingMeasurement(&vl53l0x_dev, &measurementData);
    if (status != VL53L0X_ERROR_NONE) {
        return STATUS_FAIL_TIMEOUT;
    }
    
    /* Check range status */
    if (measurementData.RangeStatus != 0) {
        /* Range error - but still report measurement */
    }
    
    /* Get measured distance in mm */
    uint16_t measured_mm = measurementData.RangeMilliMeter;
    
    /* Fill result structure */
    result->vl53l0x.measured = measured_mm;
    result->vl53l0x.target = current_spec.vl53l0x.target_dist;
    result->vl53l0x.tolerance = current_spec.vl53l0x.tolerance;
    
    /* Calculate difference */
    int16_t diff = (int16_t)measured_mm - (int16_t)current_spec.vl53l0x.target_dist;
    if (diff < 0) diff = -diff;
    result->vl53l0x.diff = (uint16_t)diff;
    
    /* Check against tolerance */
    if (result->vl53l0x.diff > current_spec.vl53l0x.tolerance) {
        return STATUS_FAIL_INVALID;
    }
    
    return STATUS_PASS;
}

static uint8_t VL53L0X_SerializeSpec(const SensorSpec_t* spec, uint8_t* buffer)
{
    if (spec == NULL || buffer == NULL) {
        return 0;
    }
    
    /* Format: [target_dist_hi][target_dist_lo][tolerance_hi][tolerance_lo] */
    buffer[0] = (uint8_t)(spec->vl53l0x.target_dist >> 8);
    buffer[1] = (uint8_t)(spec->vl53l0x.target_dist & 0xFF);
    buffer[2] = (uint8_t)(spec->vl53l0x.tolerance >> 8);
    buffer[3] = (uint8_t)(spec->vl53l0x.tolerance & 0xFF);
    
    return 4;
}

static uint8_t VL53L0X_ParseSpec(const uint8_t* buffer, SensorSpec_t* spec)
{
    if (buffer == NULL || spec == NULL) {
        return 0;
    }
    
    /* Parse big-endian format */
    spec->vl53l0x.target_dist = (uint16_t)((buffer[0] << 8) | buffer[1]);
    spec->vl53l0x.tolerance = (uint16_t)((buffer[2] << 8) | buffer[3]);
    
    return 4;
}

static uint8_t VL53L0X_SerializeResult(const SensorResult_t* result, uint8_t* buffer)
{
    if (result == NULL || buffer == NULL) {
        return 0;
    }
    
    /* Format: [measured][target][tolerance][diff] - all 16-bit big-endian */
    buffer[0] = (uint8_t)(result->vl53l0x.measured >> 8);
    buffer[1] = (uint8_t)(result->vl53l0x.measured & 0xFF);
    buffer[2] = (uint8_t)(result->vl53l0x.target >> 8);
    buffer[3] = (uint8_t)(result->vl53l0x.target & 0xFF);
    buffer[4] = (uint8_t)(result->vl53l0x.tolerance >> 8);
    buffer[5] = (uint8_t)(result->vl53l0x.tolerance & 0xFF);
    buffer[6] = (uint8_t)(result->vl53l0x.diff >> 8);
    buffer[7] = (uint8_t)(result->vl53l0x.diff & 0xFF);
    
    return 8;
}
