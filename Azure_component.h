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

#ifndef AZURE_COMPONENT_H
#define AZURE_COMPONENT_H
#include "parson.h"
#include "iothub_device_client_ll.h"

//
// Handle representing a thermostat component.
//
typedef void* HR_COMPONENT_HANDLE;

//
// CreateHandle allocates a handle to correspond to the thermostat controller.
// This operation is only for allocation and does NOT invoke any I/O operations.
//
HR_COMPONENT_HANDLE CreateHandle(const char* componentName);

//
// ThermostatComponent_Destroy frees resources associated with hrThermostatComponentHandle.
//
void ThermostatComponent_Destroy(HR_COMPONENT_HANDLE hrThermostatComponentHandle);

//
// ThermostatComponent_SendCurrentTemperature sends a telemetry message indicating the current temperature.
//
void ThermostatComponent_SendCurrentTemperature(HR_COMPONENT_HANDLE hrThermostatComponentHandle, IOTHUB_DEVICE_CLIENT_LL_HANDLE deviceClient, int file);

#endif

