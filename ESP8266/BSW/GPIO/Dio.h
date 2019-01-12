//
//  Dio.h
//  SmartHome
//
//  Created by Tran Xuan on 9/8/18.
//  Copyright © 2018 Tran Xuan. All rights reserved.
//

#ifndef Dio_h
#define Dio_h

#include <stdio.h>

void dioInit(void);
void dioSetOutput(uint8_t st);
uint8_t dioGetOutput(void);
void dioSetOutputWithPos(uint8_t pos, uint8_t st);
uint8_t dioGetOutputWithPos(uint8_t pos);
#define LED1_INIT()   gpio_enable(2, GPIO_OUTPUT)
#define LED2_INIT()   gpio_enable(16, GPIO_OUTPUT)
#define LED1_ON()     gpio_write(2, 0)
#define LED2_ON()     gpio_write(16, 0)
#define LED1_OFF()    gpio_write(2, 1)
#define LED2_OFF()    gpio_write(16, 1)
#define SW_INIT()     gpio_enable(5, GPIO_OUTPUT)
#define SW_ON()       gpio_write(5, 1)
#define SW_OFF()      gpio_write(5, 0)
#define SW_READ()     gpio_read(5)
#define GAS_INIT()    gpio_enable(4, GPIO_INPUT)
#define GAS_READ()    gpio_read(4)
#define BT_INIT()     gpio_set_pullup(4,1,1); gpio_enable(4, GPIO_INPUT)
#define BT_READ()     gpio_read(4)

#endif /* Dio_h */
