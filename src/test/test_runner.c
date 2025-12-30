/**
 * @file test_runner.c
 * @brief Test runner implementation with async support
 */

#include "test/test_runner.h"
#include "stm32h7xx_hal.h"
#include <string.h>

/*============================================================================*/
/* Private Types                                                              */
/*============================================================================*/

typedef enum {
    ASYNC_MODE_NONE = 0,
    ASYNC_MODE_ALL,
    ASYNC_MODE_SINGLE,
} AsyncMode_t;

typedef struct {
    TestState_t     state;
    AsyncMode_t     mode;
    uint8_t         current_index;      /* Current sensor index for ALL mode */
    SensorID_t      target_sensor;      /* Target sensor for SINGLE mode */
    TestReport_t    report;
} AsyncContext_t;

/*============================================================================*/
/* Private Variables                                                          */
/*============================================================================*/

static AsyncContext_t async_ctx;

/*============================================================================*/
/* Private Function Prototypes                                                */
/*============================================================================*/

static void RunSensorTest(const SensorDriver_t* driver, SensorTestResult_t* result);

/*============================================================================*/
/* Public Functions - Initialization                                          */
/*============================================================================*/

void TestRunner_Init(void)
{
    memset(&async_ctx, 0, sizeof(async_ctx));
    async_ctx.state = TEST_STATE_IDLE;
    async_ctx.mode = ASYNC_MODE_NONE;
}

/*============================================================================*/
/* Private Functions                                                          */
/*============================================================================*/

static void RunSensorTest(const SensorDriver_t* driver, SensorTestResult_t* result)
{
    result->sensor_id = driver->id;

    /* Initialize sensor if needed */
    if (driver->init != NULL) {
        HAL_StatusTypeDef init_status = driver->init();
        if (init_status != HAL_OK) {
            result->status = STATUS_FAIL_INIT;
            return;
        }
    }

    /* Run test */
    if (driver->run_test != NULL) {
        result->status = driver->run_test(&result->result);
    } else {
        result->status = STATUS_NOT_TESTED;
    }
}

/*============================================================================*/
/* Public Functions - Synchronous                                             */
/*============================================================================*/

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
        RunSensorTest(driver, test_result);

        if (test_result->status == STATUS_PASS) {
            report->pass_count++;
        } else if (test_result->status != STATUS_NOT_TESTED) {
            report->fail_count++;
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

    RunSensorTest(driver, test_result);

    if (test_result->status == STATUS_PASS) {
        report->pass_count = 1;
    } else if (test_result->status != STATUS_NOT_TESTED) {
        report->fail_count = 1;
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

/*============================================================================*/
/* Public Functions - Asynchronous                                            */
/*============================================================================*/

bool TestRunner_StartAllAsync(void)
{
    if (async_ctx.state == TEST_STATE_RUNNING) {
        return false;  /* Already running */
    }

    /* Initialize async context */
    memset(&async_ctx.report, 0, sizeof(TestReport_t));
    async_ctx.report.timestamp = HAL_GetTick();
    async_ctx.report.sensor_count = SensorManager_GetCount();
    async_ctx.current_index = 0;
    async_ctx.mode = ASYNC_MODE_ALL;
    async_ctx.state = TEST_STATE_RUNNING;

    return true;
}

bool TestRunner_StartSingleAsync(SensorID_t sensor_id)
{
    if (async_ctx.state == TEST_STATE_RUNNING) {
        return false;  /* Already running */
    }

    /* Validate sensor ID */
    if (!SensorManager_IsValidID(sensor_id)) {
        return false;
    }

    /* Initialize async context */
    memset(&async_ctx.report, 0, sizeof(TestReport_t));
    async_ctx.report.timestamp = HAL_GetTick();
    async_ctx.report.sensor_count = 1;
    async_ctx.target_sensor = sensor_id;
    async_ctx.current_index = 0;
    async_ctx.mode = ASYNC_MODE_SINGLE;
    async_ctx.state = TEST_STATE_RUNNING;

    return true;
}

void TestRunner_ProcessAsync(void)
{
    if (async_ctx.state != TEST_STATE_RUNNING) {
        return;
    }

    if (async_ctx.mode == ASYNC_MODE_ALL) {
        /* Process one sensor per call */
        uint8_t sensor_count = SensorManager_GetCount();

        if (async_ctx.current_index >= sensor_count) {
            /* All sensors tested */
            async_ctx.state = TEST_STATE_COMPLETE;
            return;
        }

        const SensorDriver_t* driver = SensorManager_GetByIndex(async_ctx.current_index);

        if (driver != NULL) {
            SensorTestResult_t* test_result = &async_ctx.report.results[async_ctx.current_index];
            RunSensorTest(driver, test_result);

            if (test_result->status == STATUS_PASS) {
                async_ctx.report.pass_count++;
            } else if (test_result->status != STATUS_NOT_TESTED) {
                async_ctx.report.fail_count++;
            }
        }

        async_ctx.current_index++;

        /* Check if all sensors done */
        if (async_ctx.current_index >= sensor_count) {
            async_ctx.state = TEST_STATE_COMPLETE;
        }

    } else if (async_ctx.mode == ASYNC_MODE_SINGLE) {
        /* Single sensor test - completes in one call */
        const SensorDriver_t* driver = SensorManager_GetByID(async_ctx.target_sensor);

        SensorTestResult_t* test_result = &async_ctx.report.results[0];
        test_result->sensor_id = async_ctx.target_sensor;

        if (driver != NULL) {
            RunSensorTest(driver, test_result);

            if (test_result->status == STATUS_PASS) {
                async_ctx.report.pass_count = 1;
            } else if (test_result->status != STATUS_NOT_TESTED) {
                async_ctx.report.fail_count = 1;
            }
        } else {
            test_result->status = STATUS_NOT_TESTED;
        }

        async_ctx.state = TEST_STATE_COMPLETE;
    }
}

TestState_t TestRunner_GetState(void)
{
    return async_ctx.state;
}

bool TestRunner_IsComplete(void)
{
    return async_ctx.state == TEST_STATE_COMPLETE;
}

bool TestRunner_IsBusy(void)
{
    return async_ctx.state == TEST_STATE_RUNNING;
}

bool TestRunner_GetAsyncReport(TestReport_t* report)
{
    if (report == NULL) {
        return false;
    }

    if (async_ctx.state != TEST_STATE_COMPLETE) {
        return false;
    }

    *report = async_ctx.report;

    /* Reset state after retrieving report */
    async_ctx.state = TEST_STATE_IDLE;
    async_ctx.mode = ASYNC_MODE_NONE;

    return true;
}

void TestRunner_CancelAsync(void)
{
    async_ctx.state = TEST_STATE_IDLE;
    async_ctx.mode = ASYNC_MODE_NONE;
}
