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
#include "Lib/OTA/http_client_ota.h"
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
#include "rboot-api.h"
#include "rboot.h"
#include "Conf/define.h"
#include "BSW/GPIO/Dio.h"

#define vTaskDelayMs(ms) vTaskDelay((ms) / portTICK_PERIOD_MS)

/*
 * How to test
 * cd test_file
 * python -m SimpleHTTPServer 8000
 * fill missing define SERVER and PORT, in your private_ssid_config.h
 * Ready for test.
 */


#define BINARY_PATH     "/firmware.bin"
#define SHA256_PATH     "/firmware.sha256"
#define VERSION_PATH    "/firmware.version"




// Default
#define SERVER "firmwareserver-thanhxuantran1994779304.codeanyapp.com"
#define PORT "8000"

#ifndef SERVER
#error "Server address is not defined define it:`192.168.X.X`"
#endif

#ifndef PORT
#error "Port is not defined example:`8080`"
#endif


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

#define EVENTS_QUEUE_SIZE 10

extern QueueHandle_t xQueue_events;
typedef struct {
    struct netconn *nc ;
    uint8_t type ;
} netconn_events;

extern SemaphoreHandle_t wifi_alive;
///* This task uses the high level GPIO API (esp_gpio.h) to blink an LED.
// *
// */
void blinkenTask(void *pvParameters)
{
    gpio_enable(2, GPIO_OUTPUT);
    while(1) {
        gpio_write(2, 1);
        DEBUG_PRINT("bum..");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_write(2, 0);
        vTaskDelay(1000/ portTICK_PERIOD_MS);
    }
}

static inline void ota_error_handling(OTA_err err) {
    //printf("Error:");
    
    switch(err) {
        case OTA_DNS_LOOKUP_FALLIED:
            printf("DNS lookup has fallied\n");
            break;
        case OTA_SOCKET_ALLOCATION_FALLIED:
            printf("Impossible allocate required socket\n");
            break;
        case OTA_SOCKET_CONNECTION_FALLIED:
            printf("Server unreachable, impossible connect\n");
            break;
        case OTA_SHA_DONT_MATCH:
            printf("Sha256 sum does not fit downloaded sha256\n");
            break;
        case OTA_REQUEST_SEND_FALLIED:
            printf("Impossible send HTTP request\n");
            break;
        case OTA_DOWLOAD_SIZE_NOT_MATCH:
            printf("Dowload size don't match with server declared size\n");
            break;
        case OTA_ONE_SLOT_ONLY:
            printf("rboot has only one slot configured, impossible switch it\n");
            break;
        case OTA_FAIL_SET_NEW_SLOT:
            printf("rboot cannot switch between rom\n");
            break;
        case OTA_IMAGE_VERIFY_FALLIED:
            printf("Dowloaded image binary checsum is fallied\n");
            break;
        case OTA_UPDATE_DONE:
            printf("Ota has completed upgrade process, all ready for system software reset\n");
            break;
        case OTA_HTTP_OK:
            printf("HTTP server has response 200, Ok\n");
            break;
        case OTA_HTTP_NOTFOUND:
            printf("HTTP server has response 404, file not found\n");
            break;
        case UP_TO_DATE:
            //printf("Already up to date!!\n");
            break;
    }
}


static void ota_task(void *PvParameter)
{
    uint32_t freeheap = sdk_system_get_free_heap_size();
    // Wait until we have joined AP and are assigned an IP *
    while (sdk_wifi_station_get_connect_status() != STATION_GOT_IP)
        vTaskDelayMs(100);
    
    while (1) {
        vTaskDelayMs(5000);
        
        OTA_err err;
        // Remake this task until ota work
        
        //DEBUG_PRINT("Free heap delta = %d  sector = %d", sdk_system_get_free_heap_size());//-freeheap,SECTOR_SIZE);
        freeheap = sdk_system_get_free_heap_size();
        err = ota_update((ota_info *) PvParameter);
        
        ota_error_handling(err);
        
        if(err != OTA_UPDATE_DONE) {
            vTaskDelayMs(1000);
            continue;
        }
        vTaskDelayMs(1000);
        printf("Delay 1\n");
        vTaskDelayMs(1000);
        printf("Delay 2\n");
        vTaskDelayMs(1000);
        printf("Delay 3\n");
        printf("Reset\n");
//
        sdk_system_restart();
    }
}

static ota_info info = {
    .server      = SERVER,
    .port        = PORT,
    .binary_path = BINARY_PATH,
    .sha256_path = SHA256_PATH,
    .version_path= VERSION_PATH,
};

// restore data from eeprom
void System_restore(void){
    
}
//initialise system
void System_Init(void){
    uart_set_baud(0, 115200);
    dioInit();
    DEBUG_PRINT("Mission begin --------------------------------\r\n");
    DEBUG_PRINT("SDK version:%s\n", sdk_system_get_sdk_version());
    char* id = get_my_id();
    DEBUG_PRINT("Chip id is :%s with lenght =%d\n", id,strlen(id));
    
    sdk_wifi_set_opmode(STATIONAP_MODE);
    struct ip_info ap_ip;
    IP4_ADDR(&ap_ip.ip, 172, 16, 0, 1);
    IP4_ADDR(&ap_ip.gw, 0, 0, 0, 0);
    IP4_ADDR(&ap_ip.netmask, 255, 255, 0, 0);
    sdk_wifi_set_ip_info(1, &ap_ip);
    
    struct sdk_softap_config ap_config = {
        //.ssid = "84F3EBB7388B",
        .ssid_hidden = 0,
        .channel = 3,
        //.ssid_len = strlen(id),
        .authmode = AUTH_WPA2_PSK,
        .password = "01227379368",
        .max_connection = 1,
        .beacon_interval = 100,
    };
    memcpy(ap_config.ssid, id, strlen(id)+1);
    ap_config.ssid_len = strlen(id);
    //memset(ap_config.ssid+strlen(id), 0, 32-strlen(id));
    //memcpy(ap_config.password, AP_PSK, strlen(AP_PSK));
    sdk_wifi_softap_set_config(&ap_config);
    ip_addr_t first_client_ip;
    IP4_ADDR(&first_client_ip, 172, 16, 0, 2);
    dhcpserver_start(&first_client_ip, 4);
    printf("DHCP started\n");
}
void  heap_task(void *pvParameters){
    while (1) {
        DEBUG_PRINT("Free heap delta = %d  sector = %d", sdk_system_get_free_heap_size());
        vTaskDelayMs(100);
    }
    
}
const Index[NUM_HOST]={0,1,2,3,4,5,6,7,8,9};
void user_init(void)
{
    System_Init();
    vSemaphoreCreateBinary(wifi_alive);
    xQueue_events = xQueueCreate( EVENTS_QUEUE_SIZE, sizeof(netconn_events));
    xTaskCreate(blinkenTask, "blinkenTask", 256, NULL, 2, NULL);
    xTaskCreate(&wifi_task, "wifi_task",  256, NULL, 2, NULL);
    //xTaskCreate(nonBlockingTCP, "lwiptest_noblock", 512, NULL, 2, NULL);
    //xTaskCreate(&heap_task, "heap", 256, NULL, 4, NULL);
    //xTaskCreate(&heap_task, "heap_task",  256, NULL, 2, NULL);
    xTaskCreate(ota_task, "get_task", 4096, &info, 2, NULL);
    for (uint8_t i=0;i<NUM_HOST;i++){
        xTaskCreate(&mqtt_task, "mqtt_task", 1024, (Index+i), 2, NULL);
    }
    //xTaskCreate(&adc_task, "adc_task",  256, NULL, 2, NULL);
}


