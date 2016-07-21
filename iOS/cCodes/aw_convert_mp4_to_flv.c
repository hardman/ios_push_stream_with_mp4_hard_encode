//
//  aw_convert_mp4_to_flv.c
//  pushStreamInC
//
//  Created by kaso on 19/7/16.
//  Copyright © 2016年 xcyo. All rights reserved.
//

#include "aw_convert_mp4_to_flv.h"
#include "aw_utils.h"

#include "aw_rtmp.h"

static aw_flv_script_tag *aw_create_flv_script_tag(aw_parsed_mp4 *parsed_mp4){
    aw_flv_script_tag *script_tag = alloc_aw_flv_script_tag();
    script_tag->duration = parsed_mp4->duration;
    script_tag->width = parsed_mp4->video_width;
    script_tag->height = parsed_mp4->video_height;
    script_tag->video_data_rate = parsed_mp4->video_data_rate;
    script_tag->frame_rate = parsed_mp4->video_frame_rate;
    script_tag->v_frame_rate = parsed_mp4->video_frame_rate;
    script_tag->a_frame_rate = parsed_mp4->audio_frame_rate;
    script_tag->a_sample_rate = parsed_mp4->audio_sample_rate;
    script_tag->a_sample_size = parsed_mp4->audio_sample_size;
    return script_tag;
}

static aw_flv_video_tag *aw_create_flv_video_config_record_tag(aw_parsed_mp4 *parsed_mp4){
    aw_flv_video_tag *video_tag = alloc_aw_flv_video_tag();
    video_tag->frame_type = aw_flv_v_frame_type_key;
    video_tag->codec_id = aw_flv_v_codec_id_H264;
    if (video_tag->codec_id == aw_flv_v_codec_id_H264) {
        video_tag->common_tag.header_size = 5;
    }else{
        video_tag->common_tag.header_size = 1;
    }
    video_tag->h264_package_type = aw_flv_v_h264_packet_type_seq_header;
    video_tag->h264_composition_time = 0;
    video_tag->config_record_data = copy_aw_data(parsed_mp4->video_config_record);
    video_tag->common_tag.timestamp = 0;
    video_tag->common_tag.data_size = parsed_mp4->video_config_record->size + 11 + video_tag->common_tag.header_size;
    return video_tag;
}

static aw_flv_audio_tag *aw_create_flv_audio_config_record_tag(aw_parsed_mp4 *parsed_mp4){
    aw_flv_audio_tag *audio_tag = alloc_aw_flv_audio_tag();
    audio_tag->sound_format = aw_flv_a_codec_id_AAC;
    if (audio_tag->sound_format == aw_flv_a_codec_id_AAC) {
        audio_tag->common_tag.header_size = 2;
    }else{
        audio_tag->common_tag.header_size = 1;
    }
    audio_tag->sound_rate = aw_flv_a_sound_rate_44kHZ;
    audio_tag->sound_size = aw_flv_a_sound_size_16_bit;
    audio_tag->sound_type = parsed_mp4->audio_is_stereo ? aw_flv_a_sound_type_stereo : aw_flv_a_sound_type_mono;
    audio_tag->aac_packet_type = aw_flv_a_aac_packge_type_aac_sequence_header;
    
    audio_tag->config_record_data = copy_aw_data(parsed_mp4->audio_config_record);
    audio_tag->common_tag.timestamp = 0;
    audio_tag->common_tag.data_size = parsed_mp4->audio_config_record->size + 11 + audio_tag->common_tag.header_size;
    return audio_tag;
}

static aw_flv_video_tag *aw_create_flv_video_tag(aw_parsed_mp4 *parsed_mp4, aw_mp4_av_sample *av_sample){
    aw_flv_video_tag *video_tag = alloc_aw_flv_video_tag();
    video_tag->frame_type = av_sample->is_key_frame ? aw_flv_v_frame_type_key: aw_flv_v_frame_type_inner;
    video_tag->codec_id = aw_flv_v_codec_id_H264;
    if (video_tag->codec_id == aw_flv_v_codec_id_H264) {
        video_tag->common_tag.header_size = 5;
    }else{
        video_tag->common_tag.header_size = 1;
    }
    video_tag->h264_package_type = aw_flv_v_h264_packet_type_nalu;
    if (av_sample->is_valid_composite_time) {
        video_tag->h264_composition_time = (uint32_t)(av_sample->composite_time * 1000);
    }else{
        video_tag->h264_composition_time = 0;
    }
    video_tag->common_tag.timestamp = (uint32_t)(av_sample->dts * 1000);
    video_tag->frame_data = alloc_aw_data(av_sample->sample_size);
    memcpy_aw_data(&video_tag->frame_data, parsed_mp4->mp4_file_data->data + av_sample->data_start_in_file, av_sample->sample_size);
    video_tag->common_tag.data_size = av_sample->sample_size + 11 + video_tag->common_tag.header_size;
    return video_tag;
}

static aw_flv_audio_tag *aw_create_flv_audio_tag(aw_parsed_mp4 *parsed_mp4, aw_mp4_av_sample *av_sample){
    aw_flv_audio_tag *audio_tag = alloc_aw_flv_audio_tag();
    audio_tag->sound_format = aw_flv_a_codec_id_AAC;
    if (audio_tag->sound_format == aw_flv_a_codec_id_AAC) {
        audio_tag->common_tag.header_size = 2;
    }else{
        audio_tag->common_tag.header_size = 1;
    }
    audio_tag->sound_rate = aw_flv_a_sound_rate_44kHZ;
    audio_tag->sound_size = aw_flv_a_sound_size_16_bit;
    audio_tag->sound_type = parsed_mp4->audio_is_stereo ? aw_flv_a_sound_type_stereo : aw_flv_a_sound_type_mono;
    audio_tag->aac_packet_type = aw_flv_a_aac_packge_type_aac_raw;
    
    audio_tag->common_tag.timestamp = (uint32_t)(av_sample->dts * 1000);
    audio_tag->frame_data = alloc_aw_data(av_sample->sample_size);
    memcpy_aw_data(&audio_tag->frame_data, parsed_mp4->mp4_file_data->data + av_sample->data_start_in_file, av_sample->sample_size);
    audio_tag->common_tag.data_size = av_sample->sample_size + 11 + audio_tag->common_tag.header_size;
    return audio_tag;
}

static void aw_write_flv_tag_for_convert_mp4_to_flv(aw_data **flv_data, aw_flv_common_tag *common_tag){
    aw_write_flv_tag(flv_data, common_tag);
    switch (common_tag->tag_type) {
        case aw_flv_tag_type_audio: {
            free_aw_flv_audio_tag(&common_tag->audio_tag);
            break;
        }
        case aw_flv_tag_type_video: {
            free_aw_flv_video_tag(&common_tag->video_tag);
            break;
        }
        case aw_flv_tag_type_script: {
            free_aw_flv_script_tag(&common_tag->script_tag);
            break;
        }
    }
}

extern void aw_convert_parsed_mp4_to_flv_data(aw_parsed_mp4 *parsed_mp4, aw_data ** flv_data){
    if (!parsed_mp4) {
        AWLog("[error] parsed_mp4 is NULL");
        return;
    }
    
    //初始化flv_data
    aw_data *inner_flv_data = *flv_data;
    if (!inner_flv_data) {
        inner_flv_data = alloc_aw_data(0);
        *flv_data = inner_flv_data;
    }
    
    //写入 flv header
    aw_write_flv_header(flv_data);
    //写入 flv meta data
    aw_write_flv_tag_for_convert_mp4_to_flv(flv_data, &aw_create_flv_script_tag(parsed_mp4)->common_tag);
    
    //写入 video config tag
    aw_write_flv_tag_for_convert_mp4_to_flv(flv_data, &aw_create_flv_video_config_record_tag(parsed_mp4)->common_tag);
    //写入 audio config tag
    aw_write_flv_tag_for_convert_mp4_to_flv(flv_data, &aw_create_flv_audio_config_record_tag(parsed_mp4)->common_tag);
    
    //写入 samples
    int i = 0;
    for (; i < parsed_mp4->frames->count; i++) {
        aw_mp4_av_sample *av_sample = aw_array_element_at_index(parsed_mp4->frames, i)->pointer_value;
        if (av_sample->is_video) {
            //写入video tag
            aw_write_flv_tag_for_convert_mp4_to_flv(flv_data, &aw_create_flv_video_tag(parsed_mp4, av_sample)->common_tag);
        }else{
            //写入audio tag
            aw_write_flv_tag_for_convert_mp4_to_flv(flv_data, &aw_create_flv_audio_tag(parsed_mp4, av_sample)->common_tag);
        }
    }
}

// ----------- rtmp -----------
static aw_rtmp_context *static_rtmp_context_for_parsed_mp4 = NULL;
static aw_data *static_rtmp_buff_data = NULL;

//打开rtmp context
extern int8_t aw_open_rtmp_context_for_parsed_mp4(const char *rtmp_url, aw_rtmp_state_changed_cb state_changed_cb){
    aw_uninit_debug_alloc();
    aw_init_debug_alloc();
    if(!static_rtmp_buff_data){
        static_rtmp_buff_data = alloc_aw_data(0);
    }
    if (!static_rtmp_context_for_parsed_mp4) {
        static_rtmp_context_for_parsed_mp4 = alloc_aw_rtmp_context(rtmp_url, state_changed_cb);
        if (static_rtmp_context_for_parsed_mp4) {
            return aw_rtmp_open(static_rtmp_context_for_parsed_mp4);
        }
    }else{
        return aw_rtmp_open(static_rtmp_context_for_parsed_mp4);
    }
    return 1;
}

//关闭rtmp context
extern void aw_close_rtmp_context_for_parsed_mp4(){
    if (static_rtmp_context_for_parsed_mp4) {
        free_aw_rtmp_context(&static_rtmp_context_for_parsed_mp4);
    }
    
    if(static_rtmp_buff_data){
        free_aw_data(&static_rtmp_buff_data);
    }
    
    aw_print_alloc_description();
}

//发送数据
static int aw_convert_mp4_to_flv_stream_send_data(aw_data *data){
    if (!aw_is_rtmp_opened(static_rtmp_context_for_parsed_mp4)) {
        return 0;
    }
    
    int write_ret = aw_rtmp_write(static_rtmp_context_for_parsed_mp4, (const char *)data->data, data->size);
    
    //    AWLog("aw_convert_mp4_to_flv_stream_send_data rtmp_write sendbytes=%u ret = %d", data->size, write_ret);
    
    //发送失败，不丢帧，去掉if就是不管成功失败都丢掉。
    if (write_ret) {
        reset_aw_data(&data);
    }
    return write_ret;
}

//发送当前数据
static int aw_convert_mp4_to_flv_stream_send_curr_data(){
    return aw_convert_mp4_to_flv_stream_send_data(static_rtmp_buff_data);
}

//发送flv header
static void aw_convert_mp4_to_flv_stream_send_header(aw_parsed_mp4 *parsed_mp4){
    if (static_rtmp_context_for_parsed_mp4->is_header_sent) {
        return;
    }
    
    //写入 flv meta data
    aw_write_flv_tag_for_convert_mp4_to_flv(&static_rtmp_buff_data, &aw_create_flv_script_tag(parsed_mp4)->common_tag);
    if(aw_convert_mp4_to_flv_stream_send_curr_data()){
        
        //写入 video config tag
        aw_write_flv_tag_for_convert_mp4_to_flv(&static_rtmp_buff_data, &aw_create_flv_video_config_record_tag(parsed_mp4)->common_tag);
        if(aw_convert_mp4_to_flv_stream_send_curr_data()){
            
            //写入 audio config tag
            aw_write_flv_tag_for_convert_mp4_to_flv(&static_rtmp_buff_data, &aw_create_flv_audio_config_record_tag(parsed_mp4)->common_tag);
            if(aw_convert_mp4_to_flv_stream_send_curr_data()){
                static_rtmp_context_for_parsed_mp4->is_header_sent = 1;
            }
        }
    }
    
    reset_aw_data(&static_rtmp_buff_data);
}

//发送 flv tags
static void aw_convert_parsed_mp4_to_flv_stream_send_body(aw_parsed_mp4 *parsed_mp4){
    if (parsed_mp4->frames->count <= 0) {
        return;
    }
    
    static_rtmp_context_for_parsed_mp4->current_time_stamp = static_rtmp_context_for_parsed_mp4->last_time_stamp;
    
    int i = 0;
    for (; i < parsed_mp4->frames->count; i++) {
        aw_mp4_av_sample *av_sample = aw_array_element_at_index(parsed_mp4->frames, i)->pointer_value;
        av_sample->dts += static_rtmp_context_for_parsed_mp4->current_time_stamp;
        //转换数据
        if (av_sample->is_video) {
            //写入video tag
            aw_write_flv_tag_for_convert_mp4_to_flv(&static_rtmp_buff_data, &aw_create_flv_video_tag(parsed_mp4, av_sample)->common_tag);
        }else{
            //写入audio tag
            aw_write_flv_tag_for_convert_mp4_to_flv(&static_rtmp_buff_data, &aw_create_flv_audio_tag(parsed_mp4, av_sample)->common_tag);
        }
        
        static_rtmp_context_for_parsed_mp4->last_time_stamp = av_sample->dts;
        
        AWLog("--------send flv tag dts=%f, cts=%f", av_sample->dts, av_sample->composite_time);
        
        //发送数据
        aw_convert_mp4_to_flv_stream_send_curr_data();
    }
}

//连接成功后，开始发送数据
static void aw_convert_parsed_mp4_to_flv_stream_start(aw_parsed_mp4 *parsed_mp4){
    if (!static_rtmp_context_for_parsed_mp4 || !static_rtmp_buff_data) {
        AWLog("[error] aw rtmp url is not open when call stream start");
        return;
    }
    if (!static_rtmp_context_for_parsed_mp4->is_header_sent) {
        aw_convert_mp4_to_flv_stream_send_header(parsed_mp4);
    }
    
    if (static_rtmp_context_for_parsed_mp4->is_header_sent) {
        aw_convert_parsed_mp4_to_flv_stream_send_body(parsed_mp4);
    }
    
    free_aw_parsed_mp4(&parsed_mp4);
}

//对外接口
extern void aw_convert_parsed_mp4_to_flv_stream(const uint8_t *mp4_file_data, size_t len){
    if (!mp4_file_data || len <= 0) {
        AWLog("[error] aw_convert_parsed_mp4_to_flv_stream mp4_file_data is null");
        return;
    }
    //解析
    aw_parsed_mp4 *parsed_mp4 = aw_parse_mp4_file_data(mp4_file_data, len);
    if (!parsed_mp4) {
        AWLog("[error] aw_convert_parsed_mp4_to_flv_stream parsed_mp4 is NULL");
        return;
    }
    
    //开始发送数据
    aw_convert_parsed_mp4_to_flv_stream_start(parsed_mp4);
}

extern void aw_convert_mp4_to_flv_test(const uint8_t *mp4_file_data, size_t len, void (*write_to_file)(aw_data *)){
    aw_uninit_debug_alloc();
    aw_init_debug_alloc();
    
    aw_parsed_mp4 *parsed_mp4 = aw_parse_mp4_file_data(mp4_file_data, len);
    
    aw_data *flv_data = NULL;
    aw_convert_parsed_mp4_to_flv_data(parsed_mp4, &flv_data);
    
    write_to_file(flv_data);
    
    free_aw_parsed_mp4(&parsed_mp4);
    free_aw_data(&flv_data);
    
    aw_print_alloc_description();
    
    AWLog("test convert_mp4_finished");
}