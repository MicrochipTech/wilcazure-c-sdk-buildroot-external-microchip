/* 
 ** HRC communication driver
 */

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


#ifndef __HRC_DRIVER__
#define __HRC_DRIVER__

#include <stdint.h>
#include "HRC_defines.h"

typedef struct {
	uint16_t ir;
	uint16_t red;
} SAMPLE;

typedef union {
	uint32_t longs[HRC_FIFO_DEPTH];
	SAMPLE sample[HRC_FIFO_DEPTH * 4];
	uint16_t words[HRC_FIFO_DEPTH * 2];
	uint8_t bytes[HRC_FIFO_DEPTH * 4];
} HRC_DATA;

uint8_t HRC_Get(int file, uint8_t anID);
uint8_t HRC_GetRevisionID(int file);
uint8_t HRC_GetPartID(int file);
uint8_t HRC_GetConfig(int file);
INT_STATUS_BITS HRC_GetStatus(int file);


void HRC_SetConfiguration(int file, uint8_t cfg);
void HRC_SetInterrupt(int file, uint8_t intrpts);
void HRC_Reset(int file);
uint8_t HRC_Initialize(int file);
uint8_t HRC_ReadFromSensor(int file, uint8_t slave_register);
uint32_t HRC_ReadBlockFromSensor(int file, uint8_t slave_register, uint8_t *block, uint8_t N);
void HRC_SendToSensor(int file, uint8_t slave_reg, uint8_t data);
void HRC_SetSamples(uint8_t value);
void HRC_SetPulseWidth(uint8_t value);
void HRC_SetRedLEDCurrent(uint8_t value);
void HRC_SetIRLEDCurrent(uint8_t value);

void HRC_Run(int file);
void HRC_Startup(int file);

// irBuff  - data from IR LED
// redBuff - data from red LED
uint8_t HRC_Read(int file, uint16_t* irBuff, uint16_t* redBuff);

// tempValue - data from temperature sensor
uint16_t HRC_ReadTemperature(int file);

#endif

