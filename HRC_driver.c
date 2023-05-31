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

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>


#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "HRC_defines.h"

int i2c_file;

__s32 I2C_smbus_access(int file, char read_write, __u8 slave_register,
		int size, union i2c_smbus_data *data, uint8_t N) {
	struct i2c_smbus_ioctl_data args;

	if (size == I2C_SMBUS_I2C_BLOCK_DATA) data->block[0] = N;
	args.read_write = read_write;
	args.command = slave_register;
	args.size = size;
	args.data = data;

	return ioctl(file, I2C_SMBUS, &args);
}

__s32 I2C_smbus_read_byte_data(int file, __u8 slave_register) {
	union i2c_smbus_data data;
	if (I2C_smbus_access(file, I2C_SMBUS_READ, slave_register,
				I2C_SMBUS_BYTE_DATA, &data, 0))
		return -1;
	else
		return 0x0FF & data.byte;
}

__s32 I2C_smbus_read_block_data(int file, __u8 slave_register, uint8_t N, uint8_t *block) {
	union i2c_smbus_data data;

	if (I2C_smbus_access(file, I2C_SMBUS_READ, slave_register,
				I2C_SMBUS_I2C_BLOCK_DATA, &data, N)) {
		return -1;
	} else {
		memcpy(block, data.block, N);
		return 0;
	}
}

bool I2C_smbus_write_byte_data(int file, __u8 slave_register, uint8_t write_data_byte) {
	union i2c_smbus_data data;
	data.byte = write_data_byte;
	if (I2C_smbus_access(file, I2C_SMBUS_WRITE, slave_register,
				I2C_SMBUS_BYTE_DATA, &data, 0))
		return false;
	else
		return true;
}

void HRC_SendToSensor(int file, uint8_t slave_reg, uint8_t data) {
	I2C_smbus_write_byte_data(file, slave_reg, data);
}

uint8_t HRC_ReadFromSensor(int file, uint8_t slave_register) {
	return I2C_smbus_read_byte_data(file, slave_register);
}

uint32_t HRC_ReadBlockFromSensor(int file, uint8_t slave_register, uint8_t *block, uint8_t N) {
	return I2C_smbus_read_block_data(file, slave_register, N, block);
}

uint8_t HRC_Get(int file, uint8_t anID) {
	return HRC_ReadFromSensor(file, anID);
}

uint8_t HRC_GetRevisionID(int file) {
	return HRC_ReadFromSensor(file, HRC_REVISION_ID);
}

uint8_t HRC_GetPartID(int file) {
	return HRC_ReadFromSensor(file, HRC_PART_ID);
}

INT_STATUS_BITS HRC_GetStatus(int file) {
	INT_STATUS_BITS status;
	status.byte = HRC_ReadFromSensor(file, HRC_INT_STATUS);
	return status;
}

uint16_t HRC_ReadTemperature(int file) {
	TEMPERATURE_VALUE temp;
	temp.byte[0] = HRC_ReadFromSensor(file, HRC_TEMP_INTEGER);
	temp.byte[1] = HRC_ReadFromSensor(file, HRC_TEMP_FRACTION);
	return temp.value;
}

void HRC_Reset(int file) {

	union {
		uint8_t byte;

		struct {
			int B0 : 1;
			int B1 : 1;
			int B2 : 1;
			int B3 : 1;
			int B4 : 1;
			int B5 : 1;
			int B6 : 1;
			int B7 : 1;
		};
	} configuration;

	configuration.byte = HRC_ReadFromSensor(file, HRC_MODE_CONFIG);
	configuration.B6 = 1;
	HRC_SendToSensor(file, HRC_MODE_CONFIG, configuration.byte);

	while (1) {
		configuration.byte = HRC_ReadFromSensor(file, HRC_MODE_CONFIG);
		if (configuration.B6 == 0) {
			break;
		}
	}

	usleep(50000);

}

uint8_t HRC_Initialize(int file) {
	uint8_t configuration = 0;

	configuration = HRC_ReadFromSensor(file, HRC_MODE_CONFIG);
	configuration = (configuration & ~0x07) | HRC_HR_ONLY;
	configuration = (configuration & ~0x07) | HRC_TEMP_EN;
	configuration = (configuration & ~0x07) | HRC_SPO2_EN;
	printf("Send to Sensor Reg:%02x Data:%02x\n", HRC_MODE_CONFIG, configuration);
	HRC_SendToSensor(file, HRC_MODE_CONFIG, configuration);

	configuration = HRC_ReadFromSensor(file, HRC_SPO2_CONFIG);
	configuration |= HRC_SPO2_HI_RES_EN;
	configuration |= HRC_SAMPLES_400;
	configuration |= HRC_PULSE_WIDTH_800;
	printf("Send to Sensor Reg:%02x Data:%02x\n", HRC_SPO2_CONFIG, configuration);
	HRC_SendToSensor(file, HRC_SPO2_CONFIG, configuration);

	configuration = HRC_ReadFromSensor(file, HRC_LED_CONFIG);
	configuration |= HRC_IR_CURRENT_110;
	configuration |= HRC_RED_CURRENT_110;
	printf("Send to Sensor Reg:%02x Data:%02x\n", HRC_LED_CONFIG, configuration);
	HRC_SendToSensor(file, HRC_LED_CONFIG, configuration);

	configuration = HRC_ReadFromSensor(file, HRC_INT_ENABLE);
	configuration |= HRC_ENA_A_FULL;
	configuration |= HRC_ENA_HR_RDY;
	configuration |= HRC_ENA_SO2_RDY;
	configuration |= HRC_ENA_TEP_RDY;
	printf("Send to Sensor Reg:%02x Data:%02x\n", HRC_INT_ENABLE, configuration);
	HRC_SendToSensor(file, HRC_INT_ENABLE, configuration);

	i2c_file = file;

	return configuration;
}

void HRC_SetSamples(uint8_t value) {
	SPO2_CONFIGURATION_BITS configuration;

	configuration.byte = HRC_ReadFromSensor(i2c_file, HRC_SPO2_CONFIG);
	configuration.SPO2_SR = value;
	printf("Send to Sensor Reg:%02x Data:%02x\n", HRC_SPO2_CONFIG, configuration.byte);
	HRC_SendToSensor(i2c_file, HRC_SPO2_CONFIG, configuration.byte);

}

void HRC_SetPulseWidth(uint8_t value) {
	SPO2_CONFIGURATION_BITS config;

	config.byte = HRC_ReadFromSensor(i2c_file, HRC_SPO2_CONFIG);
	config.SPO2_SR = value;
	printf("Send to Sensor Reg:%02x Data:%02x\n", HRC_SPO2_CONFIG, config.byte);
	HRC_SendToSensor(i2c_file, HRC_SPO2_CONFIG, config.byte);

}

void HRC_SetRedLEDCurrent(uint8_t value) {
	LED_CONFIGURATION_BITS configuration;

	configuration.byte = HRC_ReadFromSensor(i2c_file, HRC_LED_CONFIG);
	configuration.RED_PA = value;
	printf("Send to Sensor Reg:%02x Data:%02x\n", HRC_LED_CONFIG, configuration.byte);
	HRC_SendToSensor(i2c_file, HRC_LED_CONFIG, configuration.byte);

}

void HRC_SetIRLEDCurrent(uint8_t value) {
	LED_CONFIGURATION_BITS configuration;

	configuration.byte = HRC_ReadFromSensor(i2c_file, HRC_LED_CONFIG);
	configuration.IR_PA = value;
	printf("Send to Sensor Reg:%02x Data:%02x\n", HRC_LED_CONFIG, configuration.byte);
	HRC_SendToSensor(i2c_file, HRC_LED_CONFIG, configuration.byte);

}
