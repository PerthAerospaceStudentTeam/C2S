/*
 * ov7670Reg.h
 *
 *  Created on: 2017/08/25
 *      Author: felix
 */

#ifndef OV7670_CONFIG_H_
#define OV7670_CONFIG_H_

#include <stdint.h>
#include "ov7670.h"

#define REG_EOF 0xFF

const uint8_t System_Config[][2] = {

		//{OV7670_CLKRC_ADDRESS, 0x80}, 		// pre-scalar = 1/1
		{OV7670_MVFP_ADDRESS, 0x01}, 		// H Flip

		// Scale PCLK
		{OV7670_CLKRC_ADDRESS, 0xA7}, // Divide by 40
//		{OV7670_COM14_ADDRESS, 0x11},
//		{OV7670_SCALING_PCLK_DIV_ADDRESS, 0x01},
		//{OV7670_, },

//		{0x0C, 0x04},  // DCW enable
//		{0x3E, 0x19},  // manual scaling, pclk/=2
//		{0x70, 0x3A},  // scaling_xsc
//		{0x71, 0x35},  // scaling_ysc
//		{0x72, 0x11}, // down sample by 2
//		{0x73, 0xf1}, // DSP clock /= 2

		{REG_EOF, REG_EOF},
};

const uint8_t QVGA_Config[][2] = {

		// Windowing
		{OV7670_HSTART_ADDRESS, 0x14}, 		// HSTART
		{OV7670_HSTOP_ADDRESS, 0x02}, 		// HSTOP
		{OV7670_HREF_ADDRESS, 0x80}, 		// HREF

		{OV7670_VSTART_ADDRESS, 0x02},   	// VSTART
		{OV7670_VSTOP_ADDRESS, 0x7A},   	// VSTOP
		{OV7670_VREF_ADDRESS, 0x0A},   		// VREF

		{OV7670_COM4_ADDRESS, 0x00},
		{OV7670_COM17_ADDRESS, 0x00},

		{REG_EOF, REG_EOF},
};

const uint8_t VGA_Config[][2] = {

		// Windowing
		{OV7670_HSTART_ADDRESS, 0x13}, 		// HSTART
		{OV7670_HSTOP_ADDRESS, 0x01}, 		// HSTOP
		{OV7670_HREF_ADDRESS, 0xB6}, 		// HREF

		{OV7670_VSTART_ADDRESS, 0x03},   	// VSTART
		{OV7670_VSTOP_ADDRESS, 0x7B},   	// VSTOP
		{OV7670_VREF_ADDRESS, 0x0A},   		// VREF

		{OV7670_COM4_ADDRESS, 0x00},
		{OV7670_COM17_ADDRESS, 0x00},

		{REG_EOF, REG_EOF},
};

const uint8_t RGB565_Config[][2] = {

		// Colour Configuration
		{OV7670_RGB444_ADDRESS, 0x00},   	// RGB444 Disable
		{OV7670_COM15_ADDRESS, 0xD0},   	// RGB565, 00 - FF
		{OV7670_TSLB_ADDRESS, 0x04},   		// UYVY, Dynamic Resolution Change (bits 1&2 reserved as 01)
		{OV7670_COM13_ADDRESS, 0x88},   	// Gamma Enable, UV auto adjust, UYVY
		{OV7670_COM11_ADDRESS, 0xE0},
		{OV7670_RSVD_B0_ADDRESS, OV7670_RSVD_B0_RESET_VALUE}, // DO NOT CLEAR (Inverts colours if removed)

		// AGC, AEC, AWB
		{OV7670_GAIN_ADDRESS, 0x04},
		{OV7670_COM8_ADDRESS, 0x4A},		// Fast AGC/AEC, AWB Enabled
		{OV7670_COM9_ADDRESS, 0x4A},   		// AGC Ceiling = 32x
		{OV7670_COM16_ADDRESS, 0x38},   	// edge enhancement, de-noise, AWG gain enabled

		// Exposure Timing (16-bit controller, MSB to LSB)
		{OV7670_AECHH_ADDRESS, 0x00}, 	// [5:0]
		{OV7670_AECH_ADDRESS, 0x06},	// [7:0]
		{OV7670_COM1_ADDRESS, 0x00},	// [1:0]

		// Colour Matrix
		{OV7670_MTX1_ADDRESS, 0xAC},
		{OV7670_MTX2_ADDRESS, 0x90},
		{OV7670_MTX3_ADDRESS, 0x20},
		{OV7670_MTX4_ADDRESS, 0x2D},
		{OV7670_MTX5_ADDRESS, 0x60},
		{OV7670_MTX6_ADDRESS, 0xE0},
		{OV7670_MTXS_ADDRESS, 0x9E},

		{REG_EOF, REG_EOF},
};

const uint8_t Grayscale_Config[][2] = { // YUV422 format, 2 input bytes, read one for grayscale. read every second byte in gpdma request?


		{REG_EOF, REG_EOF},
};

const uint8_t YCbCr_Config[][2] = {


		{REG_EOF, REG_EOF},
};

const uint8_t Test_Config[][2] = {

		// (XSC[7], YSC[7]) == (1,0) "8-bar colour bar)
		{OV7670_SCALING_XSC_ADDRESS, 0x3A | 0x80},
		{OV7670_SCALING_YSC_ADDRESS, 0x35 | 0x80},
		{OV7670_COM17_ADDRESS, 0x08}, // Colour Bar

		{REG_EOF, REG_EOF},
};

#endif /* OV7670_CONFIG_H_ */
