//
//  aw_convert_mp4_to_flv.h
//  pushStreamInC
//
//  Created by kaso on 19/7/16.
//  Copyright © 2016年 xcyo. All rights reserved.
//

#ifndef aw_convert_mp4_to_flv_h
#define aw_convert_mp4_to_flv_h

#include <stdio.h>
#include "aw_parse_mp4.h"
#include "aw_encode_flv.h"

extern void aw_convert_parsed_mp4_to_flv_data(aw_parsed_mp4 *parsed_mp4, aw_data ** flv_data);

extern void aw_convert_mp4_to_flv_test(const uint8_t *mp4_file_data, size_t len, void (*write_to_file)(aw_data *));

#endif /* aw_convert_mp4_to_flv_h */
