/**
 * @file main.cpp
 * @brief PSA Sensor Test Firmware - Main Application Entry Point
 *
 * This is the main application file that replaces the CubeMX-generated main.c.
 * It initializes all system components and runs the main application loop.
 *
 * Hardware Configuration:
 *   - MCU: STM32H723VGT6 @ 384MHz
 *   - I2C1: VL53L0X ToF sensor (PB6: SCL, PB7: SDA)
 *   - I2C4: MLX90640 IR sensor (PB8: SCL, PB9: SDA)
 *   - UART4: Host communication (115200 bps)
 *   - IWDG: Independent Watchdog (timeout ~2 seconds)
 */

extern "C" {
#include "main.h"
#include "config.h"
#include "hal/i2c_handler.h"
#include "hal/uart_handler.h"
#include "protocol/protocol.h"
#include "sensors/sensor_manager.h"
#include "test/test_runner.h"
}

/*============================================================================*/
/* Global Variables (used by HAL MSP and IT)                                  */
/*============================================================================*/

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;
I2C_HandleTypeDef hi2c3;
I2C_HandleTypeDef hi2c4;
UART_HandleTypeDef huart4;

#if WATCHDOG_ENABLED
static IWDG_HandleTypeDef hiwdg;
#endif

/*============================================================================*/
/* Private Function Prototypes                                                */
/*============================================================================*/

static void App_Init(void);
static void App_MainLoop(void);
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C4_Init(void);
static void MX_UART4_Init(void);
static void MPU_Config(void);

#if WATCHDOG_ENABLED
static void MX_IWDG_Init(void);
#endif

/*============================================================================*/
/* UART Receive Callback Override                                             */
/*============================================================================*/

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    UART_Handler_RxCpltCallback(huart);
}

/*============================================================================*/
/* Debug Test Functions (call from debugger)                                  */
/*============================================================================*/

/* Debug results - check these in Watch window after calling test functions */
volatile int dbg_i2c1_ready = -1;      /* 0=OK, 1=ERROR */
volatile int dbg_i2c4_ready = -1;      /* 0=OK, 1=ERROR */
volatile int dbg_vl53l0x_init = -1;    /* 0=OK, 1=ERROR */
volatile int dbg_mlx90640_init = -1;   /* 0=OK, 1=ERROR */
volatile uint16_t dbg_vl53l0x_dist = 0;  /* Distance in mm */
volatile int16_t dbg_mlx90640_temp = 0;  /* Max temp x100 (2500 = 25.00C) */

/* I2C scan results - found device addresses (7-bit) */
volatile uint8_t dbg_i2c1_devices[8] = {0};  /* Found addresses on I2C1 */
volatile uint8_t dbg_i2c2_devices[8] = {0};  /* Found addresses on I2C2 */
volatile uint8_t dbg_i2c3_devices[8] = {0};  /* Found addresses on I2C3 */
volatile uint8_t dbg_i2c4_devices[8] = {0};  /* Found addresses on I2C4 */
volatile int dbg_i2c1_count = 0;             /* Number of devices found on I2C1 */
volatile int dbg_i2c2_count = 0;             /* Number of devices found on I2C2 */
volatile int dbg_i2c3_count = 0;             /* Number of devices found on I2C3 */
volatile int dbg_i2c4_count = 0;             /* Number of devices found on I2C4 */

/**
 * @brief Initialize I2C2 and I2C3 for scanning
 */
static void MX_I2C2_Init(void)
{
    hi2c2.Instance = I2C2;
    hi2c2.Init.Timing = 0x009032AE;
    hi2c2.Init.OwnAddress1 = 0;
    hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c2.Init.OwnAddress2 = 0;
    hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c2);
}

static void MX_I2C3_Init(void)
{
    hi2c3.Instance = I2C3;
    hi2c3.Init.Timing = 0x009032AE;
    hi2c3.Init.OwnAddress1 = 0;
    hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c3.Init.OwnAddress2 = 0;
    hi2c3.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c3);
}

/**
 * @brief Scan all I2C buses for devices (call from debugger)
 *
 * Results: dbg_i2cX_devices[], dbg_i2cX_count for X=1,2,3,4
 */
extern "C" void DBG_ScanI2C(void)
{
    dbg_i2c1_count = 0;
    dbg_i2c2_count = 0;
    dbg_i2c3_count = 0;
    dbg_i2c4_count = 0;

    /* Initialize I2C2 and I2C3 for scanning */
    __HAL_RCC_I2C2_CLK_ENABLE();
    __HAL_RCC_I2C3_CLK_ENABLE();
    MX_I2C2_Init();
    MX_I2C3_Init();

    /* Scan I2C1 (addresses 0x08 to 0x77) */
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(addr << 1), 1, 10) == HAL_OK) {
            if (dbg_i2c1_count < 8) {
                dbg_i2c1_devices[dbg_i2c1_count++] = addr;
            }
        }
    }

    /* Scan I2C2 (addresses 0x08 to 0x77) */
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        if (HAL_I2C_IsDeviceReady(&hi2c2, (uint16_t)(addr << 1), 1, 10) == HAL_OK) {
            if (dbg_i2c2_count < 8) {
                dbg_i2c2_devices[dbg_i2c2_count++] = addr;
            }
        }
    }

    /* Scan I2C3 (addresses 0x08 to 0x77) */
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        if (HAL_I2C_IsDeviceReady(&hi2c3, (uint16_t)(addr << 1), 1, 10) == HAL_OK) {
            if (dbg_i2c3_count < 8) {
                dbg_i2c3_devices[dbg_i2c3_count++] = addr;
            }
        }
    }

    /* Scan I2C4 (addresses 0x08 to 0x77) */
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        if (HAL_I2C_IsDeviceReady(&hi2c4, (uint16_t)(addr << 1), 1, 10) == HAL_OK) {
            if (dbg_i2c4_count < 8) {
                dbg_i2c4_devices[dbg_i2c4_count++] = addr;
            }
        }
    }
}

/**
 * @brief Test I2C device presence (call from debugger)
 *
 * Results stored in dbg_i2c1_ready and dbg_i2c4_ready
 * 0 = Device found, 1 = Device not found
 */
extern "C" void DBG_TestI2C(void)
{
    dbg_i2c1_ready = (I2C_Handler_IsDeviceReady(I2C_BUS_1, 0x29, 100) == HAL_OK) ? 0 : 1;
    dbg_i2c4_ready = (I2C_Handler_IsDeviceReady(I2C_BUS_4, 0x33, 100) == HAL_OK) ? 0 : 1;
}

/**
 * @brief Initialize and test VL53L0X sensor (call from debugger)
 *
 * Results: dbg_vl53l0x_init (0=OK), dbg_vl53l0x_dist (distance in mm)
 */
extern "C" void DBG_TestVL53L0X(void)
{
    const SensorDriver_t* driver = SensorManager_GetByID(SENSOR_ID_VL53L0X);
    if (driver == NULL) {
        dbg_vl53l0x_init = -2;
        return;
    }

    dbg_vl53l0x_init = (driver->init() == HAL_OK) ? 0 : 1;

    if (dbg_vl53l0x_init == 0) {
        /* Set a default spec and run test */
        SensorSpec_t spec;
        spec.vl53l0x.target_dist = 500;
        spec.vl53l0x.tolerance = 2000;
        driver->set_spec(&spec);

        SensorResult_t result;
        driver->run_test(&result);
        dbg_vl53l0x_dist = result.vl53l0x.measured;
    }
}

/**
 * @brief Initialize and test MLX90640 sensor (call from debugger)
 *
 * Results: dbg_mlx90640_init (0=OK), dbg_mlx90640_temp (temp x100)
 */
extern "C" void DBG_TestMLX90640(void)
{
    const SensorDriver_t* driver = SensorManager_GetByID(SENSOR_ID_MLX90640);
    if (driver == NULL) {
        dbg_mlx90640_init = -2;
        return;
    }

    dbg_mlx90640_init = (driver->init() == HAL_OK) ? 0 : 1;

    if (dbg_mlx90640_init == 0) {
        /* Set a default spec and run test */
        SensorSpec_t spec;
        spec.mlx90640.target_temp = 2500;  /* 25.00C */
        spec.mlx90640.tolerance = 5000;    /* +/- 50C */
        driver->set_spec(&spec);

        SensorResult_t result;
        driver->run_test(&result);
        dbg_mlx90640_temp = result.mlx90640.max_temp;
    }
}

/*============================================================================*/
/* Error Handler                                                              */
/*============================================================================*/

extern "C" void Error_Handler(void)
{
    __disable_irq();
    while (1) {
        /* Hang in infinite loop on error */
    }
}

/*============================================================================*/
/* Main Function                                                              */
/*============================================================================*/

int main(void)
{
    /* MPU Configuration */
    MPU_Config();
    
    /* MCU Configuration */
    HAL_Init();
    
    /* Configure the system clock */
    SystemClock_Config();
    
    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_I2C4_Init();
    MX_UART4_Init();

    /* Initialize application */
    App_Init();

    /* === DEBUG TEST MODE === */
    /* Scan I2C buses first to find all devices */
    DBG_ScanI2C();

    /* Run sensor tests */
    DBG_TestI2C();
    DBG_TestVL53L0X();
    DBG_TestMLX90640();

    /* Halt here - check dbg_* variables in Watch window */
    while (1) {
        __NOP();  /* Set breakpoint here to check results */
    }
    /* === END DEBUG TEST MODE === */

#if 0  /* Disabled for debugging */
    /* Initialize watchdog (after all other init to avoid reset during boot) */
#if WATCHDOG_ENABLED
    MX_IWDG_Init();
#endif

    /* Main application loop */
    while (1)
    {
        App_MainLoop();
    }
#endif
}

/*============================================================================*/
/* Private Functions                                                          */
/*============================================================================*/

/**
 * @brief Initialize application modules
 */
static void App_Init(void)
{
    /* Initialize I2C handlers */
    I2C_Handler_Init(I2C_BUS_1, &hi2c1);
    I2C_Handler_Init(I2C_BUS_4, &hi2c4);
    
    /* Initialize UART handler */
    UART_Handler_Init(&huart4);
    
    /* Initialize sensor manager (registers all sensor drivers) */
    SensorManager_Init();
    
    /* Initialize test runner */
    TestRunner_Init();
    
    /* Initialize protocol handler */
    Protocol_Init();
}

/**
 * @brief Main application loop
 *
 * Processes incoming protocol messages, handles async sensor testing,
 * and refreshes the watchdog timer.
 */
static void App_MainLoop(void)
{
    /* Process protocol communications */
    Protocol_Process();

    /* Process async test execution (non-blocking) */
    TestRunner_ProcessAsync();

    /* Refresh watchdog timer */
#if WATCHDOG_ENABLED
    HAL_IWDG_Refresh(&hiwdg);
#endif
}

/*============================================================================*/
/* CubeMX Generated Functions (copied from Core/Src/main.c)                   */
/*============================================================================*/

/**
 * @brief System Clock Configuration
 */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 96;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 125;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                                |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV16;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief I2C1 Initialization Function
 */
static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x009032AE;  /* ~400kHz (Fast Mode) - from working code */
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief I2C4 Initialization Function
 */
static void MX_I2C4_Init(void)
{
    hi2c4.Instance = I2C4;
    hi2c4.Init.Timing = 0x009032AE;  /* ~400kHz (Fast Mode) */
    hi2c4.Init.OwnAddress1 = 0;
    hi2c4.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c4.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c4.Init.OwnAddress2 = 0;
    hi2c4.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c4.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c4.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c4) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c4, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c4, 0) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief UART4 Initialization Function
 */
static void MX_UART4_Init(void)
{
    huart4.Instance = UART4;
    huart4.Init.BaudRate = 115200;
    huart4.Init.WordLength = UART_WORDLENGTH_8B;
    huart4.Init.StopBits = UART_STOPBITS_1;
    huart4.Init.Parity = UART_PARITY_NONE;
    huart4.Init.Mode = UART_MODE_TX_RX;
    huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart4.Init.OverSampling = UART_OVERSAMPLING_16;
    huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart4.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart4) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetTxFifoThreshold(&huart4, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetRxFifoThreshold(&huart4, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_DisableFifoMode(&huart4) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief GPIO Initialization Function
 */
static void MX_GPIO_Init(void)
{
    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
}

#if WATCHDOG_ENABLED
/**
 * @brief IWDG Initialization Function
 *
 * Configures Independent Watchdog with ~10 second timeout.
 * LSI clock is approximately 32kHz on STM32H7.
 * Timeout = (Prescaler * Reload) / LSI_freq
 *         = (256 * 1250) / 32000 = 10 seconds
 */
static void MX_IWDG_Init(void)
{
    hiwdg.Instance = IWDG1;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
    hiwdg.Init.Window = 4095;
    hiwdg.Init.Reload = 1250;  /* ~10 second timeout at 32kHz LSI with /256 prescaler */

    if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
        Error_Handler();
    }
}
#endif

/**
 * @brief MPU Configuration
 */
static void MPU_Config(void)
{
    MPU_Region_InitTypeDef MPU_InitStruct = {0};

    HAL_MPU_Disable();

    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER0;
    MPU_InitStruct.BaseAddress = 0x0;
    MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
    MPU_InitStruct.SubRegionDisable = 0x87;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}
