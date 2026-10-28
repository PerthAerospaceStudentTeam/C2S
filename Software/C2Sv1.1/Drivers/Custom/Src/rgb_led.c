/*
 * rgb_led.c
 *
 *  Created on: 25 Sept 2026
 *      Author: felix
 */

#include "rgb_led.h"

#define NS_TO_CYC(ns)  ((uint32_t)(((uint64_t)(ns) * SystemCoreClock) / 1000000000ULL))

#define T0H  NS_TO_CYC(320)
#define T1H  NS_TO_CYC(640)
#define TBIT NS_TO_CYC(1250)

static inline void wait_until(uint32_t t)
{
    while ((int32_t)(DWT->CYCCNT - t) < 0) { }
}

static uint32_t c_t0h, c_t1h, c_bit;

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
    HAL_Delay(1);                                    // >= 80 us reset before first frame

    RGB_SetColour(0, 0, 0);                          // force LED off
}

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

    HAL_Delay(1);   // >= 80 us low: latch
}
