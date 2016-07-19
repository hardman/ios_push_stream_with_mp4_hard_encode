//
//  aw_mp4box.h
//  pushStreamInC
//
//  Created by kaso on 12/7/16.
//  Copyright © 2016年 xcyo. All rights reserved.
//

#ifndef aw_mp4box_h
#define aw_mp4box_h

#include <stdio.h>
#include "aw_dict.h"
#include "aw_data.h"

//mp4 文件
typedef struct aw_mp4_box{
    aw_dict *parsed_data;
    int32_t type;
    
    //inner
    int64_t size;
    char *type_string;
    char uuid[16];
    aw_data *data;
    aw_dict *children;
    struct aw_mp4_box *parent_box;
    struct aw_mp4_box *root_box;
} aw_mp4_box;

extern uint32_t aw_mp4_bytes_fm_ascii(uint8_t a, uint8_t b, uint8_t c, uint8_t d);

extern void aw_mp4_int32_to_ascii_str(uint32_t, char [5]);

extern aw_mp4_box *alloc_aw_mp4_box();
extern void free_aw_mp4_box(aw_mp4_box **);

extern void aw_mp4box_parse_data(aw_mp4_box *box, aw_data *mp4_data);

extern aw_mp4_box *aw_mp4box_find_box(aw_mp4_box *box, const char *type_name);

extern void aw_test_mp4_box(void *data, int32_t len);

#endif /* aw_mp4box_h */
