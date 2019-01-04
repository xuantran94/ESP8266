//
//  NetworkHandle.h
//  SmartHome
//
//  Created by Tran Xuan on 8/8/18.
//  Copyright © 2018 Tran Xuan. All rights reserved.
//

#ifndef NetworkHandle_h
#define NetworkHandle_h

#include "cJSON.h"

void  wifi_task(void *pvParameters);
char *  get_my_id(void);
void nonBlockingTCP(void *pvParameters);

#endif /* NetworkHandle_h */
