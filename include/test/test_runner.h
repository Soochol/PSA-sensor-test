/**
 * @file test_runner.h
 * @brief Test execution and result management
 *
 * Coordinates sensor testing and produces test reports.
 * Supports both synchronous and asynchronous (non-blocking) test execution.
 */

#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sensors/sensor_types.h"
#include "sensors/sensor_manager.h"
#include "config.h"

/*============================================================================*/
/* Types                                                                      */
/*============================================================================*/

/**
 * @brief Async test execution state
 */
typedef enum {
    TEST_STATE_IDLE = 0,        /* No test in progress */
    TEST_STATE_RUNNING,         /* Test is running */
    TEST_STATE_COMPLETE,        /* Test completed, results available */
} TestState_t;

/**
 * @brief Individual sensor test result
 */
typedef struct {
    SensorID_t      sensor_id;      /* Sensor identifier */
    TestStatus_t    status;         /* Test result status */
    SensorResult_t  result;         /* Measurement result */
} SensorTestResult_t;

/**
 * @brief Complete test report
 */
typedef struct {
    uint8_t             sensor_count;               /* Number of sensors tested */
    uint8_t             pass_count;                 /* Number of passed tests */
    uint8_t             fail_count;                 /* Number of failed tests */
    uint32_t            timestamp;                  /* Report timestamp (ms) */
    SensorTestResult_t  results[MAX_SENSORS];       /* Individual results */
} TestReport_t;

/*============================================================================*/
/* Synchronous Functions                                                      */
/*============================================================================*/

/**
 * @brief Initialize test runner
 */
void TestRunner_Init(void);

/**
 * @brief Run all registered sensor tests (blocking)
 * @param report Output test report
 */
void TestRunner_RunAll(TestReport_t* report);

/**
 * @brief Run single sensor test (blocking)
 * @param sensor_id Sensor to test
 * @param report Output test report (will contain single result)
 */
void TestRunner_RunSingle(SensorID_t sensor_id, TestReport_t* report);

/**
 * @brief Serialize test report to byte buffer
 * @param report Test report
 * @param buffer Output buffer
 * @return Number of bytes written
 */
uint16_t TestRunner_SerializeReport(const TestReport_t* report, uint8_t* buffer);

/*============================================================================*/
/* Asynchronous Functions                                                     */
/*============================================================================*/

/**
 * @brief Start asynchronous test of all registered sensors
 * @return true if test started, false if already running
 */
bool TestRunner_StartAllAsync(void);

/**
 * @brief Start asynchronous test of single sensor
 * @param sensor_id Sensor to test
 * @return true if test started, false if already running or invalid sensor
 */
bool TestRunner_StartSingleAsync(SensorID_t sensor_id);

/**
 * @brief Process async test (call from main loop)
 *
 * Advances the test state machine. Must be called periodically
 * when async test is in progress.
 */
void TestRunner_ProcessAsync(void);

/**
 * @brief Get current async test state
 * @return Current test state
 */
TestState_t TestRunner_GetState(void);

/**
 * @brief Check if async test is complete
 * @return true if test completed and results available
 */
bool TestRunner_IsComplete(void);

/**
 * @brief Check if test runner is busy (test in progress)
 * @return true if test is running
 */
bool TestRunner_IsBusy(void);

/**
 * @brief Get async test report (valid only when state is COMPLETE)
 * @param report Output test report
 * @return true if report retrieved, false if no results available
 */
bool TestRunner_GetAsyncReport(TestReport_t* report);

/**
 * @brief Cancel async test in progress
 */
void TestRunner_CancelAsync(void);

#ifdef __cplusplus
}
#endif

#endif /* TEST_RUNNER_H */
