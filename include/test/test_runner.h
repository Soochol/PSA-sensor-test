/**
 * @file test_runner.h
 * @brief Test execution and result management
 * 
 * Coordinates sensor testing and produces test reports.
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
/* Functions                                                                  */
/*============================================================================*/

/**
 * @brief Initialize test runner
 */
void TestRunner_Init(void);

/**
 * @brief Run all registered sensor tests
 * @param report Output test report
 */
void TestRunner_RunAll(TestReport_t* report);

/**
 * @brief Run single sensor test
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

#ifdef __cplusplus
}
#endif

#endif /* TEST_RUNNER_H */
