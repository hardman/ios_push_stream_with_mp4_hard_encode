//
//  aw_utils.h
//  pushStreamInC
//
//  Created by kaso on 11/7/16.
//  Copyright © 2016年 xcyo. All rights reserved.
//

#ifndef aw_utils_h
#define aw_utils_h

#include <stdio.h>
#include "aw_alloc.h"

#define AWLog(...)  \
do{ \
printf(__VA_ARGS__); \
printf("\n");\
}while(0)

#endif /* aw_utils_h */
