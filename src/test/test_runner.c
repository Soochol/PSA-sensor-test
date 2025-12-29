/**
 * @file test_runner.c
 * @brief Test runner implementation
 */

#include "test/test_runner.h"
#include "stm32h7xx_hal.h"
#include <string.h>

/*============================================================================*/
/* Public Functions                                                           */
/*============================================================================*/

void TestRunner_Init(void)
{
    /* Nothing to initialize currently */
}

void TestRunner_RunAll(TestReport_t* report)
{
    if (report == NULL) {
        return;
    }
    
    /* Initialize report */
    memset(report, 0, sizeof(TestReport_t));
    report->timestamp = HAL_GetTick();
    
    uint8_t sensor_count = SensorManager_GetCount();
    report->sensor_count = sensor_count;
    
    /* Run test for each registered sensor */
    for (uint8_t i = 0; i < sensor_count && i < MAX_SENSORS; i++) {
        const SensorDriver_t* driver = SensorManager_GetByIndex(i);
        
        if (driver == NULL) {
            continue;
        }
        
        SensorTestResult_t* test_result = &report->results[i];
        test_result->sensor_id = driver->id;
        
        /* Initialize sensor if needed */
        if (driver->init != NULL) {
            HAL_StatusTypeDef init_status = driver->init();
            if (init_status != HAL_OK) {
                test_result->status = STATUS_FAIL_INIT;
                report->fail_count++;
                continue;
            }
        }
        
        /* Run test */
        if (driver->run_test != NULL) {
            test_result->status = driver->run_test(&test_result->result);
            
            if (test_result->status == STATUS_PASS) {
                report->pass_count++;
            } else {
                report->fail_count++;
            }
        } else {
            test_result->status = STATUS_NOT_TESTED;
        }
    }
}

void TestRunner_RunSingle(SensorID_t sensor_id, TestReport_t* report)
{
    if (report == NULL) {
        return;
    }
    
    /* Initialize report */
    memset(report, 0, sizeof(TestReport_t));
    report->timestamp = HAL_GetTick();
    report->sensor_count = 1;
    
    /* Get sensor driver */
    const SensorDriver_t* driver = SensorManager_GetByID(sensor_id);
    
    SensorTestResult_t* test_result = &report->results[0];
    test_result->sensor_id = sensor_id;
    
    if (driver == NULL) {
        test_result->status = STATUS_NOT_TESTED;
        return;
    }
    
    /* Initialize sensor if needed */
    if (driver->init != NULL) {
        HAL_StatusTypeDef init_status = driver->init();
        if (init_status != HAL_OK) {
            test_result->status = STATUS_FAIL_INIT;
            report->fail_count = 1;
            return;
        }
    }
    
    /* Run test */
    if (driver->run_test != NULL) {
        test_result->status = driver->run_test(&test_result->result);
        
        if (test_result->status == STATUS_PASS) {
            report->pass_count = 1;
        } else {
            report->fail_count = 1;
        }
    } else {
        test_result->status = STATUS_NOT_TESTED;
    }
}

uint16_t TestRunner_SerializeReport(const TestReport_t* report, uint8_t* buffer)
{
    if (report == NULL || buffer == NULL) {
        return 0;
    }
    
    uint16_t idx = 0;
    
    /* Header: [sensor_count][pass_count][fail_count][timestamp_32bit] */
    buffer[idx++] = report->sensor_count;
    buffer[idx++] = report->pass_count;
    buffer[idx++] = report->fail_count;
    
    /* Timestamp (32-bit big-endian) */
    buffer[idx++] = (uint8_t)(report->timestamp >> 24);
    buffer[idx++] = (uint8_t)(report->timestamp >> 16);
    buffer[idx++] = (uint8_t)(report->timestamp >> 8);
    buffer[idx++] = (uint8_t)(report->timestamp & 0xFF);
    
    /* Individual results */
    for (uint8_t i = 0; i < report->sensor_count && i < MAX_SENSORS; i++) {
        const SensorTestResult_t* result = &report->results[i];
        
        /* [sensor_id][status] */
        buffer[idx++] = (uint8_t)result->sensor_id;
        buffer[idx++] = (uint8_t)result->status;
        
        /* Result data (sensor-specific serialization) */
        const SensorDriver_t* driver = SensorManager_GetByID(result->sensor_id);
        
        if (driver != NULL && driver->serialize_result != NULL) {
            uint8_t result_len = driver->serialize_result(&result->result, &buffer[idx]);
            idx += result_len;
        } else {
            /* Default: copy raw bytes */
            memcpy(&buffer[idx], result->result.raw, 8);
            idx += 8;
        }
    }
    
    return idx;
}
