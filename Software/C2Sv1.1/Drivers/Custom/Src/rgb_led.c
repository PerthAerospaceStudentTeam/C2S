/*
 * rgb_led.c
 *
 *  Created on: 25 Sept 2026
 *      Author: felix
 */

#include "rgb_led.h"

#include "cmsis_os2.h"

/** @brief Convert nanoseconds to CPU cycles at the current @c SystemCoreClock. */
#define NS_TO_CYC(ns)  ((uint32_t)(((uint64_t)(ns) * SystemCoreClock) / 1000000000ULL))

#define T0H  NS_TO_CYC(320)   /**< High time for a 0 bit (cycles). */
#define T1H  NS_TO_CYC(640)   /**< High time for a 1 bit (cycles). */
#define TBIT NS_TO_CYC(1250)  /**< Total bit period (cycles). */

extern osKernelState_t KernelState;

/** @brief Busy-wait until @c DWT->CYCCNT reaches @p t (wrap-safe). */
static inline void wait_until(uint32_t t)
{
    while ((int32_t)(DWT->CYCCNT - t) < 0) { }
}

static uint32_t c_t0h, c_t1h, c_bit;   /**< Cached bit timings in cycles. */

/**
 * @brief Initialise the DWT cycle counter and turn the LED off.
 * @note  Calls osDelay(), so the kernel must be running.
 */
void RGB_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->LAR = 0xC5ACCE55;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    c_t0h = NS_TO_CYC(320);
    c_t1h = NS_TO_CYC(640);
    c_bit = NS_TO_CYC(1250);

    LED_GPIO_Port->BSRR = (uint32_t)LED_Pin << 16;   // idle low
    osDelay(1); // >= 80 us reset before first frame

    RGB_SetColour(0, 0, 0);                          // force LED off
}

/**
 * @brief Set the LED colour (24-bit GRB frame, MSB first).
 * @param red    Red level, 0-255.
 * @param green  Green level, 0-255.
 * @param blue   Blue level, 0-255.
 * @note  Interrupts are disabled while the frame is sent; calls osDelay()
 *        afterwards to latch, so do not call from an ISR.
 */
void RGB_SetColour(uint8_t red, uint8_t green, uint8_t blue)
{
    uint32_t frame = ((uint32_t)green << 16) | ((uint32_t)red << 8) | blue;
    const uint32_t set_mask = LED_Pin;
    const uint32_t clr_mask = (uint32_t)LED_Pin << 16;

    __disable_irq();
    for (int i = 23; i >= 0; i--)
    {
        uint32_t high = ((frame >> i) & 1U) ? c_t1h : c_t0h;  // before timing starts

        uint32_t start = DWT->CYCCNT;
        LED_GPIO_Port->BSRR = set_mask;
        while ((DWT->CYCCNT - start) < high) { }
        LED_GPIO_Port->BSRR = clr_mask;
        while ((DWT->CYCCNT - start) < c_bit) { }
    }
    __enable_irq();

    // >= 80 us low: latch
    osDelay(1);
}
