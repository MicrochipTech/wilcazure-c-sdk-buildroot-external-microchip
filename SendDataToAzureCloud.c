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



// Standard C header files.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <err.h>
#include <errno.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <string.h>

#include <sys/select.h>
#include <termios.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
// JSON parser library.
#include "parson.h"

// IoT Hub device client and IoT core utility related header files.
#include "iothub.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "iothubtransportmqtt.h"
#include "iothub_message.h"
#include "iothub_client_properties.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/xlogging.h"
#include "iothubtransportmqtt.h"

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
// For devices that do not have (or want) an OS level trusted certificate store,
// but instead bring in default trusted certificates from the Azure IoT C SDK.
#include "azure_c_shared_utility/shared_util_options.h"
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

#include "config.h" 

#ifdef USE_PROV_MODULE_FULL
//#include "dps_ll.h" 
#endif // USE_PROV_MODULE_FULL

// Headers that provide implementation for subcomponents (the two thermostat components and DeviceInfo)
#include "Azure_component.h"
//#include "iothub_deviceinfo_component.h"
#include "HRC_driver.h"

#define CURRENT_WORKING_SET_BUFFER_SIZE 64

// Every time the main loop wakes up, on the g_sendTelemetryPollInterval(th) pass will send a telemetry message.
// So we will send telemetry every (g_sendTelemetryPollInterval * g_sleepBetweenPollsMs) milliseconds; 30 seconds as currently configured.
static const int g_sendTelemetryPollInterval = 400;

// Whether tracing at the IoT Hub client is enabled or not.
static bool g_hubClientTraceEnabled = true;

// DTMI indicating this device's model identifier.
static const char g_temperatureControllerModelId[] = "dtmi:com:example:TemperatureController;1";

// Metadata to add to telemetry messages.
static const char g_jsonContentType[] = "application/json";
static const char g_utf8EncodingType[] = "utf8";

// HR_COMPONENT_HANDLE represent the thermostat components that are sub-components of the temperature controller.
// Note that we do NOT have an analogous DeviceInfo component handle because there is only DeviceInfo subcomponent and its
// implementation is straightforward.
HR_COMPONENT_HANDLE Handle1;

// Name of subcomponents that TemmperatureController implements.
static const char g_thermostatComponent1Name[] = "thermostat1";

// Minimum value we will return for working set, + some random number.
static const int g_workingSetMinimum = 1000;
// Random number for working set will range between the g_workingSetMinimum and (g_workingSetMinimum+g_workingSetRandomModulo).
static const int g_workingSetRandomModulo = 500;
// Format string for sending a telemetry message with the working set.
static const char g_workingSetTelemetryFormat[] = "{\"workingSet\":%d}";

// Values of connection / security settings read from environment variables and/or DPS runtime.
DEVICE_CONFIGURATION g_DeviceConfiguration;

//
// TempControlComponent_UpdatedPropertyCallback is invoked when properties arrive from the server.
//
static void TempControlComponent_UpdatedPropertyCallback(
		//IOTHUB_CLIENT_PROPERTY_PAYLOAD_TYPE payloadType, 
		IOTHUB_CLIENT_PROPERTY_VALUE_TYPE payloadType, 
		const unsigned char* payload,
		size_t payloadLength,
		void* userContextCallback)
{
	IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE propertiesReader = NULL;
	IOTHUB_CLIENT_PROPERTY_PARSED property;
	int propertiesVersion;
	IOTHUB_CLIENT_RESULT clientResult;

	// The properties arrive as a raw JSON buffer (which is not null-terminated).  IoTHubClient_Properties_Deserializer_Create parses 
	// this into a more convenient form to allow property-by-property enumeration over the updated properties.
	if ((clientResult = IoTHubClient_Properties_Deserializer_Create(payloadType, payload, payloadLength, &propertiesReader)) != IOTHUB_CLIENT_OK)
	{
		printf("IoTHubClient_Deserialize_Properties failed, error=%d", clientResult);
	}
	else if ((clientResult = IoTHubClient_Properties_Deserializer_GetVersion(propertiesReader, &propertiesVersion)) != IOTHUB_CLIENT_OK)
	{
		printf("IoTHubClient_Properties_Deserializer_GetVersion failed, error=%d", clientResult);
	}
	else
	{
		bool propertySpecified;
		property.structVersion = IOTHUB_CLIENT_PROPERTY_PARSED_STRUCT_VERSION_1;

		while ((clientResult = IoTHubClient_Properties_Deserializer_GetNext(propertiesReader, &property, &propertySpecified)) == IOTHUB_CLIENT_OK)
		{
			if (propertySpecified == false)
			{
				break;
			}

			if (property.propertyType == IOTHUB_CLIENT_PROPERTY_TYPE_REPORTED_FROM_CLIENT)
			{
				// We are iterating over a property that the device has previously sent to IoT Hub; 
				// this shows what IoT Hub has recorded the reported property as.
				//
				// There are scenarios where a device may use this, such as knowing whether the
				// given property has changed on the device and needs to be re-reported.
				//
				// This sample doesn't do anything with this, so we'll continue on when we hit reported properties.
				continue;
			}

			// Process IOTHUB_CLIENT_PROPERTY_TYPE_WRITABLE propertyType, which means IoT Hub is configuring a property
			// on this device.
			//
			// If we receive a component or property the model does not support, log the condition locally but do not report this
			// back to IoT Hub.
			if (property.componentName == NULL) 
			{   
				printf("Property arrived for TemperatureControl component itself.  This does not support properties on it (all properties are on subcomponents)");
			}
			else
			{
				printf("Component %s is not implemented by the TemperatureController", property.componentName);
			}

			IoTHubClient_Properties_DeserializerProperty_Destroy(&property);
		}
	}

	IoTHubClient_Properties_Deserializer_Destroy(propertiesReader);
}

//
// TempControlComponent_SendWorkingSet sends a telemetry indicating the current working set of the device, in 
// the unit of kibibytes (https://en.wikipedia.org/wiki/Kibibyte).
//
void TempControlComponent_SendWorkingSet(IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient) 
{
	IOTHUB_MESSAGE_HANDLE messageHandle = NULL;
	IOTHUB_MESSAGE_RESULT messageResult;
	IOTHUB_CLIENT_RESULT iothubClientResult;
	char workingSetTelemetryPayload[CURRENT_WORKING_SET_BUFFER_SIZE];

	int workingSet = g_workingSetMinimum + (rand() % g_workingSetRandomModulo);

	// Create the telemetry message body to send.
	if (snprintf(workingSetTelemetryPayload, sizeof(workingSetTelemetryPayload), g_workingSetTelemetryFormat, workingSet) < 0)
	{
		printf("Unable to create a workingSet telemetry payload string");
	}
	// Create the message handle and specify its metadata.
	else if ((messageHandle = IoTHubMessage_CreateFromString(workingSetTelemetryPayload)) == NULL)
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
	else if ((iothubClientResult = IoTHubDeviceClient_LL_SendEventAsync(deviceClient, messageHandle, NULL, NULL)) != IOTHUB_CLIENT_OK)
	{
		printf("Unable to send telemetry message, error=%d", iothubClientResult);
	}

	IoTHubMessage_Destroy(messageHandle);
}

// CreateDeviceClientLLHandle creates the IOTHUB_DEVICE_CLIENT_LL_HANDLE based on environment configuration.
// If CONNECTION_SECURITY_TYPE_DPS is used, the call will block until DPS provisions the device.
//
static IOTHUB_DEVICE_CLIENT_LL_HANDLE CreateDeviceClientLLHandle(void)
{
	IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient = NULL;

	if (g_DeviceConfiguration.securityType == SECURITY_TYPE_CONNECTION_STRING)
	{
		if ((deviceClient = IoTHubDeviceClient_LL_CreateFromConnectionString(g_DeviceConfiguration.u.connectionString, MQTT_Protocol)) == NULL)
		{
			printf("Failure creating Iot Hub client.  Hint: Check your connection string");
		}
	}
#ifdef USE_PROV_MODULE_FULL
	else if ((deviceClient = CreateDeviceClientLLHandle_ViaDps(&g_DeviceConfiguration)) == NULL)
	{
		printf("Cannot retrieve IoT Hub connection information from DPS client");
	}
#endif /* USE_PROV_MODULE_FULL */

	return deviceClient;
}

//
// CreateAndConfigureDeviceClientHandle creates a IOTHUB_DEVICE_CLIENT_LL_HANDLE for this application, setting its
// ModelId along with various callbacks.
//
static IOTHUB_DEVICE_CLIENT_LL_HANDLE CreateAndConfigureDeviceClientHandle(void)
{
	IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient = NULL;
	IOTHUB_CLIENT_RESULT iothubClientResult;
	bool urlAutoEncodeDecode = true;
	int iothubInitResult;
	bool result;

	// Before invoking any IoT Hub Device SDK functionality, IoTHub_Init must be invoked.
	if ((iothubInitResult = IoTHub_Init()) != 0)
	{
		printf("Failure to initialize client, error=%d", iothubInitResult);
		result = false;
	}
	// Create the deviceClient.
	else if ((deviceClient = CreateDeviceClientLLHandle()) == NULL)
	{
		printf("Failure creating Iot Hub client.  Hint: Check your connection string or DPS configuration");
		result = false;
	}
	// Sets verbosity level.
	else if ((iothubClientResult = IoTHubDeviceClient_LL_SetOption(deviceClient, OPTION_LOG_TRACE, &g_hubClientTraceEnabled)) != IOTHUB_CLIENT_OK)
	{
		printf("Unable to set logging option, error=%d", iothubClientResult);
		result = false;
	}
	// Sets the name of ModelId for this  device.
	// This *MUST* be set before the client is connected to IoTHub.  We do not automatically connect when the 
	// handle is created, but will implicitly connect to subscribe for command and property callbacks below.
	else if ((iothubClientResult = IoTHubDeviceClient_LL_SetOption(deviceClient, OPTION_MODEL_ID, g_temperatureControllerModelId)) != IOTHUB_CLIENT_OK)
	{
		printf("Unable to set the ModelID, error=%d", iothubClientResult);
		result = false;
	}
	// Enabling auto url encode will have the underlying SDK perform URL encoding operations automatically for telemetry message properties.
	else if ((iothubClientResult = IoTHubDeviceClient_LL_SetOption(deviceClient, OPTION_AUTO_URL_ENCODE_DECODE, &urlAutoEncodeDecode)) != IOTHUB_CLIENT_OK)
	{
		printf("Unable to set auto Url encode option, error=%d", iothubClientResult);
		result = false;
	}
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
	// Setting the Trusted Certificate.  This is only necessary on systems without built in certificate stores.
	else if ((iothubClientResult = IoTHubDeviceClient_LL_SetOption(deviceClient, OPTION_TRUSTED_CERT, certificates)) != IOTHUB_CLIENT_OK)
	{
		printf("Unable to set the trusted cert, error=%d", iothubClientResult);
		result = false;
	}
#endif  // SET_TRUSTED_CERT_IN_SAMPLES
	// Retrieve all properties for the device and also subscribe for any future writable property update changes.
//	else if ((iothubClientResult = IoTHubDeviceClient_LL_GetPropertiesAndSubscribeToUpdatesAsync(deviceClient, TempControlComponent_UpdatedPropertyCallback, (void*)deviceClient)) != IOTHUB_CLIENT_OK)
//	{
//		printf("Unable to subscribe and get properties, error=%d", iothubClientResult);
//		result = false;
//	}
	else
	{
		result = true;
	}

	if (result == false)
	{
		IoTHubDeviceClient_LL_Destroy(deviceClient);
		deviceClient = NULL;

		if (iothubInitResult == 0)
		{
			IoTHub_Deinit();
		}
	}

	return deviceClient;
}

// AllocateThermostatComponents allocates subcomponents of this module.  These are implemented in separate .c 
// files and the thermostat components have handles to simulate how a more complex device might be composed.

static bool AllocateThermostatComponents(void)
{
	bool result;

	// CreateHandle creates handles to process the subcomponents of Temperature Controller (namely
	// thermostat1 and thermostat2) that require state and need to process callbacks from IoT Hub.  The other component,
	// deviceInfo, is so simple (one time send of data) as to not need a handle.
	if ((Handle1 = CreateHandle(g_thermostatComponent1Name)) == NULL)
	{
		printf("Unable to create component handle for %s", g_thermostatComponent1Name);
		result = false;
	}
	else
	{
		result = true;
	}

	if (result == false)
	{
		ThermostatComponent_Destroy(Handle1);
	}

	return result;
}

volatile uint8_t data_ready = 0;

int main(int argc,char ** argv)
{
	uint8_t slave_addr;
	const char *path = argv[1];
	int file,rc;

	if (argc == 1)
		errx(-1, "path [i2c address] [register]");

	if (argc > 2)
		slave_addr = strtoul(argv[2], NULL, 0);

	file = open(path, O_RDWR);
	if (file < 0)
		err(errno, "Tried to open '%s'", path);

	rc = ioctl(file, I2C_SLAVE, slave_addr);
	if (rc < 0)
		err(errno, "Tried to set device address '0x%02x'", slave_addr);

	HRC_Startup(file);

	IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient = NULL;

	g_DeviceConfiguration.modelId = g_temperatureControllerModelId; 
	g_DeviceConfiguration.enableTracing = g_hubClientTraceEnabled;

	if (GetConnectionSettingsFromEnvironment(&g_DeviceConfiguration) == false)
	{
		printf("Cannot read required environment variable");
	}

	else if (AllocateThermostatComponents() == false)
	{
		printf("Failure allocating components");
	}

	else if ((deviceClient = CreateAndConfigureDeviceClientHandle()) == NULL)
	{
		printf("Failure creating Iot Hub device client");
		ThermostatComponent_Destroy(Handle1);
	}

	else
	{
		printf("Successfully created device client.  Hit Control-C to exit program\n");
		int numberOfIterations = 0;

		while (true)
		{
			// incoming requests from the server and to do connection keep alives.
			if ((numberOfIterations % g_sendTelemetryPollInterval) == 0)
			{
				TempControlComponent_SendWorkingSet(deviceClient);
				ThermostatComponent_SendCurrentTemperature(Handle1, deviceClient, file);
				sleep(1);
			}

			IoTHubDeviceClient_LL_DoWork(deviceClient);
			numberOfIterations++;
		}

		// Free the memory allocated to track simulated thermostat.
		ThermostatComponent_Destroy(Handle1);

		// Clean up the IoT Hub SDK handle.
		IoTHubDeviceClient_LL_Destroy(deviceClient);
		// Free all IoT Hub subsystem.
		IoTHub_Deinit();
	}

	return 0;
}
