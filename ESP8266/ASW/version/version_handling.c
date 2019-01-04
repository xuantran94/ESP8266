//
//  version_handling.c
//  SmartHome
//
//  Created by Tran Xuan on 7/4/18.
//  Copyright © 2018 Tran Xuan. All rights reserved.
//



#include "version_handling.h"

uint8_t isNeedUpdate(char *version){
    uint8_t version_array[4]={0};
    uint8_t index=0;
    while(*version != 0x00){
        if(*version>0x2f&&*version<0x3a){
            version_array[index]=version_array[index]*10+(*version++-0x30);
        }else{
            index++;
            *version++;
            if(4==index) break;
        }
    }
//    printf("current version is %d.%d.%d;%d\r\n",DEVICE,RELEASE,BUGFIX,REVISION);
//    printf("version is %d.%d.%d;%d\r\n",version_array[0],version_array[1],version_array[2],version_array[3]);
    if(version_array[0]!=DEVICE) return 0;
    if(version_array[1]<RELEASE) return 0;
    if(version_array[2]<BUGFIX)  return 0;
    if(version_array[3]<=REVISION)return 0;
    return 1;
}


