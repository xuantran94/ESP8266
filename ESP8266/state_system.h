//
//  state_system.h
//  SmartHome
//
//  Created by Tran Xuan on 8/7/18.
//  Copyright © 2018 Tran Xuan. All rights reserved.
//

#ifndef state_system_h
#define state_system_h

enum SystemState
{
    initializition,
    Configurattion,
    Working,
    Err,
};
void SystemRunning(void);
#endif /* state_system_h */
