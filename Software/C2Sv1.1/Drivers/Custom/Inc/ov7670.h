/**
 * @file OV7670.h
 * @brief Main header file for the OV7670 camera module.
 *
 * @details pinkeee @ j.t0dd@protonmail.com
 *          take-iwiw
 * @note See README for contributing guides and info.
 * @copyright See License file.
 */

#ifndef OV7670_H
#define OV7670_H

#include <stdint.h>
#include "main.h"

// Code below is adapted from take-iwiw

typedef uint32_t RET;

// 320 * 240 pixels. 2 pixels per word. 4 Bytes per word.
#define MAX_PICTURE_BUFF (320 * 240 * 2 / 4)

#define SLAVE_ADDR 0x42

#define RET_OK           0x00000000
#define RET_NO_DATA      0x00000001
#define RET_DO_NOTHING   0x00000002
#define RET_ERR          0x80000001
#define RET_ERR_OF       0x80000002
#define RET_ERR_TIMEOUT  0x80000004
#define RET_ERR_STATUS   0x80000008
#define RET_ERR_PARAM    0x80000010
#define RET_ERR_FILE     0x80000020
#define RET_ERR_MEMORY   0x80000040

RET OV7670_Init(DCMI_HandleTypeDef *p_hdcmi, DMA_HandleTypeDef *p_hdma_dcmi, I2C_HandleTypeDef *p_hi2c);
RET OV7670_Config(uint8_t colour_mode, uint8_t resolution_mode, uint8_t test_pattern_mode);
void OV7670_RegisterCallback(void (*cbHsync)(uint32_t h), void (*cbVsync)(uint32_t v));

// Code below is from pinkeee

/**
 *  @brief SCCB camera write slave address
 *  @return 0x42
 */
#define OV7670_WRITE_ADDRESS (uint8_t)0x42

/**
 * @brief SCCB camera read slave address
 * @return 0x43
 */
#define OV7670_READ_ADDRESS (uint8_t)0x43

/**
 * @brief Timeout for whole driver coms
 * @return 15seconds
 */
#define OV7670_TIMEOUT (uint32_t)15

/* Common Control registers (COM) */

/**
 * @brief COM1 RW register
 * @details Common Control 1
 *  Bit[7]: Reserved
 *  Bit[6]: CCIR656 format
 *           0: Disable
 *           1: Enable
 *  Bit[5:2]: Reserved
 *  Bit[1:0]:
 *  AEC low 2 LSB (see registers AECHH for AEC[15:10]
 *      and AECH for AEC[9:2])
 * @return 0x04
 */
#define OV7670_COM1_ADDRESS (uint8_t)0x04

/**
 * @brief COM2 RW register
 * @details Common Control 2
 *  Bit[7:5]: Reserved
 *  Bit[4]: Soft sleep mode
 *  Bit[3:2]: Reserved
 *  Bit[1:0]: Output Drive Capability
 *      00: 1x
 *      01: 2x
 *      10: 3x
 *      11: 4x
 * @return 0x09
 */
#define OV7670_COM2_ADDRESS (uint8_t)0x09

/**
 * @brief COM3 RW register
 * @details Common Control 3
 *  Bit[7]: Reserved
 *  Bit[6]: Output data MSB and LSB swap
 *  Bit[5]: Tri-state option for output clock (PCLK, HREF/HSYNC,
 *          VSYNC, and STROBE) at power-down period
 *          0: Tri-state at this period
 *          1: No tri-state at this period
 *  Bit[4]: Tri-state option for output data (D[7:0]) at power-down
 *          period
 *          0: Tri-state at this period
 *          1: No tri-state at this period
 *  Bit[3]: Scale enable
 *          0: Disable
 *          1: Enable - if set to a pre-defined format (see
 *          COM7[5:3]), then COM14[3] must be set to 1 for
 *          manual adjustment.
 *  Bit[2]: DCW enable
 *          0: Disable
 *          1: Enable - if set to a pre-defined format (see
 *          COM7[5:3]), then COM14[3] must be set to 1 for
 *          manual adjustment.
 *  Bit[1:0]: Reserved
 * @return 0x0C
 */
#define OV7670_COM3_ADDRESS (uint8_t)0x0C

/**
 * @brief COM4 RW register
 * @details Common Control 4
 *  Bit[7:6]: Reserved
 *  Bit[5:4]: Average option (must be same value as COM17[7:6])
 *          00: Full window
 *          01: 1/2 window
 *          10: 1/4 window
 *          11: 1/4 window
 *  Bit[3:0]: Reserved
 * @return 0x0D
 */
#define OV7670_COM4_ADDRESS (uint8_t)0x0D

/**
 * @brief COM5 RW register
 * @details Common Control 5
 *  Bit[7:0]: Reserved
 * @return 0x0E
 */
#define OV7670_COM5_ADDRESS (uint8_t)0x0E

/**
 * @brief COM6 RW register
 * @details Common Control 6
 *  Bit[7]: Output of optical black row option
 *          0: Disable HREF at optical black
 *          1: Enable HREF at optical black
 *  Bit[6:2]: Reserved
 *  Bit[1]: Reset all timing when format changes
 *          0: No reset
 *          1: Resets timing
 *  Bit[0]: Reserved
 * @return 0x0F
 */
#define OV7670_COM6_ADDRESS (uint8_t)0x0F

/**
 * @brief COM7 RW register
 * @details Common Control 7
 *  Bit[7]: SCCB register reset
 *          0: No change
 *          1: Resets all registers to default values
 *  Bit[6]: Reserved
 *  Bit[5]: Output format - CIF selection
 *  Bit[4]: Output format - QVGA selection
 *  Bit[3]: Output format - QCIF selection
 *  Bit[2]: Output format - RGB selection (see below)
 *  Bit[1]: Color bar
 *          0: Disable
 *          1: Enable
 *  Bit[0]: Output format - Raw RGB (see below)
 *                           COM7[2]         COM7[0]
 *  YUV                        0               0
 *  RGB                        1               0
 *  Raw Bayer RGB              0               1
 *  Processed Bayer RGB        1               1
 * @return 0x12
 */
#define OV7670_COM7_ADDRESS (uint8_t)0x12

/**
 * @brief COM8 RW register
 * @details Common Control 8
 *  Bit[7]: Enable fast AGC/AEC algorithm
 *  Bit[6]: AEC - Step size limit
 *          0: Step size is limited to vertical blank
 *          1: Unlimited step size
 *  Bit[5]: Banding filter ON/OFF - In order to turn ON the banding
 *          filter, BD50ST (0x9D) or BD60ST (0x9E) must be set to a
 *          non-zero value.
 *          0: ON
 *          1: OFF
 *  Bit[4:3]: Reserved
 *  Bit[2]: AGC Enable
 *  Bit[1]: AWB Enable
 *  Bit[0]: AEC Enable
 * @return 0x13
 */
#define OV7670_COM8_ADDRESS (uint8_t)0x13

/**
 * @brief COM9 RW register
 * @details Common Control 9
 *  Bit[7]: Reserved
 *  Bit[6:4]: Automatic Gain Ceiling - maximum AGC value
 *            000: 2x
 *            001: 4x
 *            010: 8x
 *            011: 16x
 *            100: 32x
 *            101: 64x
 *            110: 128x
 *            111: Not allowed
 *  Bit[3:1]: Reserved
 *  Bit[0]: Freeze AGC/AEC
 * @return 0x14
 */
#define OV7670_COM9_ADDRESS (uint8_t)0x14

/**
 * @brief COM10 RW register
 * @details Common Control 10
 *  Bit[7]: Reserved
 *  Bit[6]: HREF changes to HSYNC
 *  Bit[5]: PCLK output option
 *          0: Free running PCLK
 *          1: PCLK does not toggle during horizontal blank
 *  Bit[4]: PCLK reverse
 *  Bit[3]: HREF reverse
 *  Bit[2]: VSYNC option
 *          0: VSYNC changes on falling edge of PCLK
 *          1: VSYNC changes on rising edge of PCLK
 *  Bit[1]: VSYNC negative
 *  Bit[0]: HSYNC negative
 * @return 0x15
 */
#define OV7670_COM10_ADDRESS (uint8_t)0x15

/**
 * @brief COM11 RW register
 * @details Common Control 11
 *  Bit[7]: Night mode
 *          0: Night mode disable
 *          1: Night mode enable - The frame rate is reduced
 *          automatically while the minimum frame rate is limited
 *          by COM11[6:5]. Also, ADVFH and ADVFL will be
 *          automatically updated.
 *  Bit[6:5]: Minimum frame rate of night mode
 *            00: Same as normal mode frame rate
 *            01: 1/2 of normal mode frame rate
 *            10: 1/4 of normal mode frame rate
 *            11: 1/8 of normal mode frame rate
 *  Bit[4]: D56_Auto
 *          0: Disable 50/60 Hz auto detection
 *          1: Enable 50/60 Hz auto detection
 *  Bit[3]: Banding filter value select (effective only when
 *          COM11[4] (uint8_t)0)
 *          0: Select BD60ST[7:0] (0x9E) as Banding Filter Value
 *          1: Select BD50ST[7:0] (0x9D) as Banding Filter Value
 *  Bit[2]: Reserved
 *  Bit[1]: Exposure timing can be less than limit of banding filter
 *          when light is too strong
 *  Bit[0]: Reserved
 * @return 0x3B
 */
#define OV7670_COM11_ADDRESS (uint8_t)0x3B

/**
 * @brief COM12 RW register
 * @details Common Control 12
 *  Bit[7]: HREF option
 *          0: No HREF when VSYNC is low
 *          1: Always has HREF
 *  Bit[6:0]: Reserved
 * @return 0x3C
 */
#define OV7670_COM12_ADDRESS (uint8_t)0x3C

/**
 * @brief COM13 RW register
 * @details Common Control 13
 *  Bit[7]: Gamma enable
 *  Bit[6]: UV saturation level - UV auto adjustment. Result is saved
 *          in register SATCTR[3:0] (0xC9)
 *  Bit[5:2]: Reserved
 *  Bit[1]: UV swap (use with register TSLB[3] (0x3A))
 *          TSLB[3], COM13[1]:
 *          00: Y U Y V
 *          01: Y V Y U
 *          10: U Y V Y
 *          11: V Y U Y
 *   Bit[0]: Reserved
 * @return 0x3D
 */
#define OV7670_COM13_ADDRESS (uint8_t)0x3D

/**
 * @brief COM14 RW register
 * @details Common Control 14
 *  Bit[7:5]: Reserved
 *  Bit[4]: DCW and scaling PCLK enable
 *          0: Normal PCLK
 *          1: DCW and scaling PCLK, controlled by register
 *          COM14[2:0] and SCALING_PCLK_DIV[3:0] (0x73))
 *  Bit[3]: Manual scaling enable for pre-defined resolution modes
 *          such as CIF, QCIF, and QVGA
 *          0: Scaling parameter cannot be adjusted manually
 *          1: Scaling parameter can be adjusted manually
 *  Bit[2:0]: PCLK divider (only when COM14[4] (uint8_t)1)
 *            000: Divided by 1
 *            001: Divided by 2
 *            010: Divided by 4
 *            011: Divided by 8
 *            100: Divided by 16
 *            101~111: Not allowed
 * @return 0x3E
 */
#define OV7670_COM14_ADDRESS (uint8_t)0x3E

/**
 * @brief COM15 RW register
 * @details Common Control 15
 *  Bit[7:6]: Data format - output full range enable
 *            0x: Output range: [10] to [F0]
 *            10: Output range: [01] to [FE]
 *            11: Output range: [00] to [FF]
 *  Bit[5:4]: RGB 555/565 option (must set COM7[2] (uint8_t)1 and
 *            COM7[0] (uint8_t)0)
 *            x0: Normal RGB output
 *            01: RGB 565
 *            11: RGB 555
 *  Bit[3:0]: Reserved
 * @return 0x40
 */
#define OV7670_COM15_ADDRESS (uint8_t)0x40

/**
 * @brief COM16 RW register
 * @details Common Control 16
 *  Bit[7:6]: Reserved
 *  Bit[5]: Enable edge enhancement auto-adjustment for YUV
 *          output (result is saved in register EDGE[4:0] (0x3F) and
 *          range is controlled by registers REG75[4:0] (0x75) and
 *          REG76[4:0] (0x76))
 *          0: Disable
 *          1: Enable
 *  Bit[4]: De-noise auto-adjustment (result is saved in register
 *          DNSTH (0x4C) and range is controlled by register
 *          REG77[7:0] (0x77))
 *          0: Disable
 *          1: Enable
 *  Bit[3]: AWB gain enable
 *  Bit[2]: Reserved
 *  Bit[1]: Color matrix coefficient double option
 *          0: Original matrix
 *          1: Double of original matrix
 *  Bit[0]: Reserved
 * @return 0x41
 */
#define OV7670_COM16_ADDRESS (uint8_t)0x41

/**
 * @brief COM17 RW register
 * @details Common Control 17
 * Bit[7:6]: AEC window must be the same value as COM4[5:4])
 *           00: Normal
 *           01: 1/2
 *           10: 1/4
 *           11: 1/4
 *  Bit[5:4]: Reserved
 *  Bit[3]: DSP color bar enable
 *          0: Disable
 *          1: Enable
 *  Bit[2:0]: Reserved
 * @return 0x42
 */
#define OV7670_COM17_ADDRESS (uint8_t)0x42

/**
 * @brief Gain control setting
 * @details AGC – Gain control gain setting
 *  Bit[7:0]: AGC[7:0] (see VREF[7:6] (0x03) for AGC[9:8])
 *   • Range: [00] to [FF]
 * @return 0x00
 */
#define OV7670_GAIN_ADDRESS (uint8_t)0x00

/**
 * @brief Blue channel gain settings
 * @details AWB – Blue channel gain setting
 *  • Range: [00] to [FF]
 * @return 0x01
 */
#define OV7670_BLUE_GAIN_ADDRESS (uint8_t)0x01

/**
 * @brief Red channel gain settings
 * @details AWB – Red channel gain setting
 *  • Range: [00] to [FF]
 * @return 0x02
 */
#define OV7670_RED_GAIN_ADDRESS (uint8_t)0x02

/**
 * @brief Vertical Frame Control (VREF) register
 * @details Vertical Frame Control
 *  Bit[7:6]: AGC[9:8] (see GAIN[7:0] (0x00) for AGC[7:0])
 *  Bit[5:4]: Reserved
 *  Bit[3:2]: VREF end low 2 bits (high 8 bits at VSTOP[7:0]
 *  Bit[1:0]: VREF start low 2 bits (high 8 bits at VSTRT[7:0]
 * @return 0x03
 */
#define OV7670_VREF_ADDRESS (uint8_t)0x03

/**
 * @brief U/B Average Level register
 * @details U/B Average Level
 *  Automatically updated based on chip output format
 * @return 0x05
 */
#define OV7670_BAVE_ADDRESS (uint8_t)0x05

/**
 * @brief Y/Gb Average Level register
 * @details Y/Gb Average Level
 *  Automatically updated based on chip output format
 * @return 0x06
 */
#define OV7670_GBAVE_ADDRESS (uint8_t)0x06

/**
 * @brief Exposure Value - AEC MSB 5 bits
 * @details Exposure Value - AEC MSB 5 bits
 *  Bit[7:6]: Reserved
 *  Bit[5:0]: AEC[15:10] (see registers AECH for AEC[9:2] and COM1
 *            for AEC[1:0])
 * @return 0x07
 */
#define OV7670_AECHH_ADDRESS (uint8_t)0x07

/**
 * @brief V/R Average Level
 * @details V/R Average Level
 *  Automatically updated based on chip output format
 * @return 0x08
 */
#define OV7670_RAVE_ADDRESS (uint8_t)0x08

/**
 * @brief Product ID Number MSB (Read only)
 * @return 0x0A
 */
#define OV7670_PID_ADDRESS (uint8_t)0x0A

/**
 * @brief Product ID Number LSB (Read only)
 * @return 0x0A
 */
#define OV7670_VER_ADDRESS (uint8_t)0x0B

/**
 * @brief Exposure settings register
 * @details Exposure Value
 *  Bit[7:0]: AEC[9:2] (see registers AECHH for AEC[15:10] and
 *            COM1 for AEC[1:0])
 * @return 0x10
 */
#define OV7670_AECH_ADDRESS (uint8_t)0x10

/**
 * @brief CLKRC RW register
 * @details Internal Clock
 *  Bit[7]: Reserved
 *  Bit[6]: Use external clock directly (no clock pre-scale available)
 *  Bit[5:0]: Internal clock pre-scalar
 *  F(internal clock) (uint8_t)F(input clock)/(Bit[5:0]+1)
 *  •  Range: [0 0000] to [1 1111]
 * @return 0x11
 */
#define OV7670_CLKRC_ADDRESS (uint8_t)0x11

/**
 * @brief Reserved register (RSVD)
 * @return 0x16
 */
#define OV7670_RSVD_16_ADDRESS (uint8_t)0x16

/**
 * @brief Horizontal frame start high
 * @details Output Format - Horizontal Frame (HREF column) start
 *          high 8-bit (low 3 bits are at HREF[2:0])
 * @return 0x17
 */
#define OV7670_HSTART_ADDRESS (uint8_t)0x17

/**
 * @brief Horizontal frame end high
 * @details Output Format - Horizontal Frame (HREF column) end
 *          high 8-bit (low 3 bits are at HREF[5:3])
 * @return 0x18
 */
#define OV7670_HSTOP_ADDRESS (uint8_t)0x18

/**
 * @brief Vertical frame start register
 * @details Output Format - Vertical Frame (row) start high 8-bit (
 *          low 2 bits are at VREF[1:0])
 * @return 0x19
 */
#define OV7670_VSTART_ADDRESS (uint8_t)0x19

/**
 * @brief Vertical frame stop register
 * @details Output Format - Vertical Frame (row) end high 8-bit (
 *          low 2 bits are at VREF[3:2])
 * @return 0x1A
 */
#define OV7670_VSTOP_ADDRESS (uint8_t)0x1A

/**
 * @brief Pixel delay shift register
 * @details Data Format - Pixel Delay Select (delays timing of the D[7:0] data
 *          relative to HREF in pixel units)
 * • Range: [00] (no delay) to [FF] (256 pixel delay which accounts for
 *               whole array)
 * @return 0x1B
 */
#define OV7670_PSHIFT_ADDRESS (uint8_t)0x1B

/**
 * @brief Manufacturer ID Byte (High) (Read only (uint8_t)0x7F)
 * @return 0x1C
 */
#define OV7670_MIDH_ADDRESS (uint8_t)0x1C

/**
 * @brief Manufacturer ID Byte (Low) (Read only (uint8_t)0xA2)
 * @return 0x1D
 */
#define OV7670_MIDL_ADDRESS (uint8_t)0x1D

/**
 * @brief Mirror/VFlip image register
 * @details Mirror/VFlip Enable
 *  Bit[7:6]: Reserved
 *  Bit[5]: Mirror
 *          0: Normal image
 *          1: Mirror image
 *  Bit[4]: VFlip enable
 *          0: Normal image
 *         1 : Vertically flip image
 *  Bit[3]: Reserved
 *  Bit[2]: Black sun enable
 *  Bit[1:0]: Reserved
 * @return 0x1E
 */
#define OV7670_MVFP_ADDRESS (uint8_t)0x1E

/**
 * @brief LAEC register (Reserved)
 * @return 0x1F
 */
#define OV7670_LAEC_ADDRESS (uint8_t)0x1F

/**
 * @brief ADC control register 0
 * @details ADC Control
 *  Bit[7:4]: Reserved
 *  Bit[3]: ADC range adjustment
 *          0: 1x range
 *          1: 1.5x range
 *  Bit[2:0]: ADC reference adjustment
 *            000: 0.8x
 *            100: 1x
 *            111: 1.2x
 * @return 0x20
 */
#define OV7670_ADCCTR0_ADDRESS (uint8_t)0x20

/**
 * @brief ADC control register 1 (Reserved)
 * @return 0x21
 */
#define OV7670_ADCCTR1_ADDRESS (uint8_t)0x21

/**
 * @brief ADC control register 2 (Reserved)
 * @return 0x22
 */
#define OV7670_ADCCTR2_ADDRESS (uint8_t)0x22

/**
 * @brief ADC control register 3 (Reserved)
 * @return 0x23
 */
#define OV7670_ADCCTR3_ADDRESS (uint8_t)0x23

/**
 * @brief AGC/AEC - Stable Operating Region (Upper Limit)
 * @return 0x24
 */
#define OV7670_AEW_ADDRESS (uint8_t)0x24

/**
 * @brief AGC/AEC - Stable Operating Region (Lower Limit)
 * @return 0x25
 */
#define OV7670_AEB_ADDRESS (uint8_t)0x25

/**
 * @brief AGC/AEC Fast Mode Operating Region
 * @details AGC/AEC Fast Mode Operating Region
 *  Bit[7:4]: High nibble of upper limit of fast mode control zone
 *  Bit[3:0]: High nibble of lower limit of fast mode control zone
 * @return 0x26
 */
#define OV7670_VPT_ADDRESS (uint8_t)0x26

/**
 * @brief B channel signal bias
 * @details B Channel Signal Output Bias (effective only when COM6[3] (uint8_t)1)
 *  Bit[7]: Bias adjustment sign
 *          0: Add bias
 *          1: Subtract bias
 *  Bit[6:0]:  Bias value of 10-bit range
 * @return 0x27
 */
#define OV7670_BBIAS_ADDRESS (uint8_t)0x27

/**
 * @brief Gb channel signal bias
 * @details B Channel Signal Output Bias (effective only when COM6[3] (uint8_t)1)
 *  Bit[7]: Bias adjustment sign
 *          0: Add bias
 *          1: Subtract bias
 *  Bit[6:0]:  Bias value of 10-bit range
 * @return 0x28
 */
#define OV7670_GBBIAS_ADDRESS (uint8_t)0x28

/**
 * @brief RSVD (Reserved)
 * @return 0x29
 */
#define OV7670_RSVD_29_ADDRESS (uint8_t)0x29

/**
 * @brief Dummy Pixel Insert MSB
 * @details Dummy Pixel Insert MSB
 *  Bit[7:4]: 4 MSB for dummy pixel insert in horizontal direction
 *  Bit[3:2]: HSYNC falling edge delay 2 MSB
 *  Bit[1:0]: HSYNC rising edge delay 2 MSB
 * @return 0x2A
 */
#define OV7670_EXHCH_ADDRESS (uint8_t)0x2A

/**
 * @brief Dummy Pixel Insert LSB
 * @details 8 LSB for dummy pixel insert in horizontal direction
 * @return 0x2B
 */
#define OV7670_EXHCL_ADDRESS (uint8_t)0x2B

/**
 * @brief R Channel Signal Output Bias
 * @details R Channel Signal Output Bias (effective only when COM6[3] (uint8_t)1)
 *  Bit[7]: Bias adjustment sign
 *          0: Add bias
 *          1: Subtract bias
 *  Bit[6:0]: Bias value of 10-bit range
 */
#define OV7670_RBIAS_ADDRESS (uint8_t)0x2C

/**
 * @brief LSB of insert dummy rows in vertical direction (1 bit equals 1 row)
 * @return 0x2D
 */
#define OV7670_ADVFL_ADDRESS (uint8_t)0x2D

/**
 * @brief MSB of insert dummy rows in vertical direction
 * @return 0x2E
 */
#define OV7670_ADVFH_ADDRESS (uint8_t)0x2E

/**
 * @brief Y/G Channel Average Value
 * @return 0x2F
 */
#define OV7670_YAVE_ADDRESS (uint8_t)0x2F

/**
 * @brief HSYNC Rising Edge Delay (low 8 bits)
 * @return 0x30
 */
#define OV7670_HSYST_ADDRESS (uint8_t)0x30

/**
 * @brief HSYNC Falling Edge Delay (low 8 bits)
 * @return 0x31
 */
#define OV7670_HSYSN_ADDRESS (uint8_t)0x31

/**
 * @brief HREF Control register
 * @details HREF Control
 *  Bit[7:6]: HREF edge offset to data output
 *  Bit[5:3]: HREF end 3 LSB (high 8 MSB at register HSTOP)
 *  Bit[2:0]: HREF start 3 LSB (high 8 MSB at register HSTART)
 * @return 0x32
 */
#define OV7670_HREF_ADDRESS (uint8_t)0x32

/**
 * @brief Array Current Control (Reserved)
 * @return 0x33
 */
#define OV7670_CHLF_ADDRESS (uint8_t)0x33


/**
 * @brief Array Reference Control (Reserved)
 * @return 0x34
 */
#define OV7670_ARBLM_ADDRESS (uint8_t)0x34

/**
 * @brief Reserved 2
 * @return 0x35-0x36
 */
#define OV7670_RSVD_35_ADDRESS (uint8_t)0x35
#define OV7670_RSVD_36_ADDRESS (uint8_t)0x36

/**
 * @brief ADC control (Reserved)
 * @return 0x37
 */
#define OV7670_ADC_ADDRESS (uint8_t)0x37

/**
 * @brief ADC and Analog Common Mode Control (Reserved)
 * @return 0x38
 */
#define OV7670_ACOM_ADDRESS (uint8_t)0x38

/**
 * @brief ADC Offset Control
 * @return 0x39
 */
#define OV7670_OFON_ADDRESS (uint8_t)0x39

/**
 * @brief Line Buffer Test options register
 * @details Line Buffer Test Option
 *  Bit[7:6]: Reserved
 *  Bit[5]: Negative image enable
 *          0: Normal image
 *          1: Negative image
 *  Bit[4]: UV output value
 *          0: Use normal UV output
 *          1: Use fixed UV value set in registers MANU and MANV
 *             as UV output instead of chip output
 *  Bit[3]: Output sequence (use with register COM13[1] (0x3D))
 *          TSLB[3], COM13[1]:
 *          00: Y U Y V
 *          01: Y V Y U
 *          10: U Y V Y
 *          11: V Y U Y
 *  Bit[2:1]: Reserved
 *  Bit[0]: Auto output window
 *  0: Sensor DOES NOT automatically set window after
 *  resolution change. The companion backend
 *  processor can adjust the output window immediately
 *  after changing the resolution
 *
 *  1: Sensor automatically sets output window when
 *  resolution changes. After resolution changes, the
 *  companion backend processor must adjust the
 *  output window after the next VSYNC pulse.
 * @return 0x3A
 */
#define OV7670_TSLB_ADDRESS (uint8_t)0x3A

/**
 * @brief Edge Enhancement Adjustment
 * @details Edge Enhancement Adjustment
 *  Bit[7:5]: Reserved
 *  Bit[4:0]: Edge enhancement factor
 * @return 0x3F
 */
#define OV7670_EDGE_ADDRESS (uint8_t)0x3F

/**
 * @brief AWB Control 1
 * @return 0x43
 */
#define OV7670_AWBC1_ADDRESS (uint8_t)0x43

/**
 * @brief AWB Control 2
 * @return 0x44
 */
#define OV7670_AWBC2_ADDRESS (uint8_t)0x44

/**
 * @brief AWB Control 3
 * @return 0x45
 */
#define OV7670_AWBC3_ADDRESS (uint8_t)0x45

/**
 * @brief AWB Control 4
 * @return 0x46
 */
#define OV7670_AWBC4_ADDRESS (uint8_t)0x46

/**
 * @brief AWB Control 5
 * @return 0x47
 */
#define OV7670_AWBC5_ADDRESS (uint8_t)0x47

/**
 * @brief AWB Control 6
 * @return 0x48
 */
#define OV7670_AWBC6_ADDRESS (uint8_t)0x48

/**
 * @brief Reserved register 0x49-0x4A
 * @return 0x49-0x4A
 */
#define OV7670_RSVD_49_ADDRESS (uint8_t)0x49
#define OV7670_RSVD_4A_ADDRESS (uint8_t)0x4A

/**
 * @brief Register 4B
 * @details Register 4B
 *  Bit[7:1]: Reserved
 *  Bit[0]: UV average enable
 * @return 0x4B
 */
#define OV7670_REG4B_ADDRESS (uint8_t)0x4B

/**
 * @brief De-noise strength
 * @return 0x4C
 */
#define OV7670_DNSTH_ADDRESS (uint8_t)0x4C

/**
 * @brief Reserved register 0x4D-0x4E
 * @return 0x4D
 */
#define OV7670_RSVD_4D_ADDRESS (uint8_t)0x4D
#define OV7670_RSVD_4E_ADDRESS (uint8_t)0x4E

/**
 * @brief Matrix Coefficient 1
 * @return 0x4F
 */
#define OV7670_MTX1_ADDRESS (uint8_t)0x4F

/**
 * @brief Matrix Coefficient 2
 * @return 0x50
 */
#define OV7670_MTX2_ADDRESS (uint8_t)0x50

/**
 * @brief Matrix Coefficient 3
 * @return 0x51
 */
#define OV7670_MTX3_ADDRESS (uint8_t)0x51

/**
 * @brief Matrix Coefficient 4
 * @return 0x52
 */
#define OV7670_MTX4_ADDRESS (uint8_t)0x52

/**
 * @brief Matrix Coefficient 5
 * @return 0x53
 */
#define OV7670_MTX5_ADDRESS (uint8_t)0x53

/**
 * @brief Matrix Coefficient 6
 * @return 0x54
 */
#define OV7670_MTX6_ADDRESS (uint8_t)0x54

/**
 * @brief Brightness Control
 * @return 0x55
 */
#define OV7670_BRIGHT_ADDRESS (uint8_t)0x55

/**
 * @brief Contrast Control
 * @return 0x56
 */
#define OV7670_CONTRAS_ADDRESS (uint8_t)0x56

/**
 * @brief Contrast Center
 * @return 0x57
 */
#define OV7670_CONTRAS_CENTER_ADDRESS (uint8_t)0x57

/**
 * @brief Matrix Coefficient Sign register
 * @details Matrix Coefficient Sign for coefficient 5 to 0
 *  Bit[7]: Auto contrast center enable
 *          0: Disable, center is set by register
 *             CONTRAS-CENTER (0x57)
 *          1: Enable, register CONTRAS-CENTER is updated
 *             automatically
 *  Bit[6]: Reserved
 *  Bit[5:0]: Matrix coefficient sign
 *            0: Plus
 *            1: Minus
 * @return 0x58
 */
#define OV7670_MTXS_ADDRESS (uint8_t)0x58

/**
 * @brief RSVD (Reserved)
 * @return 0x59-0x61
 */
#define OV7670_RSVD_59_ADDRESS (uint8_t)0x59
#define OV7670_RSVD_5A_ADDRESS (uint8_t)0x5A
#define OV7670_RSVD_5B_ADDRESS (uint8_t)0x5B
#define OV7670_RSVD_5C_ADDRESS (uint8_t)0x5C
#define OV7670_RSVD_5D_ADDRESS (uint8_t)0x5D
#define OV7670_RSVD_5E_ADDRESS (uint8_t)0x5E
#define OV7670_RSVD_5F_ADDRESS (uint8_t)0x5F
#define OV7670_RSVD_60_ADDRESS (uint8_t)0x60
#define OV7670_RSVD_61_ADDRESS (uint8_t)0x61

/**
 * @brief Lens Correction Option 1
 * @details X Coordinate of Lens Correction Center
 *          Relative to Array Center
 * @return 0x62
 */
#define OV7670_LCC1_ADDRESS (uint8_t)0x62

/**
 * @brief Lens Correction Option 2
 * @details Y Coordinate of Lens Correction Center
 *          Relative to Array Center
 * @return 0x63
 */
#define OV7670_LCC2_ADDRESS (uint8_t)0x63

/**
 * @brief Lens Correction Option 3
 * @details G Channel Compensation Coefficient when LCC5[2] (0x66) is 1
 *          R, G, and B Channel Compensation Coefficient when LCC5[2] (0x66) is
 *          0
 * @return 0x64
 */
#define OV7670_LCC3_ADDRESS (uint8_t)0x64

/**
 * @brief Lens Correction Option 4
 * @details Radius of the circular section where no
             compensation applies
* @return 0x65
*/
#define OV7670_LCC4_ADDRESS (uint8_t)0x65

/**
 * @brief Lens Correction Control
 * @details Lens Correction Control
 *  Bit[7:3]: Reserved
 *  Bit[2]: Lens correction control select
 *          0: R, G, and B channel compensation coefficient is set
 *             by register LCC3
 *          1: R, G, and B channel compensation coefficient is set
 *             by registers LCC6, LCC3, and LCC7, respectively
 *  Bit[1]: Reserved
 *  Bit[0]: Lens correction enable
 *          0: Disable
 *          1: Enable
 * @return 0x66
 */
#define OV7670_LCC5_ADDRESS (uint8_t)0x66

/**
 * @brief Manual U Value (effective only when register TSLB[4] is high)
 * @return 0x67
 */
#define OV7670_MANU_ADDRESS (uint8_t)0x67

/**
 * @brief Manual V Value (effective only when register TSLB[4] is high)
 * @return 0x68
 */
#define OV7670_MANV_ADDRESS (uint8_t)0x68

/**
 * @brief Fixed Gain Control
 * @details Fix Gain Control
 *  Bit[7:6]: Fix gain for Gr channel
 *            00: 1x
 *            01: 1.25x
 *            10: 1.5x
 *            11: 1.75x
 *  Bit[5:4]: Fix gain for Gb channel
 *            00: 1x
 *            01: 1.25x
 *            10: 1.5x
 *            11: 1.75x
 *  Bit[3:2]: Fix gain for R channel
 *            00: 1x
 *            01: 1.25x
 *            10: 1.5x
 *            11: 1.75x
 *  Bit[1:0]: Fix gain for B channel
 *            00: 1x
 *            01: 1.25x
 *            10: 1.5x
 *            11: 1.75x
 * @return 0x69
 */
#define OV7670_GFIX_ADDRESS (uint8_t)0x69

/**
 * @brief G Channel AWB Gain
 * @return 0x6A
 */
#define OV7670_GGAIN_ADDRESS (uint8_t)0x6A

/**
 * @brief DBLV register
 * @details DBLV register
 *  Bit[7:6]: PLL control
 *            00: Bypass PLL
 *            01: Input clock x4
 *            10: Input clock x6
 *            11: Input clock x8
 *  Bit[5]: Reserved
 *  Bit[4]: Regulator control
 *          0: Enable internal regulator
 *          1: Bypass internal regulator
 *  Bit[3:0]: Clock divider control for DSP scale control (valid only
 *            when COM14[3] (uint8_t)1)
 * @return 0x6B
 */
#define OV7670_DBLV_ADDRESS (uint8_t)0x6B

/**
 * @brief AWB Control 3
 * return 0x6C
 */
#define OV7670_AWBCTR3_ADDRESS (uint8_t)0x6C

/**
 * @brief AWB Control 2
 * return 0x6D
 */
#define OV7670_AWBCTR2_ADDRESS (uint8_t)0x6D

/**
 * @brief AWB Control 1
 * return 0x6E
 */
#define OV7670_AWBCTR1_ADDRESS (uint8_t)0x6E

/**
 * @brief AWB Control 0
 * return 0x6F
 */
#define OV7670_AWBCTR0_ADDRESS (uint8_t)0x6F

/**
 * @brief SCALING_XSC register
 * @details SCALING_XSC register
 *  Bit[7]: Test_pattern[0] - works with test_pattern[1] test_pattern
 *          (SCALING_XSC[7], SCALING_YSC[7]):
 *          00: No test pattern output
 *          01: Shifting "1"
 *          10: 8-bar color bar
 *          11: Fade to gray color bar
 *  Bit[6:0]: Horizontal scale factor
 * @return 0x70
 */
#define OV7670_SCALING_XSC_ADDRESS (uint8_t)0x70

/**
 * @brief SCALING_YSC register
 * @details SCALING_YSC register
 *  Bit[7]: Test_pattern[1] - works with test_pattern[0] test_pattern
 *          (SCALING_XSC[7], SCALING_YSC[7]):
 *          00: No test pattern output
 *          01: Shifting "1"
 *          10: 8-bar color bar
 *          11: Fade to gray color bar
 *  Bit[6:0]: Vertical scale factor
 * @return 0x71
 */
#define OV7670_SCALING_YSC_ADDRESS (uint8_t)0x71

/**
 * @brief DCW Control register
 * @details DCW Control
 *  Bit[7]: Vertical average calculation option
 *          0: Vertical truncation
 *          1: Vertical rounding
 *  Bit[6]: Vertical down sampling option
 *          0: Vertical truncation
 *          1: Vertical rounding
 *  Bit[5:4]: Vertical down sampling rate
 *            00: No vertical down sampling
 *            01: Vertical down sample by 2
 *            10: Vertical down sample by 4
 *            11: Vertical down sample by 8
 *  Bit[3]: Horizontal average calculation option
 *          0: Horizontal truncation
 *          1: Horizontal rounding
 *  Bit[2]: Horizontal down sampling option
 *          0: Horizontal truncation
 *          1: Horizontal rounding
 *  Bit[1:0]: Horizontal down sampling rate
 *            00: No horizontal down sampling
 *            01: Horizontal down sample by 2
 *            10: Horizontal down sample by 4
 *            11: Horizontal down sample by 8
 * @return 0x72
 */
#define OV7670_SCALING_DCWCTR_ADDRESS (uint8_t)0x72

/**
 * @brief SCALING_PCLK_DIV register
 * @details SCALING_PCLK_DIV
 *  Bit[7:4]: Reserved
 *  Bit[3]: Bypass clock divider for DSP scale control
 *          0: Enable clock divider
 *          1: Bypass clock divider
 *  Bit[2:0]: Clock divider control for DSP scale control (valid only
 *            when COM14[3] (uint8_t)1). Should change with COM14[2:0].
 *            000: Divided by 1
 *            001: Divided by 2
 *            010: Divided by 4
 *            011: Divided by 8
 *            100: Divided by 16
 *            101~111: Not allowed
 * @return 0x73
 */
#define OV7670_SCALING_PCLK_DIV_ADDRESS (uint8_t)0x73

/**
 * @brief Register 74
 * @details REG74
 *  Bit[7:5]: Reserved
 *  Bit[4]: Digital gain control select
 *          0: Digital gain control by VREF[7:6]
 *          1: Digital gain control by REG74[1:0]
 *  Bit[3:2]: Reserved
 *  Bit[1:0]: Digital gain manual control
 *            00: Bypass
 *            01: 1x
 *            10: 2x
 *            11: 4x
 * @return 0x74
 */
#define OV7670_REG74_ADDRESS (uint8_t)0x74

/**
 * @brief Register 75
 * @details REG75
 *  Bit[7:5]: Reserved
 *  Bit[4:0]: Edge enhancement lower limit
 * @return 0x75
 */
#define OV7670_REG75_ADDRESS (uint8_t)0x75

/**
 * @brief Register 76
 * @details REG76
 *  Bit[7]: Reserved
 *  Bit[6]: White pixel correction enable
 *          0: Disable
 *          1: Enable
 *  Bit[5]: Black pixel correction enable
 *          0: Disable
 *          1: Enable
 *  Bit[4:0]: Edge enhancement higher limit
 * @return 0x76
 */
#define OV7670_REG76_ADDRESS (uint8_t)0x76

/**
 * @brief Register 77
 * @details REG77
 *  Bit[7:0]: Offset, de-noise range control
 * @return 0x77
 */
#define OV7670_REG77_ADDRESS (uint8_t)0x77

/**
 * @brief RSVD (Reserved)
 * @return 0x78-0x79
 */
#define OV7670_RSVD_78_ADDRESS (uint8_t)0x78
#define OV7670_RSVD_79_ADDRESS (uint8_t)0x79

/**
 * @brief SLOP register
 * @details Gamma Curve Highest Segment Slop - calculated as follows:
 *          SLOP[7:0] (uint8_t)(0x100 - GAM15[7:0]) x 4/3
 * @return 0x7A
 */
#define OV7670_SLOP_ADDRESS (uint8_t)0x7A

/**
 * @brief Gamma Curve register 1
 * @details Gamma Curve 1st Segment Input End Point 0x04 Output Value
 * @return 0x7B
 */
#define OV7670_GAM1_ADDRESS (uint8_t)0x7B

/**
 * @brief Gamma Curve register 2
 * @details Gamma Curve 2nd Segment Input End Point 0x08 Output Value
 * @return 0x7C
 */
#define OV7670_GAM2_ADDRESS (uint8_t)0x7C

/**
 * @brief Gamma Curve register 3
 * @details Gamma Curve 3rd Segment Input End Point 0x10 Output Value
 * @return 0x7D
 */
#define OV7670_GAM3_ADDRESS (uint8_t)0x7D

/**
 * @brief Gamma Curve register 4
 * @details Gamma Curve 4th Segment Input End Point 0x20 Output Value
 * @return 0x7E
 */
#define OV7670_GAM4_ADDRESS (uint8_t)0x7E

/**
 * @brief Gamma Curve register 5
 * @details Gamma Curve 5th Segment Input End Point 0x28 Output Value
 * @return 0x7F
 */
#define OV7670_GAM5_ADDRESS (uint8_t)0x7F

/**
 * @brief Gamma Curve register 6
 * @details Gamma Curve 6th Segment Input End Point 0x30 Output Value
 * @return 0x80
 */
#define OV7670_GAM6_ADDRESS (uint8_t)0x80

/**
 * @brief Gamma Curve register 7
 * @details Gamma Curve 7th Segment Input End Point 0x38 Output Value
 * @return 0x81
 */
#define OV7670_GAM7_ADDRESS (uint8_t)0x81

/**
 * @brief Gamma Curve register 8
 * @details Gamma Curve 8th Segment Input End Point 0x40 Output Value
 * @return 0x82
 */
#define OV7670_GAM8_ADDRESS (uint8_t)0x82

/**
 * @brief Gamma Curve register 9
 * @details Gamma Curve 9th Segment Input End Point 0x48 Output Value
 * @return 0x83
 */
#define OV7670_GAM9_ADDRESS (uint8_t)0x83

/**
 * @brief Gamma Curve register 10
 * @details Gamma Curve 10th Segment Input End Point 0x50 Output Value
 * @return 0x84
 */
#define OV7670_GAM10_ADDRESS (uint8_t)0x84

/**
 * @brief Gamma Curve register 11
 * @details Gamma Curve 11th Segment Input End Point 0x60 Output Value
 * @return 0x85
 */
#define OV7670_GAM11_ADDRESS (uint8_t)0x85

/**
 * @brief Gamma Curve register 12
 * @details Gamma Curve 12th Segment Input End Point 0x70 Output Value
 * @return 0x86
 */
#define OV7670_GAM12_ADDRESS (uint8_t)0x86

/**
 * @brief Gamma Curve register 13
 * @details Gamma Curve 13th Segment Input End Point 0x90 Output Value
 * @return 0x87
 */
#define OV7670_GAM13_ADDRESS (uint8_t)0x87

/**
 * @brief Gamma Curve register 14
 * @details Gamma Curve 14th Segment Input End Point 0xB0 Output Value
 * @return 0x88
 */
#define OV7670_GAM14_ADDRESS (uint8_t)0x88

/**
 * @brief Gamma Curve register 15
 * @details Gamma Curve 15th Segment Input End Point 0xD0 Output Value
 * @return 0x89
 */
#define OV7670_GAM15_ADDRESS (uint8_t)0x89

/**
 * @brief RSVD (Reserved)
 * @return 0x8A-0x8B
 */
#define OV7670_RSVD_8A_ADDRESS (uint8_t)0x8A
#define OV7670_RSVD_8B_ADDRESS (uint8_t)0x8B

/**
 * @brief RGB444 register
 * @details RGB444 register
 *  Bit[7:2]: Reserved
 *  Bit[1]: RGB444 enable, effective only when COM15[4] is high
 *          0: Disable
 *          1: Enable
 *  Bit[0]: RGB444 word format
 *          0: xR GB
 *          1: RG Bx
 * @return 0x8C
 */
#define OV7670_RGB444_ADDRESS (uint8_t)0x8C

/**
 * @brief RSVD (Reserved)
 * @return 0x8D-0x91
 */
#define OV7670_RSVD_8D_ADDRESS (uint8_t)0x8D
#define OV7670_RSVD_8E_ADDRESS (uint8_t)0x8E
#define OV7670_RSVD_8F_ADDRESS (uint8_t)0x8F
#define OV7670_RSVD_90_ADDRESS (uint8_t)0x90
#define OV7670_RSVD_91_ADDRESS (uint8_t)0x91

/**
 * @brief Dummy Row low 8 bits register
 * @return 0x92
 */
#define OV7670_DM_LNL_ADDRESS (uint8_t)0x92

/**z
 * @brief Dummy Row high 8 bits register
 * @return 0x93
 */
#define OV7670_DM_LNH_ADDRESS (uint8_t)0x93

/**
 * @brief Lens Correction Option 6 (effective only when LCC5[2] is high)
 * @return 0x94
 */
#define OV7670_LCC6_ADDRESS (uint8_t)0x94

/**
 * @brief Lens Correction Option 7 (effective only when LCC5[2] is high)
 * @return 0x95
 */
#define OV7670_LCC7_ADDRESS (uint8_t)0x95

/**
 * @brief RSVD (Reserved)
 * @return 0x96-0x9C
 */
#define OV7670_RSVD_96_ADDRESS (uint8_t)0x96
#define OV7670_RSVD_97_ADDRESS (uint8_t)0x97
#define OV7670_RSVD_98_ADDRESS (uint8_t)0x98
#define OV7670_RSVD_99_ADDRESS (uint8_t)0x99
#define OV7670_RSVD_9A_ADDRESS (uint8_t)0x9A
#define OV7670_RSVD_9B_ADDRESS (uint8_t)0x9B
#define OV7670_RSVD_9C_ADDRESS (uint8_t)0x9C

/**
 * @brief 50 Hz Banding Filter Value
 * @details 50 Hz Banding Filter Value
 *  (effective only when COM8[5] is high and COM11[3] is high)
 * @return 0x9D
 */
#define OV7670_BD50ST_ADDRESS (uint8_t)0x9D

/**
 * @brief 60 Hz Banding Filter Value
 * @details 60 Hz Banding Filter Value
 *  (effective only when COM8[5] is high and COM11[3] is low)
 * @return 0x9E
 */
#define OV7670_BD60ST_ADDRESS (uint8_t)0x9E

/**
 * @brief HAECC1 Histogram-based AEC/AGC Control 1
 * @return 0x9F
 */
#define OV7670_HAECC1_ADDRESS (uint8_t)0x9F

/**
 * @brief HAECC1 Histogram-based AEC/AGC Control 2
 * @return 0xA0
 */
#define OV7670_HAECC2_ADDRESS (uint8_t)0xA0

/**
 * @brief RSVD (Reserved)
 * @return 0xA1
 */
#define OV7670_RSVD_A1_ADDRESS (uint8_t)0xA1

/**
 * @brief Pixel Clock Delay
 * @return 0xA2
 */
#define OV7670_SCALING_PCLK_DELAY_ADDRESS (uint8_t)0xA2

/**
 * @brief RSVD (Reserved)
 * @return 0xA3
 */
#define OV7670_RSVD_A3_ADDRESS (uint8_t)0xA3

/**
 * @brief NT_CTRL register
 * @details NT_CTRL register
 *  Bit[7:4]: Reserved
 *  Bit[3]: Auto frame rate adjustment dummy row selection
 *          0: N is equal to the maximum exposure, time less than
 *             frame period when banding filter is enabled
 *          1: N is equal to the number of rows per frame
 *  Bit[2]: Reserved
 *  Bit[1:0]: Auto frame rate adjustment switch point
 *            00: Insert dummy row at 2x gain
 *            01: Insert dummy row at 4x gain
 *            10: Insert dummy row at 8x gain
 * @return 0xA4
 */
#define OV7670_NT_CTRL_ADDRESS (uint8_t)0xA4

/**
 * @brief BD50MAX register
 * @return 0xA5
 */
#define OV7670_BD50MAX_ADDRESS (uint8_t)0xA5

/**
 * @brief HAECC3 Histogram-based AEC/AGC Control 3
 * @return 0xA6
 */
#define OV7670_HAECC3_ADDRESS (uint8_t)0xA6

/**
 * @brief HAECC4 Histogram-based AEC/AGC Control 4
 * @return 0xA7
 */
#define OV7670_HAECC4_ADDRESS (uint8_t)0xA7

/**
 * @brief HAECC5 Histogram-based AEC/AGC Control 5
 * @return 0xA8
 */
#define OV7670_HAECC5_ADDRESS (uint8_t)0xA8

/**
 * @brief HAECC6 Histogram-based AEC/AGC Control 6
 * @return 0xA9
 */
#define OV7670_HAECC6_ADDRESS (uint8_t)0xA9

/**
 * @brief HAECC7 Histogram-based AEC/AGC Control 7
 * @return 0xAA
 */
#define OV7670_HAECC7_ADDRESS (uint8_t)0xAA

/**
 * @brief BD60MAX register
 * @return 0xAB
 */
#define OV7670_BD60MAX_ADDRESS (uint8_t)0xAB

/**
 * @brief STR-OPT register
 * @details Register AC
 *  Bit[7]:Strobe enable
 *  Bit[6]: R / G / B gain controlled by STR_R (0xAD) / STR_G
 *          (0xAE) / STR_B (0xAF) for LED output frame
 *  Bit[5:4]: Xenon mode option
 *            00: 1 row
 *            01: 2 rows
 *            10: 3 rows
 *            11: 4 rows
 *  Bit[3:2]: Reserved
 *  Bit[1:0]: Mode select
 *            00: Xenon
 *            01: LED 1
 *            1x: LED 2
 * @return 0xAC
 */
#define OV7670_STR_OPT_ADDRESS (uint8_t)0xAC

/**
 * @brief R Gain for LED Output Frame
 * @return 0xAD
 */
#define OV7670_STR_R_ADDRESS (uint8_t)0xAD

/**
 * @brief G Gain for LED Output Frame
 * @return 0xAE
 */
#define OV7670_STR_G_ADDRESS (uint8_t)0xAE

/**
 * @brief B Gain for LED Output Frame
 * @return 0xAF
 */
#define OV7670_STR_B_ADDRESS (uint8_t)0xAF

/**
 * @brief RSVD (Reserved)
 * @return 0xB0
 */
#define OV7670_RSVD_B0_ADDRESS (uint8_t)0xB0

/**
 * @brief ABLC1 register
 * @details ABLC1 register
 *  Bit[7:3]: Reserved
 *  Bit[2]: ABLC enable
 *          0: Disable ABLC function, BLC function is still active
 *          1: Enable ABLC function
 *  Bit[1:0]: Reserved
 * @return 0xB1
 */
#define OV7670_ABLC1_ADDRESS (uint8_t)0xB1

/**
 * @brief RSVD (Reserved)
 * @return 0xB2
 */
#define OV7670_RSVD_B2_ADDRESS (uint8_t)0xB2

/**
 * @brief ABLC Target
 * @return 0xB3
 */
#define OV7670_THL_ST_ADDRESS (uint8_t)0xB3

/**
 * @brief RSVD (Reserved)
 * @return 0xB4
 */
#define OV7670_RSVD_B4_ADDRESS (uint8_t)0xB4

/**
 * @brief ABLC Stable Range
 * @return 0xB5
 */
#define OV7670_THL_DLT_ADDRESS (uint8_t)0xB5

/**
 * @brief RSVD (Reserved)
 * @return 0xB6-0xBD
 */
#define OV7670_RSVD_B6_ADDRESS (uint8_t)0xB6
#define OV7670_RSVD_B7_ADDRESS (uint8_t)0xB7
#define OV7670_RSVD_B8_ADDRESS (uint8_t)0xB8
#define OV7670_RSVD_B9_ADDRESS (uint8_t)0xB9
#define OV7670_RSVD_BA_ADDRESS (uint8_t)0xBA
#define OV7670_RSVD_BB_ADDRESS (uint8_t)0xBB
#define OV7670_RSVD_BC_ADDRESS (uint8_t)0xBC
#define OV7670_RSVD_BD_ADDRESS (uint8_t)0xBD

/**
 * @brief Blue Channel Black Level Compensation
 * @details Blue Channel Black Level Compensation
 *  Bit[7]: Reserved
 *  Bit[6]: Sign bit
 *  Bit[5:0]: Blue channel black level compensation
 * @return 0xBE
 */
#define OV7670_AD_CHB_ADDRESS (uint8_t)0xBE

/**
 * @brief Red Channel Black Level Compensation
 * @details Red Channel Black Level Compensation
 *  Bit[7]: Reserved
 *  Bit[6]: Sign bit
 *  Bit[5:0]: Red channel black level compensation
 * @return 0xBF
 */
#define OV7670_AD_CHR_ADDRESS (uint8_t)0xBF

/**
 * @brief Gb Channel Black Level Compensation
 * @details Gb Channel Black Level Compensation
 *  Bit[7]: Reserved
 *  Bit[6]: Sign bit
 *  Bit[5:0]: Gb channel black level compensation
 * @return 0xC0
 */
#define OV7670_AD_CHGB_ADDRESS (uint8_t)0xC0

/**
 * @brief Gr Channel Black Level Compensation
 * @details Gr Channel Black Level Compensation
 *  Bit[7]: Reserved
 *  Bit[6]: Sign bit
 *  Bit[5:0]: Gr channel black level compensation
 * @return 0xC1
 */
#define OV7670_AD_CHGR_ADDRESS (uint8_t)0xC1

/**
 * @brief RSVD (Reserved)
 * @return 0xC2-0xC8
 */
#define OV7670_RSVD_C2_ADDRESS (uint8_t)0xC2
#define OV7670_RSVD_C3_ADDRESS (uint8_t)0xC3
#define OV7670_RSVD_C4_ADDRESS (uint8_t)0xC4
#define OV7670_RSVD_C5_ADDRESS (uint8_t)0xC5
#define OV7670_RSVD_C6_ADDRESS (uint8_t)0xC6
#define OV7670_RSVD_C7_ADDRESS (uint8_t)0xC7
#define OV7670_RSVD_C8_ADDRESS (uint8_t)0xC8

/**
 * @brief Saturation Control register
 * @details Saturation Control
 *  Bit[7:4]: UV saturation control minimum
 *  Bit[3:0]: UV saturation control result
 * @return 0xC9
 */
#define OV7670_SATCTR_ADDRESS (uint8_t)0xC9


/* Init values! */

/**
 * @brief Values for registers to be set at camera init.
 * @details For this camera, these values aren't really mentioned in the datasheet
 *          and instead contact has to be made with OmniVision to get the best
 *          values. Instead of doing that myself, these values are from the Linux
 *          kernel driver for this camera, these values came from OmniVision.
 *
 *          The camera init should be done in this order too. These settings
 *          give VGA VUYU, 30fps
 */

/**
 * @brief COM7 software reset value.
 * @details Writing 0x80 to the COM7 register sets
 *          all registers to their default value.
 *          Default does not mean optimal though.
 *
 *          This is recommended to be run at the
 *          very start of the driver init.
 * @return 0x80
 */
#define OV7670_COM7_SOFT_RESET (uint8_t)0x80

/**
 * @brief CLKRC 30fps clock scale
 * @details 0x1 == 0000 0001
 *  This value sets the internal clock pre-scaler
 * @return 0x1
 */
#define OV7670_CLKRC_RESET_VALUE (uint8_t)0x01

/**
 * @brief TSLB magic reserved bit
 * @details 0000 0100
 * @return 0x04
 */
#define OV7670_TSLB_RESET_VALUE (uint8_t)0x04

/**
 * @brief COM7 YUV reset value
 * @details 0000 0000
 * @return 0x0
 */
#define OV7670_COM7_RESET_VALUE (uint8_t)0x0

/**
 * @brief HSTART reset, value from OV doesn't make sense at all
 * @details 0001 0011
 * @return 0x13
 */
#define OV7670_HSTART_RESET_VALUE (uint8_t)0x13

/**
 * @brief HSTOP reset, value from OV doesn't make sense at all
 * @details 0000 0001
 * @return 0x01
 */
#define OV7670_HSTOP_RESET_VALUE (uint8_t)0x01

/**
 * @brief HREF reset, MSB bits from HSTART AND HSTOP
 * @details 1011 0110 , HREF edge offset (uint8_t)10
 * @return 0xB6
 */
#define OV7670_HREF_RESET_VALUE (uint8_t)0xB6

/**
 * @brief VSTART reset
 * @details 0001 0010
 * @return 0x02
 */
#define OV7670_VSTART_RESET_VALUE (uint8_t)0x02

/**
 * @brief VSTOP reset, this makes more sense compared to HSTOP.
 * @details 0111 1010
 * @return 0x7A
 */
#define OV7670_VSTOP_RESET_VALUE (uint8_t)0x7A

/**
 * @brief VREF reset, start low 2 bits from VSTOP/START
 * @details 0000 1010
 * @return 0x0A
 */
#define OV7670_VREF_RESET_VALUE (uint8_t)0x0A

/**
 * @brief COM3 reset value
 * @details 0000 0000 , nothing important
 * @return 0x0
 */
#define OV7670_COM14_RESET_VALUE (uint8_t)0x0

/**
 * @brief SCALING_XSC reset value
 * @details 0011 1010 , Max horizontal scale factor, no test pattern output
 * @return 0x3A
 */
#define OV7670_SCALING_XSC_RESET_VALUE (uint8_t)0x3A

/**
 * @brief SCALING_PCLK_DIV reset value
 * @details 1111 0000 , Reserved magic???
 * @return 0xF0
 */
#define OV7670_SCALING_PCLK_DIV_RESET_VALUE (uint8_t)0xF0

/**
 * @brief SLOP , Gamma curve highest segment slop
 * @details 0010 0000
 * @return 0x20
 */
#define OV7670_SLOP_RESET_VALUE (uint8_t)0x20

/**
 * @brief GAM1 Gamma curve 1
 * @details 0001 0000
 * @return 0x10
 */
#define OV7670_GAM1_RESET_VALUE (uint8_t)0x10

/**
 * @brief GAM2 Gamma curve 2
 * @details 0001 1110
 * @return 0x1E
 */
#define OV7670_GAM2_RESET_VALUE (uint8_t)0x1E

/**
 * @brief GAM3 Gamma curve 3
 * @details 0011 0101
 * @return 0x35
 */
#define OV7670_GAM3_RESET_VALUE (uint8_t)0x35

/**
 * @brief GAM4 Gamma curve 4
 * @details 0101 1010
 * @return 0x5A
 */
#define OV7670_GAM4_RESET_VALUE (uint8_t)0x5A

/**
 * @brief GAM5 Gamma curve 5
 * @details 0110 1001
 * @return 0x69
 */
#define OV7670_GAM5_RESET_VALUE (uint8_t)0x69

/**
 * @brief GAM6 Gamma curve 6
 * @details 0111 0110
 * @return 0x76
 */
#define OV7670_GAM6_RESET_VALUE (uint8_t)0x76

/**
 * @brief GAM7 Gamma curve 7
 * @details 1000 0000
 * @return 0x80
 */
#define OV7670_GAM7_RESET_VALUE (uint8_t)0x80

/**
 * @brief GAM8 Gamma curve 8
 * @details 1000 1000
 * @return 0x88
 */
#define OV7670_GAM8_RESET_VALUE (uint8_t)0x88

/**
 * @brief GAM9 Gamma curve 9
 * @details 1000 1111
 * @return 0x8F
 */
#define OV7670_GAM9_RESET_VALUE (uint8_t)0x8F

/**
 * @brief GAM10 Gamma curve 10
 * @details 1001 0110
 * @return 0x96
 */
#define OV7670_GAM10_RESET_VALUE (uint8_t)0x96

/**
 * @brief GAM11 Gamma curve 11
 * @details 1010 0011
 * @return 0xA3
 */
#define OV7670_GAM11_RESET_VALUE (uint8_t)0xA3

/**
 * @brief GAM12 Gamma curve 12
 * @details 1010 1111
 * @return 0xAF
 */
#define OV7670_GAM12_RESET_VALUE (uint8_t)0xAF

/**
 * @brief GAM13 Gamma curve 13
 * @details 1100 0100
 * @return 0xC4
 */
#define OV7670_GAM13_RESET_VALUE (uint8_t)0xC4

/**
 * @brief GAM14 Gamma curve 14
 * @details 1101 0111
 * @return 0xD7
 */
#define OV7670_GAM14_RESET_VALUE (uint8_t)0xD7

/**
 * @brief GAM15 Gamma curve 15
 * @details 1110 1000
 * @return 0xE8
 */
#define OV7670_GAM15_RESET_VALUE (uint8_t)0xE8

/**
 * @brief COM8 reset, disable AGC and AEC first. One/Two.
 * @details 1110 0000 , reserved magic, banding on, AEC step size, fast algorithm
 * @return 0xE0
 */
#define OV7670_COM8_RESET_VALUE_ONE (uint8_t)0xE0

/**
 * @brief COM4 reset, magic reserved 6th bit
 * @details 0100 0000
 * @return 0x40
 */
#define OV7670_COM4_RESET_VALUE (uint8_t)0x40

/**
 * @brief COM9 reset, magic reserved, 4x gain ceiling
 * @details 0001 1000
 * @return 0x18
 */
#define OV7670_COM9_RESET_VALUE (uint8_t)0x18

/**
 * @brief BD50MAX reset
 * @detais 0000 0101
 * @return 0x05
 */
#define OV7670_BD50MAX_RESET_VALUE (uint8_t)0x05

/**
 * @brief BD60MAX reset
 * @detais 0000 0111
 * @return 0x07
 */
#define OV7670_BD60MAX_RESET_VALUE (uint8_t)0x07

/**
 * @brief AEW reset
 * @details 1001 0101
 * @return 0x95
 */
#define OV7670_AEW_RESET_VALUE (uint8_t)0x95

/**
 * @brief AEB reset
 * @details 0011 00111
 * @return 0x33
 */
#define OV7670_AEB_RESET_VALUE (uint8_t)0x33

/**
 * @brief VPT reset
 * @details 1110 0011
 * @return 0xE3
 */
#define OV7670_VPT_RESET_VALUE (uint8_t)0xE3

/**
 * @brief HAECC1 reset
 * @details 0111 1000
 * @return 0x78
 */
#define OV7670_HAECC1_RESET_VALUE (uint8_t)0x78

/**
 * @brief HAECC2 reset
 * @details 0110 1000
 * @return 0x68
 */
#define OV7670_HAECC2_RESET_VALUE (uint8_t)0x68

/**
 * @brief Reserved magic again
 * @details 0000 0011
 * @return 0x03
 */
#define OV7670_RSVD_A1_RESET_VALUE (uint8_t)0x03

/**
 * @brief HAECC3 reset
 * @details 1101 1000
 * @return 0xD8
 */
#define OV7670_HAECC3_RESET_VALUE (uint8_t)0xD8

/**
 * @brief HAECC4 reset
 * @details 1101 1000
 * @return 0xD8
 */
#define OV7670_HAECC4_RESET_VALUE (uint8_t)0xD8

/**
 * @brief HAECC6 reset
 * @details 1001 0000
 * @return 0x90
 */
#define OV7670_HAECC6_RESET_VALUE (uint8_t)0x90

/**
 * @brief HAECC7 reset
 * @details 1001 0100
 * @return 0x94
 */
#define OV7670_HAECC7_RESET_VALUE (uint8_t)0x94

/**
 * @brief COM8 reset 2
 * @details 1110 0101 , Enables FAST AEC, AEC STEP, BFILT, AGC, AEC
 * @return 0xE5
 */
#define OV7670_COM8_RESET_VALUE_TWO (uint8_t)0xE5

/**
 * @brief COM5 reset
 * @details 0110 0001, magic reserved
 * @return 0x61
 */
#define OV7670_COM5_RESET_VALUE (uint8_t)0x61

/**
 * @brief COM6 reset
 * @details 0100 1011, magic reserved
 * @return 0x4B
 */
#define OV7670_COM6_RESET_VALUE (uint8_t)0x4B

/**
 * @brief RSVD reset
 * @details 0000 0010
 * @return 0x02
 */
#define OV7670_RSVD_16_RESET_VALUE (uint8_t)0x02

/**
 * @brief MVFP reset
 * @details 0000 0111
 * @return 0x07
 */
#define OV7670_MVFP_RESET_VALUE (uint8_t)0x07

/**
 * @brief ADCCTR2 reset
 * @details 1001 0001
 * @return 0x91
 */
#define OV7670_ADCCTR2_RESET_VALUE (uint8_t)0x91

/**
 * @brief RSVD reset
 * @details 0000 0111
 * @return 0x07
 */
#define OV7670_RSVD_29_RESET_VALUE (uint8_t)0x07

/**
 * @brief CHLF reset value, reserved
 * @details 0000 1011
 * @return 0x0B
 */
#define OV7670_CHLF_RESET_VALUE (uint8_t)0x0B

/**
 * @brief RSVD reset
 * @details 0000 1011
 * @return 0x0B
 */
#define OV7670_RSVD_35_RESET_VALUE (uint8_t)0x0B

/**
 * @brief ADC reset
 * @details 0001 1101
 * @return 0x1D
 */
#define OV7670_ADC_RESET_VALUE (uint8_t)0x1D

/**
 * @brief ACOM reset
 * @details 0111 0001
 * @return 0x71
 */
#define OV7670_ACOM_RESET_VALUE (uint8_t)0x71

/**
 * @brief OFON reset
 * @details 0010 1010
 * @return 0x2A
 */
#define OV7670_OFON_RESET_VALUE (uint8_t)0x2A

/**
 * @brief COM12 reset
 * @details 0111 1000, magic reserved
 * @return 0x78
 */
#define OV7670_COM12_RESET_VALUE (uint8_t)0x78

/**
 * @brief RSVD reset
 * @details 0100 0000
 * @return 0x40
 */
#define OV7670_RSVD_4D_RESET_VALUE (uint8_t)0x40

/**
 * @brief RSVD reset
 * @details 0010 0000
 * @return 0x20
 */
#define OV7670_RSVD_4E_RESET_VALUE (uint8_t)0x20

/**
 * @brief GFIX reset
 * @details 0000 0000
 * @return 0x0
 */
#define OV7670_GFIX_RESET_VALUE (uint8_t)0x0

/**
 * @brief DBLV reset
 * @details 0100 1010
 * @return 0x4A
 */
#define OV7670_DBLV_RESET_VALUE (uint8_t)0x4A

/**
 * @brief REG74 reset
 * @details 0001 0000
 * @return 0x10
 */
#define OV7670_REG74_RESET_VALUE (uint8_t)0x10

/**
 * @brief RSVD reset
 * @details 0100 1111
 * @return 0x4F
 */
#define OV7670_RSVD_8D_RESET_VALUE (uint8_t)0x4F

/**
 * @brief RSVD reset
 * @details 0000 0000
 * @return 0x0
 */
#define OV7670_RSVD_8E_RESET_VALUE (uint8_t)0x0

/**
 * @brief RSVD reset
 * @details 0000 0000
 * @return 0x0
 */
#define OV7670_RSVD_8F_RESET_VALUE (uint8_t)0x0

/**
 * @brief RSVD reset
 * @details 0000 0000
 * @return 0x0
 */
#define OV7670_RSVD_90_RESET_VALUE (uint8_t)0x0

/**
 * @brief RSVD reset
 * @details 0000 0000
 * @return 0x0
 */
#define OV7670_RSVD_91_RESET_VALUE (uint8_t)0x0

/**
 * @brief RSVD reset
 * @details 0000 0000
 * @return 0x0
 */
#define OV7670_RSVD_96_RESET_VALUE_ONE (uint8_t)0x0

/**
 * @brief RSVD reset
 * @details 0000 0000
 * @return 0x0
 */
#define OV7670_RSVD_9A_RESET_VALUE_ONE (uint8_t)0x0

/**
 * @brief RSVD reset
 * @details 1000 0100
 * @return 0x84
 */
#define OV7670_RSVD_B0_RESET_VALUE (uint8_t)0x84

/**
 * @brief ABLC1 reset
 * @details 0000 1100
 * @return 0x0C
 */
#define OV7670_ABLC1_RESET_VALUE (uint8_t)0x0C

/**
 * @brief RSVD reset
 * @details 0000 1110
 * @return 0x0E
 */
#define OV7670_RSVD_B2_RESET_VALUE (uint8_t)0x0E

/**
 * @brief THL_ST reset
 * @details 1000 0010
 * @return 0x82
 */
#define OV7670_THL_ST_RESET_VALUE (uint8_t)0x82

/**
 * @brief RSVD reset
 * @details 0000 1010
 * @return 0x0A
 */
#define OV7670_RSVD_B8_RESET_VALUE (uint8_t)0x0A

/**
 * @brief AWBC1 reset
 * @details 0000 1010
 * @return 0x0A
 */
#define OV7670_AWBC1_RESET_VALUE (uint8_t)0x0A

/**
 * @brief AWBC3 reset
 * @details 0011 0100
 * @return 0x34
 */
#define OV7670_AWBC3_RESET_VALUE (uint8_t)0x34

/**
 * @brief AWBC4 reset
 * @details 0101 1000
 * @return 0x58
 */
#define OV7670_AWBC4_RESET_VALUE (uint8_t)0x58

/**
 * @brief AWBC5 reset
 * @details 0010 1000
 * @return 0x28
 */
#define OV7670_AWBC5_RESET_VALUE (uint8_t)0x28

/**
 * @brief AWBC6 reset
 * @details 0011 1010
 * @return 0x3A
 */
#define OV7670_AWBC6_RESET_VALUE (uint8_t)0x3A

/**
 * @brief RSVD reset
 * @details 1000 1000
 * @return 0x88
 */
#define OV7670_RSVD_59_RESET_VALUE (uint8_t)0x88

/**
 * @brief RSVD reset
 * @details 1000 1000
 * @return 0x88
 */
#define OV7670_RSVD_5A_RESET_VALUE (uint8_t)0x88

/**
 * @brief RSVD reset
 * @details 0100 0100
 * @return 0x44
 */
#define OV7670_RSVD_5B_RESET_VALUE (uint8_t)0x44

/**
 * @brief RSVD reset
 * @details 0110 0111
 * @return 0x67
 */
#define OV7670_RSVD_5C_RESET_VALUE (uint8_t)0x67

/**
 * @brief RSVD reset
 * @details 0100 1001
 * @return 0x49
 */
#define OV7670_RSVD_5D_RESET_VALUE (uint8_t)0x49

/**
 * @brief RSVD reset
 * @details 0000 1110
 * @return 0x0E
 */
#define OV7670_RSVD_5E_RESET_VALUE (uint8_t)0x0E

/**
 * @brief AWBCTR3 reset
 * @details 0000 1010
 * @return 0x0A
 */
#define OV7670_AWBCTR3_RESET_VALUE (uint8_t)0x0A

/**
 * @brief AWBCTR2 reset
 * @details 0101 0101
 * @return 0x55
 */
#define OV7670_AWBCTR2_RESET_VALUE (uint8_t)0x55

/**
 * @brief AWBCTR1 reset
 * @details 0001 0001
 * @return 0x11
 */
#define OV7670_AWBCTR1_RESET_VALUE (uint8_t)0x11

/**
 * @brief AWBCTR0 reset
 * @details 0110 1111
 * @return 0x6F
 */
#define OV7670_AWBCTR0_RESET_VALUE (uint8_t)0x9F

/**
 * @brief GGAIN reset
 * @details 0100 0000
 * @return 0x40
 */
#define OV7670_GGAIN_RESET_VALUE (uint8_t)0x40

/**
 * @brief BLUE Gain reset
 * @details 0100 0000
 * @return 0x40
 */
#define OV7670_BLUE_GAIN_RESET_VALUE (uint8_t)0x40

/**
 * @brief RED Gain reset
 * @details 0110 0000
 * @return 0x60
 */
#define OV7670_RED_GAIN_RESET_VALUE (uint8_t)0x60

/**
 * @brief COM8 reset three
 * @details 1110 0111
 * @return 0xE7
 */
#define OV7670_COM8_RESET_VALUE_THREE (uint8_t)0xE7

/**
 * @brief MTX1 reset
 * @details 1000 0000
 * @return 0x80
 */
#define OV7670_MTX1_RESET_VALUE (uint8_t)0x80

/**
 * @brief MTX2 reset
 * @details 1000 0000
 * @return 0x80
 */
#define OV7670_MTX2_RESET_VALUE (uint8_t)0x80

/**
 * @brief MTX3 reset
 * @details 0000 0000
 * @return 0x0
 */
#define OV7670_MTX3_RESET_VALUE (uint8_t)0x0

/**
 * @brief MTX4 reset
 * @details 0010 0010
 * @return 0x22
 */
#define OV7670_MTX4_RESET_VALUE (uint8_t)0x22

/**
 * @brief MTX5 reset
 * @details 0101 1110
 * @return 0x5E
 */
#define OV7670_MTX5_RESET_VALUE (uint8_t)0x5E

/**
 * @brief MTX6 reset
 * @details 1000 0000
 * @return 0x80
 */
#define OV7670_MTX6_RESET_VALUE (uint8_t)0x80

/**
 * @brief MTXS reset
 * @details 1001 1110
 * @return 0x9E
 */
#define OV7670_MTXS_RESET_VALUE (uint8_t)0x9E

/**
 * @brief COM16 reset
 * @details 0000 1000
 * @return 0x08
 */
#define OV7670_COM16_RESET_VALUE_ONE (uint8_t)0x08

/**
 * @brief EDGE reset
 * @details 0000 0000
 * @return 0x0
 */
#define OV7670_EDGE_RESET_VALUE (uint8_t)0x0

/**
 * @brief REG75 reset
 * @details 0000 0101
 * @return 0x05
 */
#define OV7670_REG75_RESET_VALUE (uint8_t)0x05

/**
 * @brief REG76 reset
 * @details 1110 0001
 * @return 0xE1
 */
#define OV7670_REG76_RESET_VALUE (uint8_t)0xE1

/**
 * @brief DNSTH reset
 * @details 0000 0000
 * @return 0x0
 */
#define OV7670_DNSTH_RESET_VALUE (uint8_t)0x0

/**
 * @brief REG77 reset
 * @details 0000 0001
 * @return 0x01
 */
#define OV7670_REG77_RESET_VALUE (uint8_t)0x01

/**
 * @brief COM13 reset
 * @details 1100 0011
 * @return 0xC3
 */
#define OV7670_COM13_RESET_VALUE (uint8_t)0xC3

/**
 * @brief REG4B reset
 * @details 0000 1001
 * @return 0x09
 */
#define OV7670_REG4B_RESET_VALUE (uint8_t)0x09

/**
 * @brief SATCTR reset
 * @details 0110 0000
 * @return 0x60
 */
#define OV7670_SATCTR_RESET_VALUE (uint8_t)0x60

/**
 * @brief COM16 reset
 * @details 0011 1000
 * @return 0x38
 */
#define OV7670_COM16_RESET_VALUE_TWO (uint8_t)0x38

/**
 * @brief COM11 reset
 * @details 0001 0010
 * @return 0x12
 */
#define OV7670_COM11_RESET_VALUE (uint8_t)0x12

/**
 * @brief NT_CTRL reset
 * @details 1000 1000
 * @return 0x88
 */
#define OV7670_NT_CTRL_RESET_VALUE (uint8_t)0x88

/**
 * @brief RSVD reset
 * @details 0000 0000
 * @return 0x0
 */
#define OV7670_RSVD_96_RESET_VALUE_TWO (uint8_t)0x0

/**
 * @brief RSVD reset
 * @details 0011 0000
 * @return 0x30
 */
#define OV7670_RSVD_97_RESET_VALUE (uint8_t)0x30

/**
 * @brief RSVD reset
 * @details 0020 0000
 * @return 0x20
 */
#define OV7670_RSVD_98_RESET_VALUE (uint8_t)0x20

/**
 * @brief RSVD reset
 * @details 0011 0000
 * @return 0x30
 */
#define OV7670_RSVD_99_RESET_VALUE (uint8_t)0x30

/**
 * @brief RSVD reset
 * @details 1000 0100
 * @return 0x84
 */
#define OV7670_RSVD_9A_RESET_VALUE_TWO (uint8_t)0x84

/**
 * @brief RSVD reset
 * @details 0010 1001
 * @return 0x29
 */
#define OV7670_RSVD_9B_RESET_VALUE (uint8_t)0x29

/**
 * @brief RSVD reset
 * @details 0000 0011
 * @return 0x03
 */
#define OV7670_RSVD_9C_RESET_VALUE (uint8_t)0x03

/**
 * @brief BD50ST reset
 * @details 0100 1100
 * @return 0x4C
 */
#define OV7670_BD50ST_RESET_VALUE (uint8_t)0x4C

/**
 * @brief BD60ST reset
 * @details 0011 1111
 * @return 0x3F
 */
#define OV7670_BD60ST_RESET_VALUE (uint8_t)0x3F

/**
 * @brief RSVD reset
 * @details 0000 0100
 * @return 0x04
 */
#define OV7670_RSVD_78_RESET_VALUE (uint8_t)0x04

/**
 * @brief RSVD reset
 * @details 0000 0001
 * @return 0x01
 */
#define OV7670_RSVD_79_RESET_VALUE_ONE (uint8_t)0x01

/**
 * @brief RSVD reset
 * @details 1111 0000
 * @return 0xF8
 */
#define OV7670_RSVD_C8_RESET_VALUE_ONE (uint8_t)0xF0

/**
 * @brief RSVD reset
 * @details 0000 1111
 * @return 0x0F
 */
#define OV7670_RSVD_79_RESET_VALUE_TWO (uint8_t)0x0F

/**
 * @brief RSVD reset
 * @details 0000 0000
 * @return 0x0
 */
#define OV7670_RSVD_C8_RESET_VALUE_TWO (uint8_t)0x0

/**
 * @brief RSVD reset
 * @details 0001 0000
 * @return 0x10
 */
#define OV7670_RSVD_79_RESET_VALUE_THREE (uint8_t)0x10

/**
 * @brief RSVD reset
 * @details 0111 1110
 * @return 0x7E
 */
#define OV7670_RSVD_C8_RESET_VALUE_THREE (uint8_t)0x7E

/**
 * @brief RSVD reset
 * @details 0001 1010
 * @return 0x0A
 */
#define OV7670_RSVD_79_RESET_VALUE_FOUR (uint8_t)0x0A

/**
 * @brief RSVD reset
 * @details 1000 0000
 * @return 0x80
 */
#define OV7670_RSVD_C8_RESET_VALUE_FOUR (uint8_t)0x80

/**
 * @brief RSVD reset
 * @details 0000 1011
 * @return 0x0B
 */
#define OV7670_RSVD_79_RESET_VALUE_FIVE (uint8_t)0x0B

/**
 * @brief RSVD reset
 * @details 0000 0001
 * @return 0x01
 */
#define OV7670_RSVD_C8_RESET_VALUE_FIVE (uint8_t)0x01

/**
 * @brief RSVD reset
 * @details 0000 1100
 * @return 0x0C
 */
#define OV7670_RSVD_79_RESET_VALUE_SIX (uint8_t)0x0C

/**
 * @brief RSVD reset
 * @details 0000 1111
 * @return 0x0F
 */
#define OV7670_RSVD_C8_RESET_VALUE_SIX (uint8_t)0x0F

/**
 * @brief RSVD reset
 * @details 0000 1101
 * @return 0x0D
 */
#define OV7670_RSVD_79_RESET_VALUE_SEVEN (uint8_t)0x0D

/**
 * @brief RSVD reset
 * @details 0010 0000
 * @return 0x20
 */
#define OV7670_RSVD_C8_RESET_VALUE_SEVEN (uint8_t)0x20

/**
 * @brief RSVD reset
 * @details 0000 1001
 * @return 0x09
 */
#define OV7670_RSVD_79_RESET_VALUE_EIGHT (uint8_t)0x09

/**
 * @brief RSVD reset
 * @details 1000 0000
 * @return 0x80
 */
#define OV7670_RSVD_C8_RESET_VALUE_EIGHT (uint8_t)0x80

/**
 * @brief RSVD reset
 * @details 0000 0010
 * @return 0x02
 */
#define OV7670_RSVD_79_RESET_VALUE_NINE (uint8_t)0x02

/**
 * @brief RSVD reset
 * @details 1100 0000
 * @return 0xC0
 */
#define OV7670_RSVD_C8_RESET_VALUE_NINE (uint8_t)0xC0

/**
 * @brief RSVD reset
 * @details 0000 0011
 * @return 0x03
 */
#define OV7670_RSVD_79_RESET_VALUE_TEN (uint8_t)0x03

/**
 * @brief RSVD reset
 * @details 0100 0000
 * @return 0x40
 */
#define OV7670_RSVD_C8_RESET_VALUE_TEN (uint8_t)0x40

/**
 * @brief RSVD reset
 * @details 0000 0101
 * @return 0x05
 */
#define OV7670_RSVD_79_RESET_VALUE_ELEVEN 0x05

/**
 * @brief RSVD reset
 * @details 0011 0000
 * @return 0x30
 */
#define OV7670_RSVD_C8_RESET_VALUE_ELEVEN 0x30

/**
 * @brief RSVD reset
 * @details 0010 0110
 * @return 0x26
 */
#define OV7670_RSVD_79_RESET_VALUE_FINAL 0x26

#endif /* OV7670_H */
