//
//  Dio.c
//  SmartHome
//
//  Created by Tran Xuan on 9/8/18.
//  Copyright © 2018 Tran Xuan. All rights reserved.
//

#include "Dio.h"
#include "espressif/esp_common.h"
#include "espressif/esp_system.h"
#include <espressif/spi_flash.h>
#include <espressif/esp_system.h>
#include <espressif/esp_common.h>
#include <espressif/esp_system.h>
#include "esp/gpio.h"


uint8_t dioStatus=0xff;
void dioInit(void){
    gpio_enable(5, GPIO_OUTPUT);
    gpio_enable(16, GPIO_OUTPUT);
    gpio_write(5, 0);
    gpio_write(16, 0);
}
void dioSetOutput(uint8_t st){
    if(dioStatus==0xff) dioStatus=0;
    dioStatus = st;
    //gpio_write(2, st&&0x01);
    gpio_write(5, dioStatus&&0x01);
    gpio_write(16, dioStatus&&0x01);
}
uint8_t dioGetOutput(void){
    return dioStatus;
}
void dioSetOutputWithPos(uint8_t pos, uint8_t st){
    if(dioStatus==0xff) dioStatus=0;
    dioStatus = (dioStatus&(~1<<pos))|(st<<pos);
    gpio_write(5, dioStatus&&0x01);
    gpio_write(16, dioStatus&&0x01);
}
uint8_t dioGetOutputWithPos(uint8_t pos){
    return dioStatus&(1<<pos);
}
