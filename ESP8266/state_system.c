//
//  state_system.c
//  SmartHome
//
//  Created by Tran Xuan on 8/7/18.
//  Copyright © 2018 Tran Xuan. All rights reserved.
//

#include "state_system.h"

enum SystemState sys_state = Configurattion;
void SystemRunning(void){
    switch (sys_state) {
        case Configurattion:
            
            break;
            
        default:
            break;
    }
}
