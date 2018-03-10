#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "apps/sntp/sntp.h"
#include "iot_wifi_conn.h"
#include "aws_iot_task.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

const char *AWSIOTTAG = "aws_iot";
#define ESP_INTR_FLAG_DEFAULT 0

static void app_sntp_init()
{
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    char strftime_buf[64];
    ESP_LOGI("Time", "Initializing SNTP\n");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, (char*) "pool.ntp.org");
    sntp_init();

    while (timeinfo.tm_year < (2016 - 1900) && ++retry < 30) {
        ESP_LOGI("Time", "Waiting for system time to be set... (%d/%d)\n",
                retry, 30);
        vTaskDelay(3000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    setenv("TZ", "GMT-8", 1); // Set timezone to Shanghai time
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI("Time", "%s\n", strftime_buf);
}

void connect_to_wifi()
{
    CWiFi *my_wifi = CWiFi::GetInstance(WIFI_MODE_STA);
    printf("connecting to wifi: %s\n...", EXAMPLE_WIFI_SSID);
    my_wifi->Connect(EXAMPLE_WIFI_SSID, EXAMPLE_WIFI_PASS, portMAX_DELAY);
}
void start_aws_iot_task()
{
    const size_t stack_size = 36*1024;
    xTaskCreatePinnedToCore(&aws_iot_task, "aws_iot_task", stack_size, NULL, 5, NULL, 1);
}
extern "C" void app_main()
{
    connect_to_wifi();
    app_sntp_init();
    start_aws_iot_task();

}
