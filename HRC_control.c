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


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "HRC_driver.h" 
//#include "websocket_protocol.h"

HRC_DATA my_data;
uint32_t counter = 0;
extern volatile uint8_t data_ready;

uint8_t RevId = 0;
uint8_t PartId = 0;
uint16_t tempValue = 0;
uint16_t irBuff[16] = {0};
uint16_t redBuff[16] = {0};
uint8_t register_dump[20];

int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y);

void HRC_Register_Dump(int file) {
	uint8_t data;

#define WAIT_US  500

	printf("======== Register Dump: ======== \n");

	usleep(WAIT_US);
	data = HRC_ReadFromSensor(file, HRC_INT_STATUS);
	printf("HRC_INT_STATUS: \t%02x\n", data);

	usleep(WAIT_US);
	data = HRC_ReadFromSensor(file, HRC_INT_ENABLE);
	printf("HRC_INT_ENABLE: \t%02x\n", data);

	usleep(WAIT_US);
	data = HRC_ReadFromSensor(file, HRC_MODE_CONFIG);
	printf("HRC_MODE_CONFIG: \t%02x\n", data);

	usleep(WAIT_US);
	data = HRC_ReadFromSensor(file, HRC_SPO2_CONFIG);
	printf("HRC_SPO2_CONFIG: \t%02x\n", data);

	usleep(WAIT_US);
	data = HRC_ReadFromSensor(file, HRC_INT_ENABLE);
	printf("HRC_INT_ENABLE: \t%02x\n", data);

	usleep(WAIT_US);
	data = HRC_ReadFromSensor(file, HRC_LED_CONFIG);
	printf("HRC_LED_CONFIG: \t%02x\n", data);

	usleep(WAIT_US);
	data = HRC_ReadFromSensor(file, HRC_TEMP_INTEGER);
	printf("HRC_TEMP_INTEGER: \t%02x\n", data);

	usleep(WAIT_US);
	data = HRC_ReadFromSensor(file, HRC_TEMP_FRACTION);
	printf("HRC_TEMP_FRACTIO: \t%02x\n", data);

	usleep(WAIT_US);
	data = HRC_ReadFromSensor(file, HRC_REVISION_ID);
	printf("HRC_REVISION_ID: \t%02x\n", data);

	usleep(WAIT_US);
	data = HRC_ReadFromSensor(file, HRC_PART_ID);
	printf("HRC_PART_ID: \t%02x\n", data);

	printf("================================ \n");

}

void HRC_Startup(int file) {
	TEMPERATURE_VALUE temperature;
	MODE_CONFIG_BITS configuration;

	RevId = HRC_GetRevisionID(file);
	PartId = HRC_GetPartID(file);
	printf("HRC Part ID = %02x Revision ID = %02x\n\r", PartId, RevId);

	printf("Reset...\n");
	HRC_Reset(file);
	printf("Initialize...\n");
	HRC_Initialize(file);
	//HRC_Register_Dump(file);

	while (HRC_GetStatus(file).A_FULL == 0);

	configuration.byte = HRC_ReadFromSensor(file, HRC_MODE_CONFIG);
	configuration.TEMP_EN = 1;
	HRC_SendToSensor(file, HRC_MODE_CONFIG, configuration.byte);
	while (HRC_GetStatus(file).TEMP_RDY == 0);
	temperature.value = HRC_ReadTemperature(file);
	printf("Temperature: %d.%d\n\r", temperature.byte[0], (int) ((0.0625 * (float) temperature.byte[1])*100.0));
	sleep(3);
	HRC_SendToSensor(file, HRC_FIFO_WRITE_PTR, 0);
	HRC_SendToSensor(file, HRC_OVER_FLOW_CNT, 0);
	HRC_SendToSensor(file, HRC_FIFO_READ_PTR, 0);
}

double CollectTempData(int file)
{
	//HRC_Initialize(file);
	uint8_t configuration = 0;
	double ret;
	configuration = HRC_ReadFromSensor(file, HRC_MODE_CONFIG);
	configuration = (configuration & ~0x07) | HRC_HR_ONLY;
	configuration = (configuration & ~0x07) | HRC_TEMP_EN;
	configuration = (configuration & ~0x07) | HRC_SPO2_EN;
	//printf("Send to Sensor Reg:%02x Data:%02x\n", HRC_MODE_CONFIG, configuration);
	HRC_SendToSensor(file, HRC_MODE_CONFIG, configuration);

	configuration = HRC_ReadFromSensor(file, HRC_SPO2_CONFIG);
	configuration |= HRC_SPO2_HI_RES_EN;
	configuration |= HRC_SAMPLES_400;
	configuration |= HRC_PULSE_WIDTH_800;
	//printf("Send to Sensor Reg:%02x Data:%02x\n", HRC_SPO2_CONFIG, configuration);
	HRC_SendToSensor(file, HRC_SPO2_CONFIG, configuration);

	configuration = HRC_ReadFromSensor(file, HRC_LED_CONFIG);
	configuration |= HRC_IR_CURRENT_110;
	configuration |= HRC_RED_CURRENT_110;
	//  printf("Send to Sensor Reg:%02x Data:%02x\n", HRC_LED_CONFIG, configuration);
	HRC_SendToSensor(file, HRC_LED_CONFIG, configuration);

	configuration = HRC_ReadFromSensor(file, HRC_INT_ENABLE);
	configuration |= HRC_ENA_A_FULL;
	configuration |= HRC_ENA_HR_RDY;
	configuration |= HRC_ENA_SO2_RDY;
	configuration |= HRC_ENA_TEP_RDY;
	//    printf("Send to Sensor Reg:%02x Data:%02x\n", HRC_INT_ENABLE, configuration);
	HRC_SendToSensor(file, HRC_INT_ENABLE, configuration);

	//i2c_file = file;	
	TEMPERATURE_VALUE temperature;
	temperature.value = HRC_ReadTemperature(file);
	printf("Temperature: %d.%d\n\r", temperature.byte[0], (int) ((0.0625 * (float) temperature.byte[1])*100.0));
	//ret = temperature.byte[0] + ((int) ((0.0625 * (float) temperature.byte[1])*100.0));
	return ret;
}

void HRC_Run(int file) {
	int8_t ix;
	struct timeval starttime, endtime, timediff;
	char string[256];
	char sub_string[100];
	uint8_t string_length;

	gettimeofday(&starttime, 0x0);
	while (HRC_GetStatus(file).A_FULL == 0) {
		usleep(1000);
	}
	gettimeofday(&endtime, 0x0);
	timeval_subtract(&timediff, &endtime, &starttime);

	HRC_ReadBlockFromSensor(file, HRC_FIFO_DATA_REG, (uint8_t*) & my_data.longs[ 0], 16);
	HRC_ReadBlockFromSensor(file, HRC_FIFO_DATA_REG, (uint8_t*) & my_data.longs[ 4], 16);
	HRC_ReadBlockFromSensor(file, HRC_FIFO_DATA_REG, (uint8_t*) & my_data.longs[ 8], 16);
	HRC_ReadBlockFromSensor(file, HRC_FIFO_DATA_REG, (uint8_t*) & my_data.longs[12], 16);

	string[0]=0;
	for (ix = 0; ix < 16; ix++) {
		sub_string[0] = 0;
		sprintf(sub_string, "%d ", my_data.sample[ix].red);
		strcat(string,sub_string);
	}
	string_length = strlen(string);

	// To delete the last " "(0x20) 
	string_length--;
	string[string_length]=0; 

	//SendDataToWebsocketClient(string, string_length);

	data_ready = 1;
}

int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y) {
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	   tv_usec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;//FILE *fd;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

