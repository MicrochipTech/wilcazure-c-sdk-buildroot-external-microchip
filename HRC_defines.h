/*
 * Copyright 2023 Microchip
 *                Nikhil Patil <nikhil.patil@microchip.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */



//double CollectTempData(int file);
#ifndef __HRC_DEFS__
#define __HRC_DEFS__

//#define HRC_DATA_READY WF_P3_PIN_GET()

#define HRC_I2C_ADR            0x57
//#define HRC_DATA_IS_READY()     ( HRC_DATA_READY == 0 )
//#define HRC_DATA_IS_NOT_READY() ( HRC_DATA_READY != 0 )

// registers' addresses
#define HRC_INT_STATUS       0x00
#define HRC_INT_ENABLE       0x01
#define HRC_FIFO_WRITE_PTR   0x02
#define HRC_OVER_FLOW_CNT    0x03
#define HRC_FIFO_READ_PTR    0x04
#define HRC_FIFO_DATA_REG    0x05
#define HRC_MODE_CONFIG      0x06
#define HRC_SPO2_CONFIG      0x07
#define HRC_LED_CONFIG       0x09
#define HRC_TEMP_INTEGER     0x16
#define HRC_TEMP_FRACTION    0x17
#define HRC_REVISION_ID      0xFE
#define HRC_PART_ID          0xFF

// mode configuration bits
#define HRC_TEMP_EN          0x08
#define HRC_HR_ONLY          0x02
#define HRC_SPO2_EN          0x03

// SpO2 configuration bits
#define HRC_SPO2_HI_RES_EN   0x40

// interrupt enable bits
#define HRC_ENA_A_FULL       0x80
#define HRC_ENA_TEP_RDY      0x40
#define HRC_ENA_HR_RDY       0x20
#define HRC_ENA_SO2_RDY      0x10

// interrupt status bits
#define HRC_PWR_RDY          0x01

// sample rate control bits [samples per second]
#define HRC_SAMPLES_MASK     0x1C // mask
#define HRC_SAMPLES_50       0x00
#define HRC_SAMPLES_100      0x04
#define HRC_SAMPLES_167      0x08
#define HRC_SAMPLES_200      0x0C
#define HRC_SAMPLES_400      0x10
#define HRC_SAMPLES_600      0x14
#define HRC_SAMPLES_800      0x18
#define HRC_SAMPLES_1000     0x1C

// LED pulse width control bits - pulse width [us]
#define HRC_PULSE_WIDTH_MASK 0x03 // mask
#define HRC_PULSE_WIDTH_200  0x00 // 13-bit ADC resolution
#define HRC_PULSE_WIDTH_400  0x01 // 14-bit ADC resolution
#define HRC_PULSE_WIDTH_800  0x02 // 15-bit ADC resolution
#define HRC_PULSE_WIDTH_1600 0x03 // 16-bit ADC resolution

// LED current control bits [ma]
#define HRC_IR_CURRENT_MASK  0x0F // mask
#define HRC_IR_CURRENT_0     0x00 // 0.0 mA
#define HRC_IR_CURRENT_44    0x01 // 4.4 mA
#define HRC_IR_CURRENT_76    0x02 // 7.6 mA
#define HRC_IR_CURRENT_110   0x03 // 11.0 mA
#define HRC_IR_CURRENT_142   0x04 // 14.2 mA
#define HRC_IR_CURRENT_174   0x05 // 17.4 mA
#define HRC_IR_CURRENT_208   0x06 // 20.8 mA
#define HRC_IR_CURRENT_240   0x07 // 24.0 mA
#define HRC_IR_CURRENT_271   0x08 // 27.1 mA
#define HRC_IR_CURRENT_306   0x09 // 30.6 mA
#define HRC_IR_CURRENT_338   0x0A // 33.8 mA
#define HRC_IR_CURRENT_370   0x0B // 37.0 mA
#define HRC_IR_CURRENT_402   0x0C // 40.2 mA
#define HRC_IR_CURRENT_436   0x0D // 43.6 mA
#define HRC_IR_CURRENT_468   0x0E // 46.8 mA
#define HRC_IR_CURRENT_500   0x0F // 50.0 mA

#define HRC_RED_CURRENT_MASK 0xF0 // mask
#define HRC_RED_CURRENT_0    0x00 // 0.0 mA
#define HRC_RED_CURRENT_44   0x10 // 4.4 mA
#define HRC_RED_CURRENT_76   0x20 // 7.6 mA
#define HRC_RED_CURRENT_110  0x30 // 11.0 mA
#define HRC_RED_CURRENT_142  0x40 // 14.2 mA
#define HRC_RED_CURRENT_174  0x50 // 17.4 mA
#define HRC_RED_CURRENT_208  0x60 // 20.8 mA
#define HRC_RED_CURRENT_240  0x70 // 24.0 mA
#define HRC_RED_CURRENT_271  0x80 // 27.1 mA
#define HRC_RED_CURRENT_306  0x90 // 30.6 mA
#define HRC_RED_CURRENT_338  0xA0 // 33.8 mA
#define HRC_RED_CURRENT_370  0xB0 // 37.0 mA
#define HRC_RED_CURRENT_402  0xC0 // 40.2 mA
#define HRC_RED_CURRENT_436  0xD0 // 43.6 mA
#define HRC_RED_CURRENT_468  0xE0 // 46.8 mA
#define HRC_RED_CURRENT_500  0xF0 // 50.0 mA


#define HRC_FIFO_DEPTH       0x10  // one sample are 4 bytes

typedef union {
	int8_t byte[2];
	uint16_t value;
} TEMPERATURE_VALUE;

typedef union {
	uint8_t byte;

	struct {
		unsigned int PWR_RDY : 1;
		unsigned int RESERVED : 3;
		unsigned int SPO2_RDY : 1;
		unsigned int HR_RDY : 1;
		unsigned int TEMP_RDY : 1;
		unsigned int A_FULL : 1;
	};
} INT_STATUS_BITS;

typedef union {
	uint8_t byte;

	struct {
		unsigned int MODE : 3;
		unsigned int TEMP_EN : 1;
		unsigned int RESERVED : 2;
		unsigned int RESET : 1;
		unsigned int SHDN : 1;
	};
} MODE_CONFIG_BITS;

typedef union {
	uint8_t byte;

	struct {
		unsigned int LED_PW : 2;
		unsigned int SPO2_SR : 2;
		unsigned int RESERVED : 1;
		unsigned int SPO2_HI_RES_EN : 1;
		unsigned int NOT_USED : 1;
	};
} SPO2_CONFIGURATION_BITS;

typedef union {
	uint8_t byte;

	struct {
		unsigned int IR_PA : 4;
		unsigned int RED_PA : 4;
	};
} LED_CONFIGURATION_BITS;

#endif
