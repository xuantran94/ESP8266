//
//  MQTT.c
//  SmartHome
//
//  Created by Tran Xuan on 9/5/18.
//  Copyright © 2018 Tran Xuan. All rights reserved.
//

#include "MQTT.h"
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <espressif/esp_sta.h>
#include <espressif/esp_wifi.h>
#include <paho_mqtt_c/MQTTESP8266.h>
#include <paho_mqtt_c/MQTTClient.h>
#include <semphr.h>
//#include "NetworkHandle.h"
#include "../../Conf/define.h"
#include "Lib/Json/cJSON.h"
#include "../Json/JsonCustom.h"
#include "espressif/esp_common.h"
#include "espressif/Esp_system.h"
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
#include "http_client_ota.h"
#include <dhcpserver.h>
#include <queue.h>
#include "../Network/NetworkHandle.h"
#include <semphr.h>
#include "MQTT.h"
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
#include "http_client_ota.h"
#include "rboot-api.h"
#include "rboot.h"
#include "BSW/GPIO/Dio.h"

#define DEBUG
#define MODULE "MQTT"
#if defined(DEBUG)
# ifndef MODULE
#  error "Module not define"
# endif
# define DEBUG_PRINT(fmt, args ...) \
printf("[%s]\t" fmt "\n", MODULE, ## args)
#else
# define DEBUG_PRINT(fmt, args ...) /* Don't do anything in release builds */
#endif

//#define MQTT_HOST "test.mosquitto.org"
#define MQTT_PORT 1883

#define MQTT_USER NULL
#define MQTT_PASS NULL
#define PUB_MSG_LEN 32
#define vTaskDelayMs(ms) vTaskDelay((ms) / portTICK_PERIOD_MS)
extern SemaphoreHandle_t wifi_alive;
char* hostList[NUM_HOST] ={"test.mosquitto.org","iot.eclipse.org","broker.hivemq.com"};
struct MQTTHost{
    char* name;
    uint8_t tryTime;
    uint8_t status;
    QueueHandle_t publish_Q;
    QueueHandle_t publish_QG;
};
uint8_t numConnectedHost=0;
volatile bool bootTime = true;
volatile uint8_t expected_msg=0;
struct MQTTHost host[NUM_HOST];

void button_task(void *pvParameters){
    uint8_t btStatus = 1;
    while(1){
        btStatus = BT_READ();
        vTaskDelay(5);
        if((btStatus==1)&&(BT_READ()==0)){
            DEBUG_PRINT("toggle light");
            btStatus = 0;
            if(SW_READ()==1){
                SW_OFF();
                char  out[2] = {'0','\0'};
                //DEBUG_PRINT("response %s", out);
                for (uint8_t i =0;i<NUM_HOST; i++) {
                    if(host[i].status){
                        if (xQueueSend(host[i].publish_Q, (void *)out, 0) == pdFALSE) {
                            //printf("%s Publish queue overflow",host[i].name);
                            xQueueReceive(host[i].publish_Q, (void *)out, 0);
                        }
                    }
                }
            }else{
                SW_ON();
                char  out[2] = {'1','\0'};
                //DEBUG_PRINT("response %s", out);
                for (uint8_t i =0;i<NUM_HOST; i++) {
                    if(host[i].status){
                        if (xQueueSend(host[i].publish_Q, (void *)out, 0) == pdFALSE) {
                            //printf("%s Publish queue overflow",host[i].name);
                            xQueueReceive(host[i].publish_Q, (void *)out, 0);
                        }
                    }
                }
            }
        }
        //DEBUG_PRINT("bt=%d\n", btStatus);
    }
}
void  sensor_task(void *pvParameters){
    while (1) {
        uint16 adc_read = 0;
        for(uint8_t i=0;i<10;i++){
            adc_read+=sdk_system_adc_read();
            vTaskDelay(20);
        }
        DEBUG_PRINT("adc=%d\n", adc_read);
        
        float temp = adc_read*3.3*10.0/1024.0;
        DEBUG_PRINT("%f\n", temp);
        char out [50];
        snprintf(out, 50, "%f", temp);
        for (uint8_t i =0;i<NUM_HOST; i++) {
            if(host[i].status){
                if (xQueueSend(host[i].publish_Q, (void *)out, 0) == pdFALSE) {
                    printf("%s Publish queue overflow",host[i].name);
                    xQueueReceive(host[i].publish_Q, (void *)out, 0);
                }
            }
        }
        
        
        uint8_t gas_status = GAS_READ();
        DEBUG_PRINT("gas=%d\n", gas_status);
        if(gas_status){
            char  outg[2] = {'0','\0'};
            for (uint8_t i =0;i<NUM_HOST; i++) {
                if(host[i].status){
                    if (xQueueSend(host[i].publish_QG, (void *)outg, 0) == pdFALSE) {
                        printf("%s Publish queue overflow",host[i].name);
                        xQueueReceive(host[i].publish_Q, (void *)outg, 0);
                    }
                }
            }
        }else{
            char  outg[2] = {'1','\0'};
            for (uint8_t i =0;i<NUM_HOST; i++) {
                if(host[i].status){
                    if (xQueueSend(host[i].publish_QG, (void *)outg, 0) == pdFALSE) {
                        printf("%s Publish queue overflow",host[i].name);
                        xQueueReceive(host[i].publish_Q, (void *)outg, 0);
                    }
                }
            }
        }
        
        vTaskDelay(20);    //Read every 200milli Sec
    }
}
static void  topic_received(mqtt_message_data_t *md)
{
    DEBUG_PRINT("%s", "enter isr");
    DEBUG_PRINT("Free heap delta = %d  sector = %d", sdk_system_get_free_heap_size());
    int i;
    bootTime = false;
    char buff[PUB_MSG_LEN];
    mqtt_message_t *message = md->message;
    printf("Received: ");
    for( i = 0; i < md->topic->lenstring.len; ++i)
        printf("%c", md->topic->lenstring.data[ i ]);

    printf(" = ");

    for( i = 0; i < (int)message->payloadlen; ++i){
        printf("%c", ((char *)(message->payload))[i]);
        //buff[i] = ((char *)(message->payload))[i];
    }
    memcpy(buff, (char*) message->payload, (int)message->payloadlen);
    buff[(int)message->payloadlen]='\0';

    printf("\r\n");
    DEBUG_PRINT("len =%d", message->payloadlen);
    DEBUG_PRINT("recive %s", buff);
    if(buff[0]=='1'){
        SW_ON();
        DEBUG_PRINT("ON");
        char  out[2] = {'1','\0'};
        DEBUG_PRINT("response %s", out);
        for (uint8_t i =0;i<NUM_HOST; i++) {
            if(host[i].status){
                if (xQueueSend(host[i].publish_Q, (void *)out, 0) == pdFALSE) {
                    printf("%s Publish queue overflow",host[i].name);
                    xQueueReceive(host[i].publish_Q, (void *)out, 0);
                }
            }
        }
    }
    else if (buff[0]=='0'){
        SW_OFF();
        DEBUG_PRINT("OFF");
        char  out[2] = {'0','\0'};
        DEBUG_PRINT("response %s", out);
        for (uint8_t i =0;i<NUM_HOST; i++) {
            if(host[i].status){
                if (xQueueSend(host[i].publish_Q, (void *)out, 0) == pdFALSE) {
                    printf("%s Publish queue overflow",host[i].name);
                    xQueueReceive(host[i].publish_Q, (void *)out, 0);
                }
            }
        }
    }
//    cJSON *root = cJSON_Parse(buff);
//    if(root){
//        uint8_t status = 0xff;
//        if(cJSON_HasObjectItem(root, "msg")){
//            uint8_t msg;
//            msg= cJSON_GetObjectItem(root, "msg")->valueint;
//            if (msg>=expected_msg){
//                expected_msg=msg+1;
//                if(cJSON_HasObjectItem(root, "sw1")){
//                    status = cJSON_GetObjectItem(root, "sw1")->valueint;
//                    dioSetOutputWithPos(0, status);
//                    DEBUG_PRINT("statuf = %d", status);
//                }else{
//                    DEBUG_PRINT("%s", "dont have sw1");
//                }
//                if(dioGetOutput()!=0xff){
//                    cJSON *deviceStatus = NULL;
//                    deviceStatus = cJSON_CreateObject();
//                    cJSON_AddNumberToObject(deviceStatus, "msg", expected_msg);
//                    cJSON_AddNumberToObject(deviceStatus, "sw1", dioGetOutputWithPos(0));
//                    char *out = cJSON_PrintUnformatted(deviceStatus);
//                    DEBUG_PRINT("response %s", out);
//                    for (uint8_t i =0;i<NUM_HOST; i++) {
//                        if(host[i].status){
//                            if (xQueueSend(host[i].publish_Q, (void *)out, 0) == pdFALSE) {
//                                printf("%s Publish queue overflow",host[i].name);
//                                xQueueReceive(host[i].publish_Q, (void *)out, 0);
//                            }
//                        }
//                    }
//                    cJSON_Delete(deviceStatus);
//                    free(out);
//                }
//            }else{
//                DEBUG_PRINT("%s", "discard this msg");
//            }
//        }
//
//    }else{
//        DEBUG_PRINT("invalid json\r\n");
//    }
//    cJSON_Delete(root);
    

}

void  mqtt_task(void *pvParameters)
{
    LED2_INIT();
    LED2_OFF();
    uint8_t index = *(uint8_t*)pvParameters;
    host[index].publish_Q = xQueueCreate(3, PUB_MSG_LEN);
    host[index].publish_QG = xQueueCreate(3, PUB_MSG_LEN);
    host[index].name = hostList[index];
    host[index].tryTime = MAX_TRY_TIME;
    //initialzation the mqtt connection
    int ret         = 0;
    struct mqtt_network network;
    mqtt_client_t client   = mqtt_client_default;
    char mqtt_client_id[20];
    uint8_t mqtt_buf[100];
    uint8_t mqtt_readbuf[100];
    mqtt_packet_connect_data_t data = mqtt_packet_connect_data_initializer;
    mqtt_network_new( &network );
    memset(mqtt_client_id, 0, sizeof(mqtt_client_id));
    strcpy(mqtt_client_id, "ESPjj-");
    strcat(mqtt_client_id, get_my_id());
    
    while(1) {
        if(host[index].tryTime || !numConnectedHost){
            if(!numConnectedHost) host[index].tryTime = MAX_TRY_TIME;
            if(!host[index].tryTime){
                vTaskDelayMs(1000);
                taskYIELD();
                continue;
            }
            xSemaphoreTake(wifi_alive, portMAX_DELAY);
            DEBUG_PRINT("(Re)connecting to %s ...%d ",host[index].name,host[index].tryTime--);
            ret = mqtt_network_connect(&network, host[index].name, MQTT_PORT);
            if( ret ){
                DEBUG_PRINT("cannot connect to %s",host[index].name);
                vTaskDelayMs(1000);
                if(!host[index].tryTime){
                    DEBUG_PRINT("give up to connect to %s", host[index].name);
                }
                taskYIELD();
                continue;
            }
            mqtt_client_new(&client, &network, 5000, mqtt_buf, 100,mqtt_readbuf, 100);
            data.willFlag       = 0;
            data.MQTTVersion    = 3;
            data.clientID.cstring   = mqtt_client_id;
            data.username.cstring   = MQTT_USER;
            data.password.cstring   = MQTT_PASS;
            data.keepAliveInterval  = 10;
            data.cleansession   = 0;
            DEBUG_PRINT("Send MQTT connect ... ");
            ret = mqtt_connect(&client, &data);
            if(ret){
                DEBUG_PRINT("error: %d\n\r", ret);
                mqtt_network_disconnect(&network);
                taskYIELD();
                continue;
            }
            DEBUG_PRINT("connected to %s",host[index].name);
            host[index].status=1;
            numConnectedHost++;
            host[index].tryTime = MAX_TRY_TIME;
            char rTopic[16];
            char tTopic[16];
            char* tGas;
            char* tGasOut;
            strcpy(rTopic,"/");
            strcat(rTopic,get_my_id());
            strcat(rTopic, "R/");
            DEBUG_PRINT("subscribe %s",rTopic);
            mqtt_subscribe(&client, rTopic ,MQTT_QOS1, topic_received);
            strcpy(tTopic, rTopic);
            tTopic[strlen(rTopic)-2]='T';
            DEBUG_PRINT("subscribe %s",tTopic);
            mqtt_subscribe(&client, tTopic, MQTT_QOS1, topic_received);
            xQueueReset(host[index].publish_Q);
            //gas = ""
            LED2_ON();
            uint32_t delay = 1000;
            while (bootTime && (delay)) {
                delay--;
                vTaskDelayMs(1);
            }
            mqtt_unsubscribe(&client, tTopic);
            tGas = "/CC50E34A4B61R/";
            tGasOut = "/CC50E34A4B61O/";
//            if(SW_READ()==1){
//                char  out[2] = {'1','\0'};
//                //DEBUG_PRINT("response %s", out);
//                for (uint8_t i =0;i<NUM_HOST; i++) {
//                    if(host[i].status){
//                        if (xQueueSend(host[i].publish_Q, (void *)out, 0) == pdFALSE) {
//                            //printf("%s Publish queue overflow",host[i].name);
//                            xQueueReceive(host[i].publish_Q, (void *)out, 0);
//                        }
//                    }
//                }
//            }else{
//                char  out[2] = {'0','\0'};
//                //DEBUG_PRINT("response %s", out);
//                for (uint8_t i =0;i<NUM_HOST; i++) {
//                    if(host[i].status){
//                        if (xQueueSend(host[i].publish_Q, (void *)out, 0) == pdFALSE) {
//                            //printf("%s Publish queue overflow",host[i].name);
//                            xQueueReceive(host[i].publish_Q, (void *)out, 0);
//                        }
//                    }
//                }
//            }
            
            while(1){
                
                vTaskDelay(1);
                char msg[PUB_MSG_LEN];
                while(xQueueReceive(host[index].publish_QG, (void *)msg, 0) == pdTRUE){
                    msg[strlen(msg)] = '\0';
                    //DEBUG_PRINT("%s publish gas: %s to %s",host[index].name,msg,tGas);
                    mqtt_message_t message;
                    message.payload = msg;
                    message.payloadlen = strlen(msg)+1;
                    message.dup = 0;
                    message.qos = MQTT_QOS0;
                    message.retained = 0;
                    ret = mqtt_publish(&client, tGasOut, &message);
                    if(msg[0]=='1'){
                        
                        msg[0]= '0';
                        message.payload = msg;
                        ret = mqtt_publish(&client, tGas, &message);
                    }
                    if (ret != MQTT_SUCCESS ){
                        DEBUG_PRINT("%s error while publishing message: %d\n", host[index].name, ret );
                        break;
                    }
                }
                while(xQueueReceive(host[index].publish_Q, (void *)msg, 0) == pdTRUE){
                    msg[strlen(msg)] = '\0';
                    //DEBUG_PRINT("%s publish temp: %s to %s",host[index].name,msg,tTopic);
                    mqtt_message_t message;
                    message.payload = msg;
                    message.payloadlen = strlen(msg)+1;
                    message.dup = 0;
                    message.qos = MQTT_QOS1;
                    message.retained = 1;
                    ret = mqtt_publish(&client, tTopic, &message);
                    if (ret != MQTT_SUCCESS ){
                        DEBUG_PRINT("%s error while publishing message: %d\n", host[index].name, ret );
                        break;
                    }
                }
                
                ret = mqtt_yield(&client, 1000);
                if (ret == MQTT_DISCONNECTED)
                    break;
            }
            LED2_OFF();
            numConnectedHost--;
            host[index].status=0;
            DEBUG_PRINT("%s Connection dropped, request restart\n\r",host[index].name);
            mqtt_network_disconnect(&network);
            taskYIELD();
        }
    }
}
