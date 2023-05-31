CROSS_COMPILE := arm-linux-gnueabi-

CC      := $(CROSS_COMPILE)gcc
LD      := $(CROSS_COMPILE)gcc

CFLAGS  := -Wall -std=gnu11 -g -D_REENTRANT


AZURE_BASE := $(shell ls ../ | grep -m 1 azure-iot-sdk-c)


AZURE_LIB_DIR := -L$(AZURE_BASE)/iothub_client \
	-L$(AZURE_BASE)/deps/parson \
	-L$(AZURE_BASE)/umock-c \
       	-L$(AZURE_BASE)/iothub_service_client \

AZURE_LIBS := -liothub_client_mqtt_ws_transport -liothub_client -lparson -lumock_c -liothub_service_client  -liothub_client_mqtt_transport -lc

AZURE_INC := -I$(AZURE_BASE)/deps/parson \
	-I$(AZURE_BASE)/iothub_client/inc \
	-I$(AZURE_BASE)/deps/umock-c/inc \
	-I$(AZURE_BASE)/CMakeFiles/parson.dir/deps/parson \
	-I$(AZURE_BASE)/umqtt/deps/umock-c/inc \
	-I$(AZURE_BASE)/umqtt/deps/azure-macro-utils-c/inc \
	-I$(AZURE_BASE)/c-utility/inc \

default : 
	$(CC) $(CFLAGS) SendDataToAzureCloud.c Azure_component.c   $(AZURE_LIBS) $(AZURE_LIB_DIR) $(AZURE_INC) -o SendDataToAzureCloud
