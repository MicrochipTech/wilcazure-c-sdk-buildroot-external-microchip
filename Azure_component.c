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

// Standard C header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <math.h>

//  routines
#include "Azure_component.h"
#include "iothub_client_properties.h"

#include"HRC_control.c"
#include"HRC_driver.c"

// Core IoT SDK utilities
#include "azure_c_shared_utility/xlogging.h"

// Size of buffer to store ISO 8601 time.
#define TIME_BUFFER_SIZE 128

// Size of buffer to store current temperature telemetry.
#define CURRENT_TEMPERATURE_BUFFER_SIZE  32

// Size of buffer to store the maximum temp since reboot property.
#define MAX_TEMPERATURE_SINCE_REBOOT_BUFFER_SIZE 32

// Maximum component size.  This comes from the spec https://github.com/Azure/opendigitaltwins-dtdl/blob/master/DTDL/v2/dtdlv2.md#component.
#define MAX_COMPONENT_NAME_LENGTH 64


// Names of properties for desired/reporting.

// Name of command this component supports to retrieve a report about the component.
//static const char g_getMaxMinReportCommandName[] = "getMaxMinReport";

// The default temperature to use before any is set
#define DEFAULT_TEMPERATURE_VALUE 22

// Format string to create an ISO 8601 time.  This corresponds to the DTDL datetime schema item.
static const char g_ISO8601Format[] = "%Y-%m-%dT%H:%M:%SZ";

// Format string for sending temperature telemetry.
static const char g_temperatureTelemetryBodyFormat[] = "{\"temperature\":%.02f}";

// Format string for sending maxTempSinceLastReboot property.
//static const char g_maxTempSinceLastRebootPropertyFormat[] = "%.2f";

// Metadata to add to telemetry messages.
static const char g_jsonContentType[] = "application/json";
static const char g_utf8EncodingType[] = "utf8";

// Start time of the program, stored in ISO 8601 format string for UTC
char g_programStartTime[TIME_BUFFER_SIZE] = {0};

//
// THERMOSTAT_COMPONENT simulates a thermostat component
//
//typedef struct THERMOSTAT_COMPONENT_TAG
typedef struct THERMOSTAT_COMPONENT_TAG
{
	// Name of this component
	char componentName[MAX_COMPONENT_NAME_LENGTH + 1];
	// Current temperature of this thermostat component
	double currentTemperature;
	// Number of times temperature has been updated, counting the initial setting as 1.  Used to determine average temperature of this thermostat component
	int numTemperatureUpdates;
	// Total of all temperature updates during current execution run.  Used to determine average temperature of this thermostat component
	double allTemperatures;
}
HR_THERMOSTAT_COMPONENT;

//
// BuildUtcTimeFromCurrentTime writes the current time, in ISO 8601 format, into the specified buffer.
//
static bool BuildUtcTimeFromCurrentTime(char* utcTimeBuffer, size_t utcTimeBufferSize)
{
	bool result;
	time_t currentTime;
	struct tm * currentTimeTm;

	time(&currentTime);
	currentTimeTm = gmtime(&currentTime);

	if (strftime(utcTimeBuffer, utcTimeBufferSize, g_ISO8601Format, currentTimeTm) == 0)
	{
		printf("snprintf on UTC time failed");
		result = false;
	}
	else
	{
		result = true;
	}

	return result;
}

HR_COMPONENT_HANDLE CreateHandle(const char* componentName)
{
	HR_THERMOSTAT_COMPONENT* thermostatComponent;

	if (strlen(componentName) > MAX_COMPONENT_NAME_LENGTH)
	{
		printf("componentName %s is too long.  Maximum length is %d", componentName, MAX_COMPONENT_NAME_LENGTH);
		thermostatComponent = NULL;
	}
	// On initial invocation, store the UTC time into g_programStartTime global.
	else if ((g_programStartTime[0] == 0) && (BuildUtcTimeFromCurrentTime(g_programStartTime, sizeof(g_programStartTime)) == false))
	{
		printf("Unable to store program start time");
		thermostatComponent = NULL;
	}
	else if ((thermostatComponent = (HR_THERMOSTAT_COMPONENT*)calloc(1, sizeof(HR_THERMOSTAT_COMPONENT))) == NULL)
	{
		printf("Unable to allocate thermostat");
	}
	else
	{
		strcpy(thermostatComponent->componentName, componentName);
		thermostatComponent->currentTemperature = DEFAULT_TEMPERATURE_VALUE;
		thermostatComponent->numTemperatureUpdates = 1;
		thermostatComponent->allTemperatures = DEFAULT_TEMPERATURE_VALUE;
	}

	return (HR_COMPONENT_HANDLE)thermostatComponent;
}

void ThermostatComponent_Destroy(HR_COMPONENT_HANDLE AzureComponentHandle)
{
	if (AzureComponentHandle != NULL)
	{
		free(AzureComponentHandle);
	}
}


void ThermostatComponent_SendCurrentTemperature(HR_COMPONENT_HANDLE AzureComponentHandle, IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient,int file)
{
	HR_THERMOSTAT_COMPONENT* hrThermostatComponent = (HR_THERMOSTAT_COMPONENT*)AzureComponentHandle;
	IOTHUB_MESSAGE_HANDLE messageHandle = NULL;
	IOTHUB_MESSAGE_RESULT messageResult;
	IOTHUB_CLIENT_RESULT iothubClientResult;

	char temperatureStringBuffer[CURRENT_TEMPERATURE_BUFFER_SIZE];
	hrThermostatComponent->currentTemperature = CollectTempData(file);
	//printf("temperature = %lf\n",hrThermostatComponent->currentTemperature);
	// Create the telemetry message body to send.
	if (snprintf(temperatureStringBuffer, sizeof(temperatureStringBuffer), g_temperatureTelemetryBodyFormat, hrThermostatComponent->currentTemperature) < 0)
	{
		printf("snprintf of current temperature telemetry failed");
	}
	// Create the message handle and specify its metadata.
	else if ((messageHandle = IoTHubMessage_CreateFromString(temperatureStringBuffer)) == NULL)
	{
		printf("IoTHubMessage_CreateFromString failed");
	}
	else if ((messageResult = IoTHubMessage_SetContentTypeSystemProperty(messageHandle, g_jsonContentType)) != IOTHUB_MESSAGE_OK)
	{
		printf("IoTHubMessage_SetContentTypeSystemProperty failed, error=%d", messageResult);
	}
	else if ((messageResult = IoTHubMessage_SetContentEncodingSystemProperty(messageHandle, g_utf8EncodingType)) != IOTHUB_MESSAGE_OK)
	{
		printf("IoTHubMessage_SetContentEncodingSystemProperty failed, error=%d", messageResult);
	}
	// Send the telemetry message.

	IoTHubMessage_Destroy(messageHandle);
}
