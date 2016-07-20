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
#include "aw_rtmp.h"

//将mp4数据转为flv数据，可以保存为文件
extern void aw_convert_parsed_mp4_to_flv_data(aw_parsed_mp4 *parsed_mp4, aw_data ** flv_data);

//将mp4数据转为flv数据，并发送至rtmp服务器
//打开rtmp stream
extern int8_t aw_open_rtmp_context_for_parsed_mp4(const char *rtmp_url, aw_rtmp_state_changed_cb state_changed_cb);
//关闭rtmp stream
extern void aw_close_rtmp_context_for_parsed_mp4();
//将mp4转为flv并发送至flv stream
extern void aw_convert_parsed_mp4_to_flv_stream(const uint8_t *mp4_file_data, size_t len);

//测试mp4转flv
extern void aw_convert_mp4_to_flv_test(const uint8_t *mp4_file_data, size_t len, void (*write_to_file)(aw_data *));

#endif /* aw_convert_mp4_to_flv_h */
