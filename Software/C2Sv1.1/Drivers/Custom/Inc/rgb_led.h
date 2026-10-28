/*
 * rgb_led.h
 *
 *  Created on: 25 Sept 2026
 *      Author: felix
 */

#ifndef CUSTOM_INC_RGB_LED_H_
#define CUSTOM_INC_RGB_LED_H_

#include "config.h"
#include "main.h"

// Macros for defined colour states
#define RGB_Error_Fatal() RGB_SetColour(2,0,0)		// RED
#define RGB_Error_microSD() RGB_SetColour(2,0,2)	// PURPLE
#define RGB_Error_Camera() RGB_SetColour(2,1,0)		// ORANGE/YELLOW
#define RGB_Nominal() RGB_SetColour(0,0,2)			// BLUE
#define RGB_NominalPaired() RGB_SetColour(0,2,0)	// GREEN

void RGB_Init(void);
void RGB_SetColour(uint8_t red, uint8_t green, uint8_t blue);


#endif /* CUSTOM_INC_RGB_LED_H_ */
