/**
 * @file uart_handler.c
 * @brief UART communication handler implementation
 */

#include "hal/uart_handler.h"
#include <string.h>

/*============================================================================*/
/* Private Types                                                              */
/*============================================================================*/

typedef struct {
    uint8_t buffer[UART_RX_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} RingBuffer_t;

/*============================================================================*/
/* Private Variables                                                          */
/*============================================================================*/

static UART_HandleTypeDef* uart_handle = NULL;
static UART_RxCallback_t rx_callback = NULL;
static RingBuffer_t rx_ring;
static uint8_t rx_byte;  /* Single byte for interrupt reception */

/*============================================================================*/
/* Private Functions                                                          */
/*============================================================================*/

static inline uint16_t RingBuffer_Count(const RingBuffer_t* rb)
{
    uint16_t head = rb->head;
    uint16_t tail = rb->tail;
    
    if (head >= tail) {
        return head - tail;
    } else {
        return UART_RX_BUFFER_SIZE - tail + head;
    }
}

static inline bool RingBuffer_IsFull(const RingBuffer_t* rb)
{
    uint16_t next_head = (rb->head + 1) % UART_RX_BUFFER_SIZE;
    return next_head == rb->tail;
}

static inline bool RingBuffer_IsEmpty(const RingBuffer_t* rb)
{
    return rb->head == rb->tail;
}

static inline void RingBuffer_Push(RingBuffer_t* rb, uint8_t byte)
{
    if (!RingBuffer_IsFull(rb)) {
        rb->buffer[rb->head] = byte;
        rb->head = (rb->head + 1) % UART_RX_BUFFER_SIZE;
    }
}

static inline bool RingBuffer_Pop(RingBuffer_t* rb, uint8_t* byte)
{
    if (RingBuffer_IsEmpty(rb)) {
        return false;
    }
    
    *byte = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % UART_RX_BUFFER_SIZE;
    return true;
}

static inline void RingBuffer_Clear(RingBuffer_t* rb)
{
    rb->head = 0;
    rb->tail = 0;
}

/*============================================================================*/
/* Public Functions                                                           */
/*============================================================================*/

HAL_StatusTypeDef UART_Handler_Init(UART_HandleTypeDef* huart)
{
    if (huart == NULL) {
        return HAL_ERROR;
    }
    
    uart_handle = huart;
    rx_callback = NULL;
    RingBuffer_Clear(&rx_ring);
    
    /* Start interrupt-based reception for single byte */
    HAL_UART_Receive_IT(uart_handle, &rx_byte, 1);
    
    return HAL_OK;
}

void UART_Handler_SetRxCallback(UART_RxCallback_t callback)
{
    rx_callback = callback;
}

HAL_StatusTypeDef UART_Handler_Send(const uint8_t* data, uint16_t len, uint32_t timeout_ms)
{
    if (uart_handle == NULL || data == NULL || len == 0) {
        return HAL_ERROR;
    }
    
    return HAL_UART_Transmit(uart_handle, (uint8_t*)data, len, timeout_ms);
}

uint16_t UART_Handler_Read(uint8_t* buffer, uint16_t max_len)
{
    if (buffer == NULL || max_len == 0) {
        return 0;
    }
    
    uint16_t count = 0;
    uint8_t byte;
    
    while (count < max_len && RingBuffer_Pop(&rx_ring, &byte)) {
        buffer[count++] = byte;
    }
    
    return count;
}

uint16_t UART_Handler_Available(void)
{
    return RingBuffer_Count(&rx_ring);
}

void UART_Handler_Process(void)
{
    /* Check if callback is set and data is available */
    if (rx_callback != NULL && !RingBuffer_IsEmpty(&rx_ring)) {
        /* Read all available data into temporary buffer */
        static uint8_t temp_buffer[UART_RX_BUFFER_SIZE];
        uint16_t len = UART_Handler_Read(temp_buffer, UART_RX_BUFFER_SIZE);
        
        if (len > 0) {
            rx_callback(temp_buffer, len);
        }
    }
}

void UART_Handler_RxCpltCallback(UART_HandleTypeDef* huart)
{
    if (huart == uart_handle) {
        /* Store received byte in ring buffer */
        RingBuffer_Push(&rx_ring, rx_byte);
        
        /* Re-enable interrupt for next byte */
        HAL_UART_Receive_IT(uart_handle, &rx_byte, 1);
    }
}

void UART_Handler_ClearRxBuffer(void)
{
    RingBuffer_Clear(&rx_ring);
}
