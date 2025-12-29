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
/* External Variables (from CubeMX generated code)                            */
/*============================================================================*/

extern "C" {
    /* CubeMX-generated peripheral handles */
    extern I2C_HandleTypeDef hi2c1;
    extern I2C_HandleTypeDef hi2c4;
    extern UART_HandleTypeDef huart4;
    
    /* CubeMX-generated initialization functions */
    void SystemClock_Config(void);
    static void MX_GPIO_Init(void);
    static void MX_I2C1_Init(void);
    static void MX_I2C4_Init(void);
    static void MX_UART4_Init(void);
    static void MPU_Config(void);
}

/*============================================================================*/
/* Private Function Prototypes                                                */
/*============================================================================*/

static void App_Init(void);
static void App_MainLoop(void);

/*============================================================================*/
/* UART Receive Callback Override                                             */
/*============================================================================*/

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    UART_Handler_RxCpltCallback(huart);
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
    
    /* Main application loop */
    while (1)
    {
        App_MainLoop();
    }
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
 * Processes incoming protocol messages and handles sensor testing.
 */
static void App_MainLoop(void)
{
    /* Process protocol communications */
    Protocol_Process();
}

/*============================================================================*/
/* CubeMX Generated Functions (copied from Core/Src/main.c)                   */
/*============================================================================*/

/**
 * @brief System Clock Configuration
 */
void SystemClock_Config(void)
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
    hi2c1.Init.Timing = 0x10B0DCFB;
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
    hi2c4.Init.Timing = 0x10B0DCFB;
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
