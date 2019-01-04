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

#endif /* Dio_h */
