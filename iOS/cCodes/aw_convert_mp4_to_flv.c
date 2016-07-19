//
//  aw_convert_mp4_to_flv.c
//  pushStreamInC
//
//  Created by kaso on 19/7/16.
//  Copyright © 2016年 xcyo. All rights reserved.
//

#include "aw_convert_mp4_to_flv.h"
#include "aw_utils.h"

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
    video_tag->h264_composition_time = av_sample->composite_time;
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