/**
 * @file vl53l0x_api.c
 * @brief VL53L0X ToF Sensor API Implementation
 * 
 * Simplified implementation based on ST VL53L0X driver.
 */

#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"
#include <string.h>

/*============================================================================*/
/* Register Addresses                                                         */
/*============================================================================*/

#define VL53L0X_REG_IDENTIFICATION_MODEL_ID         0xC0
#define VL53L0X_REG_SYSRANGE_START                  0x00
#define VL53L0X_REG_RESULT_INTERRUPT_STATUS         0x13
#define VL53L0X_REG_RESULT_RANGE_STATUS             0x14
#define VL53L0X_REG_SYSTEM_INTERRUPT_CONFIG_GPIO    0x0A
#define VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR          0x0B
#define VL53L0X_REG_GPIO_HV_MUX_ACTIVE_HIGH         0x84
#define VL53L0X_REG_GLOBAL_CONFIG_REF_EN_START_SELECT 0xB6
#define VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0 0xB0
#define VL53L0X_REG_DYNAMIC_SPAD_REF_EN_START_OFFSET 0x4F
#define VL53L0X_REG_DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD 0x4E

#define VL53L0X_EXPECTED_MODEL_ID                   0xEE

/*============================================================================*/
/* Private Variables                                                          */
/*============================================================================*/

static VL53L0X_DeviceModes currentMode = VL53L0X_DEVICEMODE_SINGLE_RANGING;
static uint8_t stopVariable = 0;

/*============================================================================*/
/* Private Function Prototypes                                                */
/*============================================================================*/

static VL53L0X_Error performSingleRefCalibration(VL53L0X_Dev_t *Dev, uint8_t vhv_init_byte);
static VL53L0X_Error getSpadInfo(VL53L0X_Dev_t *Dev, uint8_t *count, uint8_t *type_is_aperture);

/*============================================================================*/
/* Public Functions                                                           */
/*============================================================================*/

VL53L0X_Error VL53L0X_DataInit(VL53L0X_Dev_t *Dev)
{
    VL53L0X_Error status = VL53L0X_ERROR_NONE;
    uint8_t data;
    
    status = VL53L0X_ReadReg(Dev, VL53L0X_REG_IDENTIFICATION_MODEL_ID, &data);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    if (data != VL53L0X_EXPECTED_MODEL_ID) {
        return VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    
    status = VL53L0X_WriteReg(Dev, 0x88, 0x00);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    status = VL53L0X_WriteReg(Dev, 0x80, 0x01);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0xFF, 0x01);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0x00, 0x00);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    status = VL53L0X_ReadReg(Dev, 0x91, &stopVariable);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    status = VL53L0X_WriteReg(Dev, 0x00, 0x01);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0xFF, 0x00);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0x80, 0x00);
    
    return VL53L0X_ERROR_NONE;
}

VL53L0X_Error VL53L0X_StaticInit(VL53L0X_Dev_t *Dev)
{
    VL53L0X_Error status = VL53L0X_ERROR_NONE;
    uint8_t spad_count;
    uint8_t spad_type_is_aperture;
    uint8_t ref_spad_map[6];
    
    status = getSpadInfo(Dev, &spad_count, &spad_type_is_aperture);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    status = VL53L0X_ReadMulti(Dev, VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad_map, 6);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    status = VL53L0X_WriteReg(Dev, 0xFF, 0x01);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, VL53L0X_REG_DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, VL53L0X_REG_DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0xFF, 0x00);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, VL53L0X_REG_GLOBAL_CONFIG_REF_EN_START_SELECT, 0xB4);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    uint8_t first_spad_to_enable = spad_type_is_aperture ? 12 : 0;
    uint8_t spads_enabled = 0;
    
    for (int i = 0; i < 48; i++) {
        if (i < first_spad_to_enable || spads_enabled == spad_count) {
            ref_spad_map[i / 8] &= ~(1 << (i % 8));
        } else if ((ref_spad_map[i / 8] >> (i % 8)) & 0x1) {
            spads_enabled++;
        }
    }
    
    status = VL53L0X_WriteMulti(Dev, VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad_map, 6);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    /* Load tuning settings */
    const uint8_t tuning_regs[][2] = {
        {0xFF, 0x01}, {0x00, 0x00}, {0xFF, 0x00}, {0x09, 0x00}, {0x10, 0x00},
        {0x11, 0x00}, {0x24, 0x01}, {0x25, 0xFF}, {0x75, 0x00}, {0xFF, 0x01},
        {0x4E, 0x2C}, {0x48, 0x00}, {0x30, 0x20}, {0xFF, 0x00}, {0x30, 0x09},
        {0x54, 0x00}, {0x31, 0x04}, {0x32, 0x03}, {0x40, 0x83}, {0x46, 0x25},
        {0x60, 0x00}, {0x27, 0x00}, {0x50, 0x06}, {0x51, 0x00}, {0x52, 0x96},
        {0x56, 0x08}, {0x57, 0x30}, {0x61, 0x00}, {0x62, 0x00}, {0x64, 0x00},
        {0x65, 0x00}, {0x66, 0xA0}, {0xFF, 0x01}, {0x22, 0x32}, {0x47, 0x14},
        {0x49, 0xFF}, {0x4A, 0x00}, {0xFF, 0x00}, {0x7A, 0x0A}, {0x7B, 0x00},
        {0x78, 0x21}, {0xFF, 0x01}, {0x23, 0x34}, {0x42, 0x00}, {0x44, 0xFF},
        {0x45, 0x26}, {0x46, 0x05}, {0x40, 0x40}, {0x0E, 0x06}, {0x20, 0x1A},
        {0x43, 0x40}, {0xFF, 0x00}, {0x34, 0x03}, {0x35, 0x44}, {0xFF, 0x01},
        {0x31, 0x04}, {0x4B, 0x09}, {0x4C, 0x05}, {0x4D, 0x04}, {0xFF, 0x00},
        {0x44, 0x00}, {0x45, 0x20}, {0x47, 0x08}, {0x48, 0x28}, {0x67, 0x00},
        {0x70, 0x04}, {0x71, 0x01}, {0x72, 0xFE}, {0x76, 0x00}, {0x77, 0x00},
        {0xFF, 0x01}, {0x0D, 0x01}, {0xFF, 0x00}, {0x80, 0x01}, {0x01, 0xF8},
        {0xFF, 0x01}, {0x8E, 0x01}, {0x00, 0x01}, {0xFF, 0x00}, {0x80, 0x00}
    };
    
    for (size_t i = 0; i < sizeof(tuning_regs) / sizeof(tuning_regs[0]); i++) {
        status = VL53L0X_WriteReg(Dev, tuning_regs[i][0], tuning_regs[i][1]);
        if (status != VL53L0X_ERROR_NONE) return status;
    }
    
    status = VL53L0X_WriteReg(Dev, VL53L0X_REG_SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    uint8_t gpio_hv_mux;
    status = VL53L0X_ReadReg(Dev, VL53L0X_REG_GPIO_HV_MUX_ACTIVE_HIGH, &gpio_hv_mux);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    status = VL53L0X_WriteReg(Dev, VL53L0X_REG_GPIO_HV_MUX_ACTIVE_HIGH, gpio_hv_mux & ~0x10);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    status = VL53L0X_WriteReg(Dev, VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
    
    return status;
}

VL53L0X_Error VL53L0X_PerformRefSpadManagement(VL53L0X_Dev_t *Dev,
                                                 uint32_t *refSpadCount,
                                                 uint8_t *isApertureSpads)
{
    uint8_t spad_count;
    uint8_t spad_type;
    VL53L0X_Error status;
    
    status = getSpadInfo(Dev, &spad_count, &spad_type);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    *refSpadCount = spad_count;
    *isApertureSpads = spad_type;
    
    return VL53L0X_ERROR_NONE;
}

VL53L0X_Error VL53L0X_PerformRefCalibration(VL53L0X_Dev_t *Dev,
                                              uint8_t *pVhvSettings,
                                              uint8_t *pPhaseCal)
{
    VL53L0X_Error status;
    
    status = performSingleRefCalibration(Dev, 0x40);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    status = performSingleRefCalibration(Dev, 0x00);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    status = VL53L0X_ReadReg(Dev, 0xCB, pVhvSettings);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    status = VL53L0X_ReadReg(Dev, 0xEE, pPhaseCal);
    
    return status;
}

VL53L0X_Error VL53L0X_SetDeviceMode(VL53L0X_Dev_t *Dev, VL53L0X_DeviceModes DeviceMode)
{
    (void)Dev;
    currentMode = DeviceMode;
    return VL53L0X_ERROR_NONE;
}

VL53L0X_Error VL53L0X_SetMeasurementTimingBudgetMicroSeconds(VL53L0X_Dev_t *Dev,
                                                               uint32_t MeasurementTimingBudgetMicroSeconds)
{
    (void)Dev;
    (void)MeasurementTimingBudgetMicroSeconds;
    return VL53L0X_ERROR_NONE;
}

VL53L0X_Error VL53L0X_PerformSingleRangingMeasurement(VL53L0X_Dev_t *Dev,
                                                        VL53L0X_RangingMeasurementData_t *pRangingMeasurementData)
{
    VL53L0X_Error status;
    
    status = VL53L0X_StartMeasurement(Dev);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    uint8_t ready = 0;
    uint32_t timeout_loops = 0;
    
    while (ready == 0) {
        status = VL53L0X_GetMeasurementDataReady(Dev, &ready);
        if (status != VL53L0X_ERROR_NONE) return status;
        
        VL53L0X_PollingDelay(Dev);
        timeout_loops++;
        
        if (timeout_loops > 50000) {
            return VL53L0X_ERROR_TIME_OUT;
        }
    }
    
    status = VL53L0X_GetRangingMeasurementData(Dev, pRangingMeasurementData);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    status = VL53L0X_ClearInterruptMask(Dev, 0);
    
    return status;
}

VL53L0X_Error VL53L0X_StartMeasurement(VL53L0X_Dev_t *Dev)
{
    VL53L0X_Error status;
    
    status = VL53L0X_WriteReg(Dev, 0x80, 0x01);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0xFF, 0x01);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0x00, 0x00);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0x91, stopVariable);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0x00, 0x01);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0xFF, 0x00);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0x80, 0x00);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    if (currentMode == VL53L0X_DEVICEMODE_SINGLE_RANGING) {
        status = VL53L0X_WriteReg(Dev, VL53L0X_REG_SYSRANGE_START, 0x01);
    } else {
        status = VL53L0X_WriteReg(Dev, VL53L0X_REG_SYSRANGE_START, 0x02);
    }
    
    return status;
}

VL53L0X_Error VL53L0X_StopMeasurement(VL53L0X_Dev_t *Dev)
{
    VL53L0X_Error status;
    
    status = VL53L0X_WriteReg(Dev, VL53L0X_REG_SYSRANGE_START, 0x01);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    status = VL53L0X_WriteReg(Dev, 0xFF, 0x01);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0x00, 0x00);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0x91, 0x00);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0x00, 0x01);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0xFF, 0x00);
    
    return status;
}

VL53L0X_Error VL53L0X_GetMeasurementDataReady(VL53L0X_Dev_t *Dev, uint8_t *pMeasurementDataReady)
{
    VL53L0X_Error status;
    uint8_t interrupt_status;
    
    status = VL53L0X_ReadReg(Dev, VL53L0X_REG_RESULT_INTERRUPT_STATUS, &interrupt_status);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    *pMeasurementDataReady = (interrupt_status & 0x07) != 0;
    
    return VL53L0X_ERROR_NONE;
}

VL53L0X_Error VL53L0X_GetRangingMeasurementData(VL53L0X_Dev_t *Dev,
                                                  VL53L0X_RangingMeasurementData_t *pRangingMeasurementData)
{
    VL53L0X_Error status;
    uint8_t data[12];
    
    memset(pRangingMeasurementData, 0, sizeof(VL53L0X_RangingMeasurementData_t));
    
    status = VL53L0X_ReadMulti(Dev, VL53L0X_REG_RESULT_RANGE_STATUS, data, 12);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    pRangingMeasurementData->RangeStatus = (data[0] & 0x78) >> 3;
    pRangingMeasurementData->RangeMilliMeter = ((uint16_t)data[10] << 8) | data[11];
    pRangingMeasurementData->SignalRateRtnMegaCps = ((uint32_t)data[6] << 8) | data[7];
    pRangingMeasurementData->AmbientRateRtnMegaCps = ((uint32_t)data[8] << 8) | data[9];
    
    return VL53L0X_ERROR_NONE;
}

VL53L0X_Error VL53L0X_ClearInterruptMask(VL53L0X_Dev_t *Dev, uint32_t InterruptMask)
{
    (void)InterruptMask;
    return VL53L0X_WriteReg(Dev, VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
}

/*============================================================================*/
/* Private Functions                                                          */
/*============================================================================*/

static VL53L0X_Error performSingleRefCalibration(VL53L0X_Dev_t *Dev, uint8_t vhv_init_byte)
{
    VL53L0X_Error status;
    uint8_t data;
    uint32_t timeout_loops = 0;
    
    status = VL53L0X_WriteReg(Dev, VL53L0X_REG_SYSRANGE_START, 0x01 | vhv_init_byte);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    do {
        status = VL53L0X_ReadReg(Dev, VL53L0X_REG_RESULT_INTERRUPT_STATUS, &data);
        if (status != VL53L0X_ERROR_NONE) return status;
        
        VL53L0X_PollingDelay(Dev);
        timeout_loops++;
        
        if (timeout_loops > 50000) {
            return VL53L0X_ERROR_TIME_OUT;
        }
    } while ((data & 0x07) == 0);
    
    status = VL53L0X_WriteReg(Dev, VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    status = VL53L0X_WriteReg(Dev, VL53L0X_REG_SYSRANGE_START, 0x00);
    
    return status;
}

static VL53L0X_Error getSpadInfo(VL53L0X_Dev_t *Dev, uint8_t *count, uint8_t *type_is_aperture)
{
    VL53L0X_Error status;
    uint8_t tmp;
    
    status = VL53L0X_WriteReg(Dev, 0x80, 0x01);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0xFF, 0x01);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0x00, 0x00);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0xFF, 0x06);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    status = VL53L0X_ReadReg(Dev, 0x83, &tmp);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    status = VL53L0X_WriteReg(Dev, 0x83, tmp | 0x04);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    status = VL53L0X_WriteReg(Dev, 0xFF, 0x07);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0x81, 0x01);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0x80, 0x01);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0x94, 0x6B);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0x83, 0x00);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    uint32_t timeout_loops = 0;
    do {
        status = VL53L0X_ReadReg(Dev, 0x83, &tmp);
        if (status != VL53L0X_ERROR_NONE) return status;
        
        VL53L0X_PollingDelay(Dev);
        timeout_loops++;
        
        if (timeout_loops > 50000) {
            return VL53L0X_ERROR_TIME_OUT;
        }
    } while (tmp == 0);
    
    status = VL53L0X_WriteReg(Dev, 0x83, 0x01);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    status = VL53L0X_ReadReg(Dev, 0x92, &tmp);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    *count = tmp & 0x7F;
    *type_is_aperture = (tmp >> 7) & 0x01;
    
    status = VL53L0X_WriteReg(Dev, 0x81, 0x00);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0xFF, 0x06);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    status = VL53L0X_ReadReg(Dev, 0x83, &tmp);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    status = VL53L0X_WriteReg(Dev, 0x83, tmp & ~0x04);
    if (status != VL53L0X_ERROR_NONE) return status;
    
    status = VL53L0X_WriteReg(Dev, 0xFF, 0x01);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0x00, 0x01);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0xFF, 0x00);
    if (status != VL53L0X_ERROR_NONE) return status;
    status = VL53L0X_WriteReg(Dev, 0x80, 0x00);
    
    return status;
}
