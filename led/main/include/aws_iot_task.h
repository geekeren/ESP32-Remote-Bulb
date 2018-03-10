#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "apps/sntp/sntp.h"

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_shadow_interface.h"
#include "driver/gpio.h"

#define AWS_IOT_MQTT_CLIENT_ID "esp32_led_mqqt"
#define AWS_IOT_SHADOW_CLIENT_ID "esp32_led_shadow"
#define AWS_IOT_MY_THING_NAME "ESP32-LED"
#define MAX_LENGTH_OF_UPDATE_JSON_BUFFER 200

#ifndef _IOT_AWS_IOT_TASK_H_
#define _IOT_AWS_IOT_TASK_H_

#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD

void aws_iot_task(void *param);

#endif
