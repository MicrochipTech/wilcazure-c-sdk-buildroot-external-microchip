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


#ifndef IOTHUB_CLIENT_PROPERTIES_H
#define IOTHUB_CLIENT_PROPERTIES_H

#include "umock_c/umock_c_prod.h"

#include <stddef.h>

#include "iothub_client_core_common.h"

#define IOTHUB_CLIENT_PROPERTY_REPORTED_STRUCT_VERSION_1 1

typedef struct IOTHUB_CLIENT_PROPERTY_REPORTED_TAG {
	int structVersion;
	const char* name;
	const char* value;
} IOTHUB_CLIENT_PROPERTY_REPORTED;

typedef struct IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE_TAG {
	int structVersion;
	const char* name;
	const char* value;
	int result;
	int ackVersion;
	const char* description;
} IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE;

#define IOTHUB_CLIENT_PROPERTY_TYPE_VALUES \
	IOTHUB_CLIENT_PROPERTY_TYPE_REPORTED_FROM_CLIENT, \
	IOTHUB_CLIENT_PROPERTY_TYPE_WRITABLE

MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUB_CLIENT_PROPERTY_TYPE, IOTHUB_CLIENT_PROPERTY_TYPE_VALUES);

#define IOTHUB_CLIENT_PROPERTY_VALUE_TYPE_VALUES \
	IOTHUB_CLIENT_PROPERTY_VALUE_STRING

MU_DEFINE_ENUM_WITHOUT_INVALID(IOTHUB_CLIENT_PROPERTY_VALUE_TYPE, IOTHUB_CLIENT_PROPERTY_VALUE_TYPE_VALUES);

#define IOTHUB_CLIENT_PROPERTY_PARSED_STRUCT_VERSION_1 1

typedef struct IOTHUB_CLIENT_PROPERTY_PARSED_TAG {
	int structVersion;
	IOTHUB_CLIENT_PROPERTY_TYPE propertyType;
	const char* componentName;
	const char* name;
	IOTHUB_CLIENT_PROPERTY_VALUE_TYPE valueType;
	union {
		const char* str;
	} value;
	size_t valueLength;
} IOTHUB_CLIENT_PROPERTY_PARSED;

MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_Properties_Serializer_CreateReported, const IOTHUB_CLIENT_PROPERTY_REPORTED*, properties, size_t, numProperties, const char*, componentName, unsigned char**, serializedProperties, size_t*, serializedPropertiesLength);

MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_Properties_Serializer_CreateWritableResponse, const IOTHUB_CLIENT_PROPERTY_WRITABLE_RESPONSE*, properties, size_t, numProperties, const char*, componentName, unsigned char**, serializedProperties, size_t*, serializedPropertiesLength);

MOCKABLE_FUNCTION(, void, IoTHubClient_Properties_Serializer_Destroy, unsigned char*, serializedProperties);

typedef struct IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_TAG* IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE;

MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_Properties_Deserializer_GetVersion, IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE, propertiesDeserializerHandle, int*, propertiesVersion);

MOCKABLE_FUNCTION(, IOTHUB_CLIENT_RESULT, IoTHubClient_Properties_Deserializer_GetNext, IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE, propertiesDeserializerHandle, IOTHUB_CLIENT_PROPERTY_PARSED*, property, bool*, propertySpecified);

MOCKABLE_FUNCTION(, void, IoTHubClient_Properties_DeserializerProperty_Destroy,  IOTHUB_CLIENT_PROPERTY_PARSED*, property);

MOCKABLE_FUNCTION(, void, IoTHubClient_Properties_Deserializer_Destroy, IOTHUB_CLIENT_PROPERTIES_DESERIALIZER_HANDLE, propertiesDeserializerHandle);

#endif /* IOTHUB_CLIENT_PROPERTIES_H */
