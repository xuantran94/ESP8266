//
//  NetworkHandle.c
//  SmartHome
//
//  Created by Tran Xuan on 8/8/18.
//  Copyright © 2018 Tran Xuan. All rights reserved.
//

#include "NetworkHandle.h"
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <espressif/esp_sta.h>
#include <espressif/esp_wifi.h>
#include <semphr.h>
#include <espressif/esp_common.h>
#include <esp8266.h>
#include <esp/uart.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <dhcpserver.h>
#include <lwip/api.h>
#include "../../BSW/GPIO/dio.h"
#include "Lib/Json/cJSON.h"
#include "../Json/JsonCustom.h"

//char* xWIFI_SSID = "default";
//char* xWIFI_PASS = "default";
////
//char* xWIFI_SSID = "Mathnasium-ToanTuDuyHoaKy";
//char* xWIFI_PASS = "@71081080";
//char* xWIFI_SSID = "xuantran";
//char* xWIFI_PASS = "01227379368";
char* xWIFI_SSID = "NguyenTruc";
char* xWIFI_PASS = "12345678";
//char* xWIFI_SSID = "xuan4gk";
//char* xWIFI_PASS = "01227379368";
//char* xWIFI_SSID = "Xuantran";
//char* xWIFI_PASS = "99999999";

#define DEBUG
#define MODULE "Network"
#if defined(DEBUG)
# ifndef MODULE
#  error "Module not define"
# endif
# define DEBUG_PRINT(fmt, args ...) \
printf("[%s]\t" fmt "\n", MODULE, ## args)
#else
# define DEBUG_PRINT(fmt, args ...) /* Don't do anything in release builds */
#endif


SemaphoreHandle_t wifi_alive;
QueueHandle_t xQueue_events;
typedef struct {
    struct netconn *nc ;
    uint8_t type ;
} netconn_events;
uint8_t status  = 0;
#define ECHO_PORT_1 50


void  wifi_task(void *pvParameters)
{
    LED1_INIT();
    while(1){
        LED1_OFF();
        struct sdk_station_config config;
        memset(config.ssid, 0, 32);
        memset(config.password, 0, 64);
        memcpy(config.ssid, xWIFI_SSID, strlen(xWIFI_SSID));
        memcpy(config.password, xWIFI_PASS, strlen(xWIFI_PASS));
        sdk_wifi_station_set_config(&config);
        
        uint8_t retries = 10;
        DEBUG_PRINT("connect to %s, with %s",config.ssid,config.password);
        while ((status != STATION_GOT_IP) && (retries)){
            status = sdk_wifi_station_get_connect_status();
            DEBUG_PRINT("%s: status = %d\n\r", __func__, status );
            if( status == STATION_WRONG_PASSWORD ){
                DEBUG_PRINT("WiFi: wrong password\n\r");
                break;
            } else if( status == STATION_NO_AP_FOUND ) {
                DEBUG_PRINT("WiFi: AP not found\n\r");
                break;
            } else if( status == STATION_CONNECT_FAIL ) {
                DEBUG_PRINT("WiFi: connection failed\r\n");
                break;
            } else{
                DEBUG_PRINT("WiFi: unknown error failed\r\n");
            }
            vTaskDelay( 5000 / portTICK_PERIOD_MS );
            --retries;
        }

        if (status == STATION_GOT_IP) {
            LED1_ON();
            DEBUG_PRINT("WiFi: Connected\n\r");
            xSemaphoreGive( wifi_alive );
            taskYIELD();
        }
        
        while ((status = sdk_wifi_station_get_connect_status()) == STATION_GOT_IP) {
            LED1_ON();
            xSemaphoreGive( wifi_alive );
            taskYIELD();
        }
        vTaskDelay( 1000 / portTICK_PERIOD_MS );
    }
}

char *  get_my_id(void)
{
    // Use MAC address for Station as unique ID
    static char my_id[13];
    static bool my_id_done = false;
    int8_t i;
    uint8_t x;
    if (my_id_done)
        return my_id;
    if (!sdk_wifi_get_macaddr(STATION_IF, (uint8_t *)my_id))
        return NULL;
    for (i = 5; i >= 0; --i)
    {
        x = my_id[i] & 0x0F;
        if (x > 9) x += 7;
        my_id[i * 2 + 1] = x + '0';
        x = my_id[i] >> 4;
        if (x > 9) x += 7;
        my_id[i * 2] = x + '0';
    }
    my_id[12] = '\0';
    my_id_done = true;
    return my_id;
}
