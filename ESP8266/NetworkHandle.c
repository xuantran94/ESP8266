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
#include "cJSON.h"
#include "JsonCustom.h"

//char* xWIFI_SSID = "default";
//char* xWIFI_PASS = "default";
//
//char* xWIFI_SSID = "Mathnasium-ToanTuDuyHoaKy";
//char* xWIFI_PASS = "@71081080";
char* xWIFI_SSID = "xuantran";
char* xWIFI_PASS = "01227379368";
//char* xWIFI_SSID = "NguyenTruc";
//char* xWIFI_PASS = "12345678";
//char* xWIFI_SSID = "xuan4gk";
//char* xWIFI_PASS = "01227379368";

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
    
    while(1){
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
            DEBUG_PRINT("WiFi: Connected\n\r");
            xSemaphoreGive( wifi_alive );
            taskYIELD();
        }
        
        while ((status = sdk_wifi_station_get_connect_status()) == STATION_GOT_IP) {
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

/*
 * This function will be call in Lwip in each event on netconn
 */
static void netCallback(struct netconn *conn, enum netconn_evt evt, uint16_t length)
{
    //Show some callback information (debug)
//    debug("sock:%u\tsta:%u\tevt:%u\tlen:%u\ttyp:%u\tfla:%02X\terr:%d", \
//          (uint32_t)conn,conn->state,evt,length,conn->type,conn->flags,conn->last_err);
    
    netconn_events events ;
    
    //If netconn got error, it is close or deleted, dont do treatments on it.
    if (conn->pending_err)
    {
        return;
    }
    //Treatments only on rcv events.
    switch (evt) {
        case NETCONN_EVT_RCVPLUS:
            events.nc = conn ;
            events.type = evt ;
            break;
        default:
            return;
            break;
    }
    
    //Send the event to the queue
    xQueueSend(xQueue_events, &events, 1000);
    
}

/*
 *  Initialize a server netconn and listen port
 */
static void set_tcp_server_netconn(struct netconn **nc, uint16_t port, netconn_callback callback)
{
    if(nc == NULL)
    {
        DEBUG_PRINT("%s: netconn missing .\n",__FUNCTION__);
        return;
    }
    *nc = netconn_new_with_callback(NETCONN_TCP, netCallback);
    if(!*nc) {
        DEBUG_PRINT("Status monitor: Failed to allocate netconn.\n");
        return;
    }
    netconn_set_nonblocking(*nc,NETCONN_FLAG_NON_BLOCKING);
    //netconn_set_recvtimeout(*nc, 10);
    netconn_bind(*nc, IP_ADDR_ANY, port);
    netconn_listen(*nc);
}

/*
 *  Close and delete a socket properly
 */
static void close_tcp_netconn(struct netconn *nc)
{
    nc->pending_err=ERR_CLSD; //It is hacky way to be sure than callback will don't do treatment on a netconn closed and deleted
    netconn_close(nc);
    netconn_delete(nc);
}

/*
 *  This task manage each netconn connection without block anything
 */
void nonBlockingTCP(void *pvParameters)
{
    
    struct netconn *nc = NULL; // To create servers
    
    set_tcp_server_netconn(&nc, ECHO_PORT_1, netCallback);
    printf("Server netconn %u ready on port %u.\n",(uint32_t)nc, ECHO_PORT_1);
    
    struct netbuf *netbuf = NULL; // To store incoming Data
    struct netconn *nc_in = NULL; // To accept incoming netconn
    //
    char buf[50];
    char* buffer;
    uint16_t len_buf;
    
    while(1) {
        
        netconn_events events;
        xQueueReceive(xQueue_events, &events, portMAX_DELAY); // Wait here an event on netconn
        
        if (events.nc->state == NETCONN_LISTEN) // If netconn is a server and receive incoming event on it
        {
            printf("Client incoming on server %u.\n", (uint32_t)events.nc);
            int err = netconn_accept(events.nc, &nc_in);
            if (err != ERR_OK)
            {
                if(nc_in)
                    netconn_delete(nc_in);
            }
            DEBUG_PRINT("New client is %u.\n",(uint32_t)nc_in);
            ip_addr_t client_addr; //Address port
            uint16_t client_port; //Client port
            netconn_peer(nc_in, &client_addr, &client_port);
            snprintf(buf, sizeof(buf), "Your address is %d.%d.%d.%d:%u.\r\n",
                     ip4_addr1(&client_addr), ip4_addr2(&client_addr),
                     ip4_addr3(&client_addr), ip4_addr4(&client_addr),
                     client_port);
            netconn_write(nc_in, buf, strlen(buf), NETCONN_COPY);
        }
        else if(events.nc->state != NETCONN_LISTEN) // If netconn is the client and receive data
        {
            if ((netconn_recv(events.nc, &netbuf)) == ERR_OK) // data incoming ?
            {
                do
                {
                    netbuf_data(netbuf, (void*)&buffer, &len_buf);
                    xWIFI_SSID =get_Json_String("ssid",buffer);
                    xWIFI_PASS =get_Json_String("pass",buffer);
                    cJSON *networkStatus = NULL;
                    networkStatus = cJSON_CreateObject();
                    cJSON_AddNumberToObject(networkStatus, "status", status);
                    cJSON_AddStringToObject(networkStatus, "ssid", xWIFI_SSID);
                    cJSON_AddStringToObject(networkStatus, "pass", xWIFI_PASS);
                    char *out = cJSON_PrintUnformatted(networkStatus);
                    DEBUG_PRINT("Client %u send: %s\n",(uint32_t)events.nc,buffer);
                    DEBUG_PRINT("response %s\n", out);
                    out[strlen(out)+1] = out[strlen(out)];
                    out[strlen(out)] = '\n';
                    //memcpy(buf, out, strlen(out));
                    netconn_write(events.nc, out, strlen(out), NETCONN_COPY);
                    free(out);
                    DEBUG_PRINT("connect to %s with %s\r\n",xWIFI_SSID,xWIFI_PASS);
                }
                while (netbuf_next(netbuf) >= 0);
                netbuf_delete(netbuf);
            }
            else
            {
                close_tcp_netconn(events.nc);
                DEBUG_PRINT("Error read netconn %u, close it \n",(uint32_t)events.nc);
            }
        }
    }
}

