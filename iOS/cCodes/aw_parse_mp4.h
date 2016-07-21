//
//  aw_parse_mp4.h
//  pushStreamInC
//
//  Created by kaso on 15/7/16.
//  Copyright © 2016年 xcyo. All rights reserved.
//

#ifndef aw_parse_mp4_h
#define aw_parse_mp4_h

#include <stdio.h>
#include "aw_dict.h"
#include "aw_alloc.h"
#include "aw_data.h"
#include "aw_array.h"

//mp4视频/音频帧
typedef struct aw_mp4_av_sample{
    //帧位于文件的哪个位置
    uint32_t data_start_in_file;
    //数据长度
    uint32_t sample_size;
    //是否关键帧
    uint8_t is_key_frame;
    //时间戳
    double dts;
    //即h264的cts pts = dts + cts
    double composite_time;
    //是否有composite_time
    int8_t is_valid_composite_time;
    //视频帧还是音频
    uint8_t is_video;
} aw_mp4_av_sample;

extern aw_mp4_av_sample *alloc_aw_mp4_av_sample();

extern void free_aw_mp4_av_sample(aw_mp4_av_sample **av_sample);

typedef struct aw_parsed_mp4 {
    //时长
    double duration;
    
    //-------视频参数--------
    //视频码率
    double video_data_rate;
    //视频帧率：一帧多长时间
    double video_frame_rate;
    //视频宽高
    double video_width;
    double video_height;
    //视频时间标尺，采样数
    double video_time_scale;
    
    //视频config record for h264
    aw_data *video_config_record;
    
    double video_start_dts;
    
    //-------音频参数--------
    //音频帧率：一帧多长时间
    double audio_frame_rate;
    //音频样本大小
    double audio_sample_size;
    //是否立体声
    int8_t audio_is_stereo;
    //音频采样率，同video的time_scale duration /time_scale = 时间(s)
    int32_t audio_sample_rate;
    //音频config record for h264
    aw_data *audio_config_record;
    
    aw_data *mp4_file_data;
    
    double audio_start_dts;
    
    //音视频帧数据
    aw_array *frames;
} aw_parsed_mp4;

extern aw_parsed_mp4 *alloc_aw_parsed_mp4();
extern void free_aw_parsed_mp4(aw_parsed_mp4 **parsed_mp4);

//解析函数
extern aw_parsed_mp4 *aw_parse_mp4_file_data(const uint8_t *mp4_file_data, size_t len);

//测试
extern void aw_parse_mp4_test(const uint8_t *mp4_file_data, size_t len);

#endif /* aw_parse_mp4_h */
