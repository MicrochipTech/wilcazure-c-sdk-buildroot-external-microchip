/*
 * Copyright 2023 Microchip
 * 		  Nikhil Patil <nikhil.patil@microchip.com>
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


#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>
#include "azure_c_shared_utility/xlogging.h"
#include <stdbool.h>

typedef enum CONNECTION_SECURITY_TYPE_TAG
{
	SECURITY_TYPE_CONNECTION_STRING,
	CONNECTION_SECURITY_TYPE_DPS
} CONNECTION_SECURITY_TYPE;

#ifdef USE_PROV_MODULE_FULL

typedef struct DPS_CONNECTION_AUTH_TAG
{
	const char* endpoint;
	const char* idScope;
	const char* deviceId;
	const char* deviceKey;
} DPS_CONNECTION_AUTH;
#endif /* USE_PROV_MODULE_FULL */
#endif

//
// DEVICE_CONFIGURATION is used to setup the IOTHUB_DEVICE_CLIENT_LL_HANDLE
//
typedef struct DEVICE_CONFIGURATION_TAG
{
	// Whether we're using connection string or DPS
	CONNECTION_SECURITY_TYPE securityType;
	// The connection string or DPS security information
	union {
		char* connectionString;
#ifdef USE_PROV_MODULE_FULL
		DPS_CONNECTION_AUTH dpsConnectionAuth;
#endif
	} u;
	// ModelId of this device
	const char* modelId;
	// Whether more verbose tracing is enabled for the IoT Hub client
	bool enableTracing;
} DEVICE_CONFIGURATION;


// Environment variable used to specify how app connects to hub and the two possible values.
static const char g_securityTypeEnvironmentVariable[] = "IOTHUB_DEVICE_SECURITY_TYPE";
static const char g_securityTypeConnectionStringValue[] = "connectionString";
static const char g_securityTypeDpsValue[] = "DPS";

// Environment variable used to specify this application's connection string.
static const char g_connectionStringEnvironmentVariable[] = "IOTHUB_DEVICE_CONNECTION_STRING";

#ifdef USE_PROV_MODULE_FULL
// Environment variable used to specify this application's DPS id scope.
static const char g_dpsIdScopeEnvironmentVariable[] = "IOTHUB_DEVICE_DPS_ID_SCOPE";

// Environment variable used to specify this application's DPS device id.
static const char g_dpsDeviceIdEnvironmentVariable[] = "IOTHUB_DEVICE_DPS_DEVICE_ID";

// Environment variable used to specify this application's DPS device key.
static const char g_dpsDeviceKeyEnvironmentVariable[] = "IOTHUB_DEVICE_DPS_DEVICE_KEY";

// Environment variable used to optionally specify this application's DPS id scope.
static const char g_dpsEndpointEnvironmentVariable[] = "IOTHUB_DEVICE_DPS_ENDPOINT";

// Global provisioning endpoint for DPS if one is not specified via the environment.
static const char g_dps_DefaultGlobalProvUri[] = "global.azure-devices-provisioning.net";
#endif


static bool GetConnectionStringFromEnvironment(DEVICE_CONFIGURATION* AzureDeviceConfiguration)
{
	bool result;

	if ((AzureDeviceConfiguration->u.connectionString = getenv(g_connectionStringEnvironmentVariable)) == NULL)
	{
		printf("Cannot read environment variable=%s", g_connectionStringEnvironmentVariable);
		result = false;
	}
	else
	{
		AzureDeviceConfiguration->securityType = SECURITY_TYPE_CONNECTION_STRING;
		result = true;
	}

	return result;
}


static bool GetDpsFromEnvironment(DEVICE_CONFIGURATION* AzureDeviceConfiguration)
{
#ifndef USE_PROV_MODULE_FULL
	// Explain to user misconfiguration.  The "run_e2e_tests" must be set to OFF because otherwise
	// the e2e's test HSM layer and symmetric key logic will conflict.
	(void)AzureDeviceConfiguration;
	printf("DPS based authentication was requested via environment variables, but DPS is not enabled.");
	printf("DPS is an optional component of the Azure IoT C SDK.  It is enabled with symmetric keys at CMake time by");
	printf("passing <-Duse_prov_client=ON -Dhsm_type_symm_key=ON -Drun_e2e_tests=OFF> to CMake's command line");
	return false;
#else
	bool result;

	if ((AzureDeviceConfiguration->u.dpsConnectionAuth.endpoint = getenv(g_dpsEndpointEnvironmentVariable)) == NULL)
	{
		// We will fall back to standard endpoint if one is not specified
		AzureDeviceConfiguration->u.dpsConnectionAuth.endpoint = g_dps_DefaultGlobalProvUri;
	}

	if ((AzureDeviceConfiguration->u.dpsConnectionAuth.idScope = getenv(g_dpsIdScopeEnvironmentVariable)) == NULL)
	{
		printf("Cannot read environment variable=%s", g_dpsIdScopeEnvironmentVariable);
		result = false;
	}
	else if ((AzureDeviceConfiguration->u.dpsConnectionAuth.deviceId = getenv(g_dpsDeviceIdEnvironmentVariable)) == NULL)
	{
		printf("Cannot read environment variable=%s", g_dpsDeviceIdEnvironmentVariable);
		result = false;
	}
	else if ((AzureDeviceConfiguration->u.dpsConnectionAuth.deviceKey = getenv(g_dpsDeviceKeyEnvironmentVariable)) == NULL)
	{
		printf("Cannot read environment variable=%s", g_dpsDeviceKeyEnvironmentVariable);
		result = false;
	}
	else
	{
		AzureDeviceConfiguration->securityType = CONNECTION_SECURITY_TYPE_DPS;
		result = true;
	}

	return result;
#endif // USE_PROV_MODULE_FULL
}

bool GetConnectionSettingsFromEnvironment(DEVICE_CONFIGURATION* AzureDeviceConfiguration)
{
	const char* securityTypeString;
	bool result;

	if ((securityTypeString = getenv(g_securityTypeEnvironmentVariable)) == NULL)
	{
		printf("Cannot read environment variable=%s", g_securityTypeEnvironmentVariable);
		result = false;
	}
	else
	{
		if (strcmp(securityTypeString, g_securityTypeConnectionStringValue) == 0)
		{
			result = GetConnectionStringFromEnvironment(AzureDeviceConfiguration);
		}
		else if (strcmp(securityTypeString, g_securityTypeDpsValue) == 0)
		{
			result = GetDpsFromEnvironment(AzureDeviceConfiguration);
		}
		else
		{
			printf("Environment variable %s must be either %s or %s", g_securityTypeEnvironmentVariable, g_securityTypeConnectionStringValue, g_securityTypeDpsValue);
			result = false;
		}
	}
	return result;
}
