//
//  version_handling.h
//  SmartHome
//
//  Created by Tran Xuan on 7/4/18.
//  Copyright © 2018 Tran Xuan. All rights reserved.
//

#ifndef version_handling_h
#define version_handling_h

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define DEVICE      1
#define RELEASE     1
#define BUGFIX      0
#define REVISION    2
uint8_t isNeedUpdate(char *version);
#endif /* version_handling_h */
