//
//  aw_parse_mp4.c
//  pushStreamInC
//
//  Created by kaso on 15/7/16.
//  Copyright © 2016年 xcyo. All rights reserved.
//

#include "aw_parse_mp4.h"
#include "aw_data.h"
#include "aw_dict.h"
#include "aw_mp4box.h"
#include "aw_utils.h"

static aw_parsed_mp4 *aw_parse_mp4_boxes(aw_dict *mp4_box_dict, aw_data *mp4_file_data);

extern aw_mp4_av_sample *alloc_aw_mp4_av_sample(){
    aw_mp4_av_sample *av_sample = aw_alloc(sizeof(aw_mp4_av_sample));
    memset(av_sample, 0, sizeof(aw_mp4_av_sample));
    return av_sample;
}

extern void free_aw_mp4_av_sample(aw_mp4_av_sample **av_sample){
    aw_free(*av_sample);
    *av_sample = NULL;
}

extern aw_parsed_mp4 *alloc_aw_parsed_mp4(){
    aw_parsed_mp4 *parsed_mp4 = aw_alloc(sizeof(aw_parsed_mp4));
    memset(parsed_mp4, 0, sizeof(aw_parsed_mp4));
    parsed_mp4->frames = alloc_aw_array(0);
    return parsed_mp4;
}

extern void free_aw_parsed_mp4(aw_parsed_mp4 **parsed_mp4){
    aw_parsed_mp4 *inner_parsed_mp4 = *parsed_mp4;
    if (inner_parsed_mp4->video_config_record) {
        free_aw_data(&inner_parsed_mp4->video_config_record);
    }
    if (inner_parsed_mp4->audio_config_record) {
        free_aw_data(&inner_parsed_mp4->audio_config_record);
    }
    if (inner_parsed_mp4->frames) {
        free_aw_array(&inner_parsed_mp4->frames);
    }
    if (inner_parsed_mp4->mp4_file_data) {
        free_aw_data(&inner_parsed_mp4->mp4_file_data);
    }
    aw_free(*parsed_mp4);
    *parsed_mp4 = NULL;
}

static void aw_free_mp4_box_for_dict(aw_mp4_box *box, int extra){
    free_aw_mp4_box(&box);
}

static void aw_free_mp4_av_sample_for_parsed_mp4(aw_mp4_av_sample *av_sample, int extra){
    free_aw_mp4_av_sample(&av_sample);
}

extern aw_parsed_mp4 *aw_parse_mp4_file_data(const uint8_t *mp4_file_data, size_t len){
    aw_data *mp4_data = alloc_aw_data((int32_t)len);
    memcpy_aw_data(&mp4_data, mp4_file_data, (int32_t)len);
    
    aw_dict *mp4box_dict = alloc_aw_dict();
    
    data_reader.start_read(mp4_data);
    while (data_reader.remain_count(mp4_data)) {
        aw_mp4_box *mp4_box = alloc_aw_mp4_box();
        aw_mp4box_parse_data(mp4_box, mp4_data);
        aw_dict_set_release_pointer(mp4box_dict, mp4_box->type_string, mp4_box, (void (*)(void *, int))aw_free_mp4_box_for_dict, 0, 1);
    }
    
    aw_parsed_mp4 *parsed_mp4 = aw_parse_mp4_boxes(mp4box_dict, mp4_data);
    
    free_aw_dict(&mp4box_dict);
    
    return parsed_mp4;
}

static void aw_parse_stbl_box(aw_mp4_box *stbl_box, aw_parsed_mp4 *parsed_mp4,int8_t is_video){
    //stts和ctts决定了每个sample的解析和播放时间戳
    //stts是具有相同时间间隔的sample数量。
    //duration = sample_count1*duration1 + ... + sample_countN * durationN
    //通过stts和ctts能够计算每个sample的dts和pts
    aw_array *sample_duration_array = alloc_aw_array(0);
    aw_mp4_box *stts_box = aw_mp4box_find_box(stbl_box, "stts");
    int sample_duration_accum = 0;
    aw_dict **stts_box_entries = aw_dict_get_release_pointer(stts_box->parsed_data, "entries");
    int i = 0;
    int sample_duration_count = aw_dict_get_int(stts_box->parsed_data, "time_to_sample_count");
    for (; i < sample_duration_count; i++) {
        aw_dict *time_to_sample = stts_box_entries[i];
        int sample_count = aw_dict_get_int(time_to_sample, "sample_count");
        int sample_duration = aw_dict_get_int(time_to_sample, "sample_duration");
        int j = 0;
        for (; j < sample_count; j++) {
            aw_array_add_int(&sample_duration_array, sample_duration_accum);
            sample_duration_accum += sample_duration;
        }
    }
    aw_array_add_int(&sample_duration_array, sample_duration_accum);
    
    //获取ctts，修正stts时间
    aw_array *video_sample_duration_offset_array = NULL;
    if (is_video) {
        aw_mp4_box * ctts_box = aw_mp4box_find_box(stbl_box, "ctts");
        if (ctts_box) {
            video_sample_duration_offset_array =
            alloc_aw_array(0);
            int ctts_entries_count = aw_dict_get_int(ctts_box->parsed_data, "sample_count");
            aw_dict **ctts_box_entries = aw_dict_get_release_pointer(ctts_box->parsed_data, "entries");
            i = 0;
            for (; i < ctts_entries_count; i++) {
                aw_dict *entry = ctts_box_entries[i];
                int sample_index = aw_dict_get_int(entry, "sample_count");
                int sample_offset = aw_dict_get_int(entry, "sample_offset");
                int j = 0;
                for (; j < sample_index; j++) {
                    aw_array_add_int(&video_sample_duration_offset_array, sample_offset);
                }
            }
        }
    }
    
    //stsc描述了chunk信息，包含几个chunk，每个chunk中有几个samples，值得注意的是，chunk以1开头。chunk其实是连续的多个samples，相当于语言中的数组。没有别的复杂含义。而chunk的firstchunk可能会跳跃，所以，需要特别处理。
    aw_mp4_box *stsc_box = aw_mp4box_find_box(stbl_box, "stsc");
    //stco纪录了每个chunk在文件中的位置。
    aw_mp4_box *stco_box = aw_mp4box_find_box(stbl_box, "stco");
    //stsz纪录了每个sample的size。通过结合上述3个box，能够精确获得每个sample的位置和大小。
    aw_mp4_box *stsz_box = aw_mp4box_find_box(stbl_box, "stsz");
    //stss纪录了关键帧，它纪录的是sample的下标
    aw_mp4_box *stss_box = aw_mp4box_find_box(stbl_box, "stss");
    
    int32_t all_chunks_dict_count = aw_dict_get_int(stsc_box->parsed_data, "entry_count");
    aw_dict **all_chunks_dict = aw_dict_get_release_pointer(stsc_box->parsed_data, "entries");
    
    //    int32_t all_chunks_pos_dict_count = aw_dict_get_int(stco_box->parsed_data, "chunk_offset_count");
    aw_dict **all_chunks_pos_dict = aw_dict_get_release_pointer(stco_box->parsed_data, "entries");
    
    //    int32_t all_sample_size_dict_count = aw_dict_get_int(stsz_box->parsed_data, "sample_size_count");
    aw_dict **all_sample_size_dict = aw_dict_get_release_pointer(stsz_box->parsed_data, "entries");
    
    int32_t all_key_frame_indexes_dict_count = 0;
    aw_dict **all_key_frame_indexes_dict = NULL;
    if (stss_box) {
        all_key_frame_indexes_dict_count = aw_dict_get_int(stss_box->parsed_data, "sync_sample_count");
        all_key_frame_indexes_dict = aw_dict_get_release_pointer(stss_box->parsed_data, "entries");
    }
    
    int32_t sample_index = 0;
    double last_video_dts = -1;
    double last_audio_dts = -1;
    i = 0;
    for (; i < all_chunks_dict_count; i++) {
        aw_dict *one_chunk_dict = all_chunks_dict[i];
        int32_t first_chunk = aw_dict_get_int(one_chunk_dict, "first_chunk");
        int32_t next_chunk = -1;
        if (i != all_chunks_dict_count - 1) {
            aw_dict *next_chunk_dict = all_chunks_dict[i + 1];
            next_chunk = aw_dict_get_int(next_chunk_dict, "first_chunk");
        }else{
            next_chunk = all_chunks_dict_count - 1;
        }
        
        int chunk_index = first_chunk - 1;
        for (; chunk_index < next_chunk - 1 ; chunk_index++) {
            int32_t samples_in_chunk = aw_dict_get_int(one_chunk_dict, "samples_one_chunk");
            int32_t chunk_pos = aw_dict_get_int(all_chunks_pos_dict[chunk_index], "offset");
            int32_t next_sample_pos = 0;
            int j = 0;
            for (; j < samples_in_chunk; j++, sample_index++) {
                int32_t curr_sample_size = aw_dict_get_int(all_sample_size_dict[sample_index], "one_sample_size");
                int32_t curr_sample_pos = chunk_pos + next_sample_pos;
                
                next_sample_pos += curr_sample_size;
                
                uint8_t is_key_frame = 0;
                int k = 0;
                for (; k < all_key_frame_indexes_dict_count; k++) {
                    aw_dict *one_index_dict = all_key_frame_indexes_dict[k];
                    int key_frame_index = aw_dict_get_int(one_index_dict, "sample_index");
                    if (key_frame_index == sample_index + 1) {
                        is_key_frame = 1;
                        break;
                    }
                }
                
                double dts = (double)aw_array_element_at_index(sample_duration_array, sample_index)->int_value;
                if (is_video) {
                    dts /= parsed_mp4->video_time_scale;
                    if (!parsed_mp4->video_frame_rate) {
                        if (last_video_dts == -1) {
                            last_video_dts = dts;
                        }else{
                            parsed_mp4->video_frame_rate = dts - last_video_dts;
                        }
                    }
                }else{
                    dts /= parsed_mp4->audio_sample_rate;
                    if (parsed_mp4->audio_frame_rate == 0) {
                        if (last_audio_dts == -1) {
                            last_audio_dts = dts;
                        }else{
                            parsed_mp4->audio_frame_rate = dts - last_audio_dts;
                        }
                    }
                }
                
                double composite_time = -1;
                if (is_video && video_sample_duration_offset_array &&video_sample_duration_offset_array->count > 0) {
                    composite_time = (double)aw_array_element_at_index(video_sample_duration_offset_array, sample_index)->int_value;
                    composite_time /= parsed_mp4->video_time_scale;
                }
                
                aw_mp4_av_sample *mp4_av_sample = alloc_aw_mp4_av_sample();
                mp4_av_sample->data_start_in_file = curr_sample_pos;
                mp4_av_sample->sample_size = curr_sample_size;
                mp4_av_sample->is_key_frame = is_key_frame;
                mp4_av_sample->dts = dts + (is_video ? parsed_mp4->video_start_dts : parsed_mp4->audio_start_dts);
                if (composite_time >= 0) {
                    mp4_av_sample->is_valid_composite_time = 1;
                    mp4_av_sample->composite_time = composite_time;
                }else{
                    mp4_av_sample->is_valid_composite_time = 0;
                    mp4_av_sample->composite_time = 0;
                }
                mp4_av_sample->is_video = is_video;
                aw_array_add_release_pointer(&parsed_mp4->frames, mp4_av_sample, (void (*)(void *, int))aw_free_mp4_av_sample_for_parsed_mp4, 0);
            }
        }
    }
    
    if(video_sample_duration_offset_array){
        free_aw_array(&video_sample_duration_offset_array);
    }
    
    if (sample_duration_array) {
        free_aw_array(&sample_duration_array);
    }
}

static void aw_parse_trak_box(aw_mp4_box *trak_box, aw_parsed_mp4 *parsed_mp4){
    aw_mp4_box *mdia_box = aw_mp4box_find_box(trak_box, "mdia");
    aw_mp4_box *hdlr_box = aw_mp4box_find_box(mdia_box, "hdlr");
    aw_mp4_box *stbl_box = aw_mp4box_find_box(mdia_box, "minf.stbl");
    aw_mp4_box *mdhd_box = aw_mp4box_find_box(mdia_box, "mdhd");
    aw_mp4_box *stsd_box = aw_mp4box_find_box(stbl_box, "stsd");
    aw_mp4_box *elst_box = aw_mp4box_find_box(trak_box, "edts.elst");
    
    int32_t start_offset = 0;
    int32_t empty_duration = 0;
    if (elst_box) {
        int edit_start_index = 0;
        int32_t start_time = 0;
        int edit_list_count = aw_dict_get_int(elst_box->parsed_data, "edit_list_count");
        aw_dict **entries = aw_dict_get_pointer(elst_box->parsed_data, "entries");
        int i = 0;
        for (; i < edit_list_count; i++) {
            aw_dict *entry = entries[i];
            int32_t duration = aw_dict_get_int(entry, "duration");
            int32_t time = aw_dict_get_int(entry, "start_time");
            if (i == 0 && time == -1) {
                empty_duration = duration;
                edit_start_index = 1;
            }else if(i == edit_start_index && time >= 0){
                start_time = time;
            }
        }
        start_offset = start_time;
    }
    
    const char *handle_type = aw_dict_get_str(hdlr_box->parsed_data, "handle_type");
    if (!strcmp(handle_type, "vide")) {
        aw_mp4_box *tkhd_box = aw_mp4box_find_box(trak_box, "tkhd");
        parsed_mp4->video_width = aw_dict_get_double(tkhd_box->parsed_data, "width");
        parsed_mp4->video_height = aw_dict_get_double(tkhd_box->parsed_data, "height");
        parsed_mp4->video_time_scale = aw_dict_get_int(mdhd_box->parsed_data, "time_scale");
        
        aw_mp4_box *avcc_box = aw_mp4box_find_box(stsd_box, "avc1.avcC");
        parsed_mp4->video_config_record = copy_aw_data(aw_dict_get_pointer(avcc_box->parsed_data, "avcc_data"));
        
        parsed_mp4->video_start_dts = -(start_offset + empty_duration / 90000 * parsed_mp4->video_time_scale) / parsed_mp4->video_time_scale;
        
        aw_parse_stbl_box(stbl_box, parsed_mp4, 1);
    }else if(!strcmp(handle_type, "soun")){
        aw_mp4_box *mp4a_box = aw_mp4box_find_box(stsd_box, "mp4a");
        parsed_mp4->audio_sample_size = aw_dict_get_int(mp4a_box->parsed_data, "sample_size");
        parsed_mp4->audio_is_stereo = aw_dict_get_int(mp4a_box->parsed_data, "channel_count") > 1;
        parsed_mp4->audio_sample_rate = aw_dict_get_int(mdhd_box->parsed_data, "time_scale");
        aw_mp4_box *esds_box = aw_mp4box_find_box(mp4a_box, "esds");
        if (!esds_box) {
            esds_box = aw_mp4box_find_box(mp4a_box, "wave.esds");
        }
        
        if (esds_box) {
            parsed_mp4->audio_config_record = copy_aw_data(aw_dict_get_pointer(esds_box->parsed_data, "decoder_config"));
        }
        
        parsed_mp4->audio_start_dts = -(start_offset + empty_duration / 90000 * parsed_mp4->audio_sample_rate) / parsed_mp4->audio_sample_rate;
        
        aw_parse_stbl_box(stbl_box, parsed_mp4, 0);
    }else{
        return;
    }
}

static aw_array_sort_compare_result aw_parse_mp4_sort_frames_compare_func(aw_array_element *ele1, aw_array_element *ele2){
    aw_mp4_av_sample *av_sample1 = ele1->pointer_value;
    aw_mp4_av_sample *av_sample2 = ele2->pointer_value;
    if (av_sample1->dts > av_sample2->dts) {
        return aw_array_sort_compare_result_great;
    }else if(av_sample1->dts < av_sample2->dts){
        return aw_array_sort_compare_result_less;
    }else{
        if (!av_sample1->is_video && av_sample2->is_video) {
            return aw_array_sort_compare_result_great;
        }else{
            if (av_sample1->data_start_in_file > av_sample2->data_start_in_file) {
                return aw_array_sort_compare_result_great;
            }else if(av_sample1->data_start_in_file < av_sample2->data_start_in_file){
                return aw_array_sort_compare_result_less;
            }else{
                return aw_array_sort_compare_result_equal;
            }
        }
    }
}

static void aw_parse_mp4_sort_frames(aw_parsed_mp4 *parsed_mp4, aw_array_sort_func sort_func){
    sort_func(parsed_mp4->frames, aw_array_sort_policy_ascending, aw_parse_mp4_sort_frames_compare_func);
}

static aw_parsed_mp4 *aw_parse_mp4_boxes(aw_dict *mp4box_dict, aw_data *mp4_file_data){
    aw_parsed_mp4 *parsed_mp4 = alloc_aw_parsed_mp4();
    parsed_mp4->mp4_file_data = mp4_file_data;
    
    aw_mp4_box *moov_box = aw_dict_get_pointer(mp4box_dict, "moov");
    
    //视频长度
    aw_mp4_box *mvhd_box = aw_mp4box_find_box(moov_box, "mvhd");
    parsed_mp4->duration = (double)aw_dict_get_int(mvhd_box->parsed_data, "duration") / aw_dict_get_int(mvhd_box->parsed_data, "time_scale");
    
    parsed_mp4->video_data_rate = mp4_file_data->size / parsed_mp4->duration * 8;
    
    int i = 0;
    while (1) {
        char trak_name[32];
        memset(trak_name, 0, 32);
        sprintf(trak_name, "trak@%d", i);
        aw_mp4_box *trak_box = aw_mp4box_find_box(moov_box, trak_name);
        if (trak_box) {
            aw_parse_trak_box(trak_box, parsed_mp4);
            i++;
        }else{
            break;
        }
    }
    
    //给parsed_mp4->frames 排序
    
    aw_parse_mp4_sort_frames(parsed_mp4, aw_array_sort_quick);
    
    return parsed_mp4;
}

//测试
extern void aw_parse_mp4_test(const uint8_t *mp4_file_data, size_t len){
    aw_uninit_debug_alloc();
    aw_init_debug_alloc();
    
    aw_parsed_mp4 *parsed_mp4 = aw_parse_mp4_file_data(mp4_file_data, len);
    
    free_aw_parsed_mp4(&parsed_mp4);
    
    aw_print_alloc_description();
}

