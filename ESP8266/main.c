#include "espressif/esp_common.h"
#include "espressif/esp_system.h"
#include <espressif/spi_flash.h>
#include <espressif/esp_system.h>
#include <espressif/esp_common.h>
#include <espressif/esp_system.h>
#include "esp/uart.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include <dhcpserver.h>
#include <queue.h>
#include "ASW/Network/NetworkHandle.h"
#include <semphr.h>
#include "ASW/MQTT/MQTT.h"
#include "arch/cc.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include <espressif/spi_flash.h>
#include <espressif/esp_system.h>
#include <espressif/esp_common.h>
#include <espressif/esp_system.h>
#include "mbedtls/sha256.h"
#include "Conf/define.h"
#include "BSW/GPIO/Dio.h"


#define vTaskDelayMs(ms) vTaskDelay((ms) / portTICK_PERIOD_MS)


#define DEBUG
#define MODULE "Main"
#if defined(DEBUG)
# ifndef MODULE
#  error "Module not define"
# endif
# define DEBUG_PRINT(fmt, args ...) \
printf("[%s]\t" fmt "\n", MODULE, ## args)
#else
# define DEBUG_PRINT(fmt, args ...) /* Don't do anything in release builds */
#endif


extern SemaphoreHandle_t wifi_alive;
//initialise system
void System_Init(void){
    uart_set_baud(0, 115200);
    //dioInit();
    SW_INIT();
    BT_INIT();
    DEBUG_PRINT("Mission begin --------------------------------\r\n");
    DEBUG_PRINT("SDK version:%s\n", sdk_system_get_sdk_version());
    char* id = get_my_id();
    DEBUG_PRINT("Chip id is :%s with lenght =%d\n", id,strlen(id));
    
    sdk_wifi_set_opmode(STATIONAP_MODE);
}
const Index[NUM_HOST]={0,1,2,3,4,5,6,7,8,9};
void user_init(void)
{
    System_Init();
    vSemaphoreCreateBinary(wifi_alive);
    xTaskCreate(&wifi_task, "wifi_task",  256, NULL, 2, NULL);
    for (uint8_t i=0;i<NUM_HOST;i++){
        xTaskCreate(&mqtt_task, "mqtt_task", 1024, (Index+i), 2, NULL);
    }
    xTaskCreate(&sensor_task, "sensor_task",  256, NULL, 2, NULL);
    //xTaskCreate(&button_task, "sensor_task",  256, NULL, 2, NULL);
}


