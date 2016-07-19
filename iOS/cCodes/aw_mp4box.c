//
//  aw_mp4box.c
//  pushStreamInC
//
//  Created by kaso on 12/7/16.
//  Copyright © 2016年 xcyo. All rights reserved.
//

#include "aw_mp4box.h"
#include "aw_utils.h"
#include <math.h>

#define BYTES_FM_ASCII(a, b, c, d) ((d) | ((c) << 8) | ((b) << 16) | ((unsigned)(a) << 24))

/**
 *  释放 mp4 box
 *
 *  @param box 要释放的mp4 box
 */
void free_aw_mp4_box(aw_mp4_box ** box){
    //todo free: parsed_data, type_string, data, children
    aw_mp4_box *mp4_box = *box;
    
    //    AWLog("free mp4 box type=%s", mp4_box->type_string);
    
    //data
    if (mp4_box->data) {
        free_aw_data(&mp4_box->data);
    }
    
    //children
    if (mp4_box->children) {
        free_aw_dict(&mp4_box->children);
    }
    
    //parsed_data
    if (mp4_box->parsed_data) {
        free_aw_dict(&mp4_box->parsed_data);
    }
    
    //type_string
    if (mp4_box->type_string) {
        aw_free(mp4_box->type_string);
    }
    
    //parent_box
    mp4_box->parent_box = NULL;
    
    //root_box
    mp4_box->root_box = NULL;
    
    aw_free(mp4_box);
    *box = NULL;
}

/**
 *  创建 mp4 box
 *
 *  @return 返回创建的box
 */
aw_mp4_box *alloc_aw_mp4_box(){
    aw_mp4_box *box = (aw_mp4_box *)aw_alloc(sizeof(aw_mp4_box));
    memset(box, 0, sizeof(aw_mp4_box));
    return box;
}

static void release_box_entries(aw_dict **entries, int size){
    int i = 0;
    for (; i < size; i++) {
        aw_dict *entry = entries[i];
        free_aw_dict(&entry);
    }
    aw_free(entries);
}

static void release_mp4_box_for_box_children(aw_mp4_box *box, int extra){
    free_aw_mp4_box(&box);
}

static void release_aw_data_for_box(aw_data *data, int extra){
    free_aw_data(&data);
}

static void reset_box_data_with_remain_data(aw_mp4_box *box){
    aw_data *box_data = box->data;
    aw_data *new_data = alloc_aw_data(0);
    memcpy_aw_data(&new_data, box_data->data + box_data->curr_pos, box_data->size - box_data->curr_pos);
    box->data = new_data;
    data_reader.start_read(new_data);
    free_aw_data(&box_data);
}

uint32_t aw_mp4_bytes_fm_ascii(uint8_t a, uint8_t b, uint8_t c, uint8_t d){
    return ((d) | ((c) << 8) | ((b) << 16) | ((unsigned)(a) << 24));
}

void aw_mp4_int32_to_ascii_str(uint32_t int32, char store[5]){
    memset(store, 0, 5);
    sprintf(store, "%c%c%c%c", int32>>24, (int32>>16) & 0xff, (int32>>8) &0xff, int32 & 0xff);
}

//创建子box
static aw_data *create_child_box(aw_mp4_box *root){
    while (data_reader.remain_count(root->data)) {
        aw_mp4_box *child_box = alloc_aw_mp4_box();
        child_box->parent_box = root;
        if (root->root_box) {
            child_box->root_box = root->root_box;
        }else{
            child_box->root_box = root;
        }
        aw_mp4box_parse_data(child_box, root->data);
        
        char child_type[5];
        aw_mp4_int32_to_ascii_str(child_box->type, child_type);
        
        if (!root->children) {
            root->children = alloc_aw_dict();
            root->children->repeat_seperate_word = '@';
        }
        
        aw_dict_set_release_pointer(root->children, child_type, child_box, (void (*)(void *, int))release_mp4_box_for_box_children, 0, 1);
    }
    return root->data;
}

aw_mp4_box *aw_mp4box_find_box(aw_mp4_box *box, const char *type_name){
    aw_mp4_box *find_box = box;
    char *p_point = (char *)type_name;
    while (1) {
        char *old_p_point = p_point;
        
        p_point = strchr(p_point, '.');
        
        char *pre_name = NULL;
        if (!p_point) {
            pre_name = old_p_point;
        }else{
            int pre_str_len = (int)(p_point - old_p_point + 1);
            pre_name = aw_alloc(pre_str_len);
            memset(pre_name, 0, pre_str_len);
            memcpy(pre_name, old_p_point, pre_str_len - 1);
        }
        
        if (find_box->children) {
            find_box = aw_dict_get_pointer(find_box->children, pre_name);
            if (find_box) {
                if (p_point) {
                    p_point++;
                }
            }
        }else{
            find_box = NULL;
        }
        
        if (p_point) {
            aw_free(pre_name);
        }
        
        if (!find_box || !p_point) {
            break;
        }
    }
    
    return find_box;
}

static void parse_box_mvhd(aw_mp4_box *box){
    uint8_t version = data_reader.read_uint8(box->data);
    uint32_t flags = data_reader.read_uint24(box->data);
    //createtime
    uint32_t create_time = data_reader.read_uint32(box->data);
    //modifytime
    uint32_t modify_time = data_reader.read_uint32(box->data);
    //timescale: 1秒钟多少个track
    uint32_t time_scale = data_reader.read_uint32(box->data);
    //duration：一共多少个track
    uint32_t duration = data_reader.read_uint32(box->data);
    //rate
    uint16_t rate1 = data_reader.read_uint16(box->data);
    uint16_t rate2 = data_reader.read_uint16(box->data);
    //volume
    uint16_t volume1 = data_reader.read_uint16(box->data);
    uint16_t volume2 = data_reader.read_uint16(box->data);
    
    aw_dict *parsed_dict= alloc_aw_dict();
    box->parsed_data = parsed_dict;
    aw_dict_set_int(parsed_dict, "version", version, 0);
    aw_dict_set_int(parsed_dict, "flags", flags, 0);
    aw_dict_set_int(parsed_dict, "create_time", create_time, 0);
    aw_dict_set_int(parsed_dict, "modify_time", modify_time, 0);
    aw_dict_set_int(parsed_dict, "time_scale", time_scale, 0);
    aw_dict_set_int(parsed_dict, "duration", duration, 0);
    char rate[32];
    sprintf(rate, "%u.%u", rate1, rate2);
    aw_dict_set_str(parsed_dict, "rate", rate, 0);
    char volume[32];
    sprintf(volume, "%u.%u", volume1, volume2);
    aw_dict_set_str(parsed_dict, "volume", volume, 0);
}

//ffmpeg: libavutil/display.c
#define CONV_FP(x) ((double) (x)) / (1 << 16)
static double get_display_rotation(const int32_t matrix[9])
{
    double rotation, scale[2];
    
    scale[0] = hypot(CONV_FP(matrix[0]), CONV_FP(matrix[3]));
    scale[1] = hypot(CONV_FP(matrix[1]), CONV_FP(matrix[4]));
    
    if (scale[0] == 0.0 || scale[1] == 0.0)
        return NAN;
    
    rotation = atan2(CONV_FP(matrix[1]) / scale[1],
                     CONV_FP(matrix[0]) / scale[0]) * 180 / M_PI;
    
    return rotation;
}

static void parse_box_tkhd(aw_mp4_box *box){
    uint8_t version = data_reader.read_uint8(box->data);
    uint32_t flags = data_reader.read_uint24(box->data);
    uint32_t create_time = data_reader.read_uint32(box->data);
    uint32_t modify_time = data_reader.read_uint32(box->data);
    uint32_t track_id = data_reader.read_uint32(box->data);
    //reserverd
    data_reader.skip_bytes(box->data, 4);
    uint32_t duration = data_reader.read_uint32(box->data);
    //reserverd
    data_reader.skip_bytes(box->data, 8);
    uint16_t layer = data_reader.read_uint16(box->data);
    uint16_t alertnate_group = data_reader.read_uint16(box->data);
    uint16_t volume = data_reader.read_uint16(box->data);
    data_reader.skip_bytes(box->data, 2);
    uint32_t matrix[3][3];
    for (int i = 0; i < 3; i++) {
        matrix[i][0] = data_reader.read_uint32(box->data);
        matrix[i][1] = data_reader.read_uint32(box->data);
        matrix[i][2] = data_reader.read_uint32(box->data);
    }
    
    double_t rotation = get_display_rotation((const int32_t *)matrix);
    
    //width
    uint16_t wid1 = data_reader.read_uint16(box->data);
    uint16_t wid2 = data_reader.read_uint16(box->data);
    //height
    uint16_t hei1 = data_reader.read_uint16(box->data);
    uint16_t hei2 = data_reader.read_uint16(box->data);
    
    aw_dict *parsed_dict= alloc_aw_dict();
    box->parsed_data = parsed_dict;
    aw_dict_set_int(parsed_dict, "version", version, 0);
    aw_dict_set_int(parsed_dict, "flags", flags, 0);
    aw_dict_set_int(parsed_dict, "create_time", create_time, 0);
    aw_dict_set_int(parsed_dict, "modify_time", modify_time, 0);
    aw_dict_set_int(parsed_dict, "track_id", track_id, 0);
    aw_dict_set_int(parsed_dict, "duration", duration, 0);
    aw_dict_set_int(parsed_dict, "layer", layer, 0);
    aw_dict_set_int(parsed_dict, "alertnate_group", alertnate_group, 0);
    aw_dict_set_int(parsed_dict, "volume", volume, 0);
    aw_dict_set_int(parsed_dict, "rotation", rotation, 0);
    char wid[32];
    sprintf(wid, "%u.%u", wid1, wid2);
    aw_dict_set_double(parsed_dict, "width", atof(wid), 0);
    char hei[32];
    sprintf(hei, "%u.%u", hei1, hei2);
    aw_dict_set_double(parsed_dict, "height", atof(hei), 0);
}

static void parse_box_elst(aw_mp4_box *box){
    uint8_t version = data_reader.read_uint8(box->data);
    uint32_t flags = data_reader.read_uint24(box->data);
    uint32_t edit_list_count = data_reader.read_uint32(box->data);
    
    aw_dict **entries = (aw_dict **) aw_alloc(sizeof(aw_dict *) * edit_list_count);
    int i = 0;
    for (; i < edit_list_count; i++) {
        aw_dict *one_dict = alloc_aw_dict();
        aw_dict_set_int(one_dict, "duration", data_reader.read_uint32(box->data), 0);
        aw_dict_set_int(one_dict, "start_time", data_reader.read_uint32(box->data), 0);
        aw_dict_set_int(one_dict, "speed", data_reader.read_uint32(box->data), 0);
        entries[i] = one_dict;
    }
    
    aw_dict *parsed_dict= alloc_aw_dict();
    box->parsed_data = parsed_dict;
    aw_dict_set_int(parsed_dict, "version", version, 0);
    aw_dict_set_int(parsed_dict, "flags", flags, 0);
    aw_dict_set_int(parsed_dict, "edit_list_count", edit_list_count, 0);
    aw_dict_set_release_pointer(parsed_dict, "entries", entries, (void (*)(void *, int extra))release_box_entries, edit_list_count, 0);
}

static void parse_box_mdhd(aw_mp4_box *box){
    uint8_t version = data_reader.read_uint8(box->data);
    uint32_t flags = data_reader.read_uint24(box->data);
    uint32_t create_time = data_reader.read_uint32(box->data);
    uint32_t modify_time = data_reader.read_uint32(box->data);
    uint32_t time_scale = data_reader.read_uint32(box->data);
    uint32_t duration = data_reader.read_uint32(box->data);
    uint16_t language = data_reader.read_uint16(box->data);
    data_reader.skip_bytes(box->data, 2);
    
    aw_dict *parsed_dict= alloc_aw_dict();
    box->parsed_data = parsed_dict;
    aw_dict_set_int(parsed_dict, "version", version, 0);
    aw_dict_set_int(parsed_dict, "flags", flags, 0);
    aw_dict_set_int(parsed_dict, "create_time", create_time, 0);
    aw_dict_set_int(parsed_dict, "modify_time", modify_time, 0);
    aw_dict_set_int(parsed_dict, "time_scale", time_scale, 0);
    aw_dict_set_int(parsed_dict, "duration", duration, 0);
    aw_dict_set_int(parsed_dict, "language", language, 0);
}

static void parse_box_hdlr(aw_mp4_box *box){
    uint8_t version = data_reader.read_uint8(box->data);
    uint32_t flags = data_reader.read_uint24(box->data);
    data_reader.skip_bytes(box->data, 4);
    uint32_t handle_type = data_reader.read_uint32(box->data);
    
    char handle_type_str[5] = {0};
    aw_mp4_int32_to_ascii_str(handle_type, handle_type_str);
    
    aw_dict *parsed_dict= alloc_aw_dict();
    box->parsed_data = parsed_dict;
    aw_dict_set_int(parsed_dict, "version", version, 0);
    aw_dict_set_int(parsed_dict, "flags", flags, 0);
    aw_dict_set_str(parsed_dict, "handle_type", handle_type_str, 0);
}

static void parse_box_stts(aw_mp4_box *box){
    uint8_t version = data_reader.read_uint8(box->data);
    uint32_t flags = data_reader.read_uint24(box->data);
    uint32_t time_to_sample_count = data_reader.read_uint32(box->data);
    
    aw_dict **entries = (aw_dict **) aw_alloc(sizeof(aw_dict *) * time_to_sample_count);
    int i = 0;
    for (; i < time_to_sample_count; i++) {
        aw_dict *one_dict = alloc_aw_dict();
        aw_dict_set_int(one_dict, "sample_count", data_reader.read_uint32(box->data), 0);
        aw_dict_set_int(one_dict, "sample_duration", data_reader.read_uint32(box->data), 0);
        entries[i] = one_dict;
    }
    
    aw_dict *parsed_dict= alloc_aw_dict();
    box->parsed_data = parsed_dict;
    aw_dict_set_int(parsed_dict, "version", version, 0);
    aw_dict_set_int(parsed_dict, "flags", flags, 0);
    aw_dict_set_int(parsed_dict, "time_to_sample_count", time_to_sample_count, 0);
    aw_dict_set_release_pointer(parsed_dict, "entries", entries, (void (*)(void *, int extra))release_box_entries, time_to_sample_count, 0);
}

static void parse_box_ctts(aw_mp4_box *box){
    uint8_t version = data_reader.read_uint8(box->data);
    uint32_t flags = data_reader.read_uint24(box->data);
    uint32_t sample_count = data_reader.read_uint32(box->data);
    
    aw_dict **entries = (aw_dict **) aw_alloc(sizeof(aw_dict *) * sample_count);
    int i = 0;
    for (; i < sample_count; i++) {
        aw_dict *one_dict = alloc_aw_dict();
        aw_dict_set_int(one_dict, "sample_count", data_reader.read_uint32(box->data), 0);
        aw_dict_set_int(one_dict, "sample_offset", data_reader.read_uint32(box->data), 0);
        entries[i] = one_dict;
    }
    
    aw_dict *parsed_dict= alloc_aw_dict();
    box->parsed_data = parsed_dict;
    aw_dict_set_int(parsed_dict, "version", version, 0);
    aw_dict_set_int(parsed_dict, "flags", flags, 0);
    aw_dict_set_int(parsed_dict, "sample_count", sample_count, 0);
    aw_dict_set_release_pointer(parsed_dict, "entries", entries, (void (*)(void *, int extra))release_box_entries, sample_count, 0);
}

static void parse_box_stsd(aw_mp4_box *box){
    uint8_t version = data_reader.read_uint8(box->data);
    uint32_t flags = data_reader.read_uint24(box->data);
    uint32_t sample_des_count = data_reader.read_uint32(box->data);
    
    reset_box_data_with_remain_data(box);
    int i = 0;
    for (; i < sample_des_count; i++) {
        create_child_box(box);
        reset_box_data_with_remain_data(box);
    }
    
    aw_dict *parsed_dict= alloc_aw_dict();
    box->parsed_data = parsed_dict;
    aw_dict_set_int(parsed_dict, "version", version, 0);
    aw_dict_set_int(parsed_dict, "flags", flags, 0);
    aw_dict_set_int(parsed_dict, "sample_des_count", sample_des_count, 0);
}

static void parse_box_stsc(aw_mp4_box *box){
    uint8_t version = data_reader.read_uint8(box->data);
    uint32_t flags = data_reader.read_uint24(box->data);
    uint32_t entry_count = data_reader.read_uint32(box->data);
    
    aw_dict **entries = (aw_dict **) aw_alloc(sizeof(aw_dict *) * entry_count);
    int i = 0;
    for (; i < entry_count; i++) {
        aw_dict *one_dict = alloc_aw_dict();
        aw_dict_set_int(one_dict, "first_chunk", data_reader.read_uint32(box->data), 0);
        aw_dict_set_int(one_dict, "samples_one_chunk", data_reader.read_uint32(box->data), 0);
        aw_dict_set_int(one_dict, "sample_description_index", data_reader.read_uint32(box->data), 0);
        entries[i] = one_dict;
    }
    
    aw_dict *parsed_dict= alloc_aw_dict();
    box->parsed_data = parsed_dict;
    aw_dict_set_int(parsed_dict, "version", version, 0);
    aw_dict_set_int(parsed_dict, "flags", flags, 0);
    aw_dict_set_int(parsed_dict, "entry_count", entry_count, 0);
    aw_dict_set_release_pointer(parsed_dict, "entries", entries, (void (*)(void *, int extra))release_box_entries, entry_count, 0);
}

static void parse_box_stss(aw_mp4_box *box){
    uint8_t version = data_reader.read_uint8(box->data);
    uint32_t flags = data_reader.read_uint24(box->data);
    uint32_t sync_sample_count = data_reader.read_uint32(box->data);
    
    aw_dict **entries = (aw_dict **) aw_alloc(sizeof(aw_dict *) * sync_sample_count);
    int i = 0;
    for (; i < sync_sample_count; i++) {
        aw_dict *one_dict = alloc_aw_dict();
        aw_dict_set_int(one_dict, "sample_index", data_reader.read_uint32(box->data), 0);;
        entries[i] = one_dict;
    }
    
    aw_dict *parsed_dict= alloc_aw_dict();
    box->parsed_data = parsed_dict;
    aw_dict_set_int(parsed_dict, "version", version, 0);
    aw_dict_set_int(parsed_dict, "flags", flags, 0);
    aw_dict_set_int(parsed_dict, "sync_sample_count", sync_sample_count, 0);
    aw_dict_set_release_pointer(parsed_dict, "entries", entries, (void (*)(void *, int extra))release_box_entries, sync_sample_count, 0);
}

static void parse_box_stsz(aw_mp4_box *box){
    uint8_t version = data_reader.read_uint8(box->data);
    uint32_t flags = data_reader.read_uint24(box->data);
    uint32_t sample_common_size = data_reader.read_uint32(box->data);
    uint32_t sample_size_count = data_reader.read_uint32(box->data);
    
    aw_dict **entries = (aw_dict **) aw_alloc(sizeof(aw_dict *) * sample_size_count);
    int i = 0;
    for (; i < sample_size_count; i++) {
        aw_dict *one_dict = alloc_aw_dict();
        aw_dict_set_int(one_dict, "one_sample_size", data_reader.read_uint32(box->data), 0);;
        entries[i] = one_dict;
    }
    
    aw_dict *parsed_dict= alloc_aw_dict();
    box->parsed_data = parsed_dict;
    aw_dict_set_int(parsed_dict, "version", version, 0);
    aw_dict_set_int(parsed_dict, "flags", flags, 0);
    aw_dict_set_int(parsed_dict, "sample_common_size", sample_common_size, 0);
    aw_dict_set_int(parsed_dict, "sample_size_count", sample_size_count, 0);
    aw_dict_set_release_pointer(parsed_dict, "entries", entries, (void (*)(void *, int extra))release_box_entries, sample_size_count, 0);
}

static void parse_box_stco(aw_mp4_box *box){
    uint8_t version = data_reader.read_uint8(box->data);
    uint32_t flags = data_reader.read_uint24(box->data);
    uint32_t chunk_offset_count = data_reader.read_uint32(box->data);
    
    aw_dict **entries = (aw_dict **) aw_alloc(sizeof(aw_dict *) * chunk_offset_count);
    int i = 0;
    for (; i < chunk_offset_count; i++) {
        aw_dict *one_dict = alloc_aw_dict();
        aw_dict_set_int(one_dict, "offset", data_reader.read_uint32(box->data), 0);;
        entries[i] = one_dict;
    }
    
    aw_dict *parsed_dict= alloc_aw_dict();
    box->parsed_data = parsed_dict;
    aw_dict_set_int(parsed_dict, "version", version, 0);
    aw_dict_set_int(parsed_dict, "flags", flags, 0);
    aw_dict_set_int(parsed_dict, "chunk_offset_count", chunk_offset_count, 0);
    aw_dict_set_release_pointer(parsed_dict, "entries", entries, (void (*)(void *, int extra))release_box_entries, chunk_offset_count, 0);
}

static uint32_t read_esds_descr_size(aw_mp4_box *box){
    uint8_t b;
    uint8_t numBytes = 0;
    uint32_t size = 0;
    
    do{
        b = data_reader.read_uint8(box->data);
        numBytes++;
        size = (size << 7) | (b & 0x7f);
    }while((b & 0x80) && numBytes < 4);
    
    return size;
}

static void parse_box_esds(aw_mp4_box *box){
    aw_dict *parsed_dict = alloc_aw_dict();
    box->parsed_data = parsed_dict;
    aw_dict_set_int(parsed_dict, "version", data_reader.read_uint8(box->data), 0);
    aw_dict_set_int(parsed_dict, "flags", data_reader.read_uint24(box->data), 0);
    
    uint8_t tag = data_reader.read_uint8(box->data);
    uint32_t size = 0;
    uint16_t es_id = 0;
    uint8_t stream_priority = 0;
    
    //mp4esdescrTag: 3
    if (tag == 3) {
        size = read_esds_descr_size(box);
        es_id = data_reader.read_uint16(box->data);
        stream_priority = data_reader.read_uint8(box->data);
        
        aw_dict_set_int(parsed_dict, "es_id", es_id, 0);
        aw_dict_set_int(parsed_dict, "stream_priority", stream_priority, 0);
        if (size < 20) {
            return;
        }
    }else{
        es_id = data_reader.read_uint16(box->data);
    }
    
    //Mp4DecoderConfigDescrTag 4
    tag = data_reader.read_uint8(box->data);
    aw_dict_set_int(parsed_dict, "es_id", es_id, 0);
    if (tag != 4) {
        return;
    }
    
    size = read_esds_descr_size(box);
    uint8_t object_type_id = data_reader.read_uint8(box->data);
    uint8_t stream_type = data_reader.read_uint8(box->data);
    uint32_t buffer_size_DB = data_reader.read_uint24(box->data);
    uint32_t max_bitrate = data_reader.read_uint32(box->data);
    uint32_t avg_bitrate = data_reader.read_uint32(box->data);
    uint32_t decoder_config_len = 0;
    aw_dict_set_int(parsed_dict, "object_type_id", object_type_id, 0);
    aw_dict_set_int(parsed_dict, "stream_type", stream_type, 0);
    aw_dict_set_int(parsed_dict, "buffer_size_DB", buffer_size_DB, 0);
    aw_dict_set_int(parsed_dict, "max_bitrate", max_bitrate, 0);
    aw_dict_set_int(parsed_dict, "avg_bitrate", avg_bitrate, 0);
    
    if (size < 15) {
        return;
    }
    
    //MP4DecSpecificDescrTag 5 flv 需要这个！！
    tag = data_reader.read_uint8(box->data);
    if (tag != 5) {
        return;
    }
    decoder_config_len = size = read_esds_descr_size(box);
    char *decoder_config;
    data_reader.read_bytes(box->data, &decoder_config, decoder_config_len);
    
    aw_data *decoder_config_data = alloc_aw_data(decoder_config_len);
    memcpy_aw_data(&decoder_config_data, decoder_config, decoder_config_len);
    
    aw_dict_set_release_pointer(parsed_dict, "decoder_config", decoder_config_data, (void (*)(void *, int))release_aw_data_for_box, 0, 0);
    
    aw_free(decoder_config);
    
    //MP4SLConfigDescrTag 6
    tag = data_reader.read_uint8(box->data);
    if (tag != 6) {
        return;
    }
    uint32_t sl_config_len = size = read_esds_descr_size(box);
    char *sl_config;
    data_reader.read_bytes(box->data, &sl_config, sl_config_len);
    
    aw_data *sl_config_data = alloc_aw_data(sl_config_len);
    memcpy_aw_data(&sl_config_data, sl_config, sl_config_len);
    
    aw_dict_set_int(parsed_dict, "sl_config_len", sl_config_len, 0);
    aw_dict_set_release_pointer(parsed_dict, "sl_config", sl_config_data, (void (*)(void *, int))release_aw_data_for_box, 0, 0);
    
    aw_free(sl_config);
}

//视频配置数据，重要！
static void parse_box_avcc(aw_mp4_box *box){
    aw_dict *parsed_dict = alloc_aw_dict();
    box->parsed_data = parsed_dict;
    reset_box_data_with_remain_data(box);
    aw_dict_set_pointer(parsed_dict, "avcc_data", box->data, 0);
}

static void parse_box_mp4a(aw_mp4_box *box){
    if (box->data->size < 44) {
        return;
    }
    data_reader.skip_bytes(box->data, 6);
    uint16_t data_ref_index = data_reader.read_uint16(box->data);
    uint16_t version = data_reader.read_uint16(box->data);
    data_reader.skip_bytes(box->data, 6);
    uint16_t channel_count = data_reader.read_uint16(box->data);
    uint16_t sample_size = data_reader.read_uint16(box->data);
    data_reader.skip_bytes(box->data, 2);
    uint32_t time_scale = data_reader.read_uint32(box->data);
    data_reader.skip_bytes(box->data, 2);
    if (version == 1) {
        data_reader.skip_bytes(box->data, 32);
    }else{
        //TODO
    }
    aw_dict *parsed_dict = alloc_aw_dict();
    box->parsed_data = parsed_dict;
    aw_dict_set_int(parsed_dict, "data_ref_index", data_ref_index, 0);
    aw_dict_set_int(parsed_dict, "version", version, 0);
    aw_dict_set_int(parsed_dict, "channel_count", channel_count, 0);
    aw_dict_set_int(parsed_dict, "sample_size", sample_size, 0);
    aw_dict_set_int(parsed_dict, "time_scale", time_scale, 0);
    
    create_child_box(box);
}

static void parse_box_chan(aw_mp4_box *box){
    //    parse_box_avcc(box);
}

static void parse_box_avc1(aw_mp4_box *box){
    data_reader.skip_bytes(box->data, 6);
    uint16_t data_ref_index = data_reader.read_uint16(box->data);
    data_reader.skip_bytes(box->data, 16);
    uint16_t width = data_reader.read_uint16(box->data);
    uint16_t height = data_reader.read_uint16(box->data);
    uint16_t hor_resolution = data_reader.read_uint32(box->data) >> 16;
    uint16_t ver_resolution = data_reader.read_uint32(box->data) >> 16;
    data_reader.skip_bytes(box->data, 4);
    uint16_t frame_count = data_reader.read_uint16(box->data);
    uint8_t str_len = data_reader.read_uint8(box->data);
    char *compress_name;
    data_reader.read_string(box->data, &compress_name, str_len);
    data_reader.skip_bytes(box->data, 32 - str_len);
    
    uint8_t bitDepth = data_reader.read_uint8(box->data);
    data_reader.skip_bytes(box->data, 2);
    
    aw_dict *parsed_dict = alloc_aw_dict();
    box->parsed_data = parsed_dict;
    aw_dict_set_int(parsed_dict, "data_ref_index", data_ref_index, 0);
    aw_dict_set_int(parsed_dict, "width", width, 0);
    aw_dict_set_int(parsed_dict, "height", height, 0);
    aw_dict_set_int(parsed_dict, "hor_resolution", hor_resolution, 0);
    aw_dict_set_int(parsed_dict, "ver_resolution", ver_resolution, 0);
    aw_dict_set_int(parsed_dict, "frame_count", frame_count, 0);
    aw_dict_set_release_pointer(parsed_dict, "compress_name", compress_name, NULL, 0, 0);
    aw_dict_set_int(parsed_dict, "bitDepth", bitDepth, 0);
    
    reset_box_data_with_remain_data(box);
    create_child_box(box);
}

void aw_mp4box_parse_data(aw_mp4_box *box, aw_data *mp4_data){
    int read_size = 0;
    box->size = data_reader.read_uint32(mp4_data);
    read_size += sizeof(uint32_t);
    if (box->size == 0) {
        return;
    }
    
    box->type = data_reader.read_uint32(mp4_data);
    box->type_string = aw_alloc(5);
    aw_mp4_int32_to_ascii_str(box->type, box->type_string);
    read_size += sizeof(uint32_t);
    if (box->type == BYTES_FM_ASCII('u', 'u', 'i', 'd')) {
        data_reader.read_string(mp4_data, (char **)&box->uuid, 16);
        read_size += 16;
    }
    
    if (box->size == 1) {
        box->size = data_reader.read_uint64(mp4_data);
        read_size += sizeof(uint64_t);
    }
    
    int box_data_size = (int)box->size - read_size;
    if (box_data_size > 0) {
        box->data = alloc_aw_data(box_data_size);
        memcpy_aw_data(&box->data, mp4_data->data + mp4_data->curr_pos, box_data_size);
        data_reader.skip_bytes(mp4_data, box_data_size);
        data_reader.start_read(box->data);
    }
    
    //    AWLog("start parse box type = %s", box->type_string);
    
    int is_handled = 1;
    
    switch (box->type) {
        case BYTES_FM_ASCII('f', 't', 'y', 'p')://1
            break;
        case BYTES_FM_ASCII('m', 'o', 'o', 'v')://4
        case BYTES_FM_ASCII('t', 'r', 'a', 'k')://6
        case BYTES_FM_ASCII('m', 'd', 'i', 'a')://8
        case BYTES_FM_ASCII('m', 'i', 'n', 'f'):
        case BYTES_FM_ASCII('s', 't', 'b', 'l'):
        case BYTES_FM_ASCII('e', 'd', 't', 's')://elst的container
        case BYTES_FM_ASCII('w', 'a', 'v', 'e'):
            create_child_box(box);
            break;
        case BYTES_FM_ASCII('m', 'd', 'a', 't')://3.
            break;
        case BYTES_FM_ASCII('m', 'v', 'h', 'd')://5
            parse_box_mvhd(box);
            break;
        case BYTES_FM_ASCII('t', 'k', 'h', 'd')://7.trak header
            parse_box_tkhd(box);
            break;
        case BYTES_FM_ASCII('e', 'l', 's', 't'):
            parse_box_elst(box);
            break;
        case BYTES_FM_ASCII('m', 'd', 'h', 'd')://9
            parse_box_mdhd(box);
            break;
        case BYTES_FM_ASCII('h', 'd', 'l', 'r')://10
            parse_box_hdlr(box);
            break;
        case BYTES_FM_ASCII('s', 't', 't', 's')://记录了每个sample的duration
            parse_box_stts(box);
            break;
        case BYTES_FM_ASCII('c', 't', 't', 's')://修正stts时间
            parse_box_ctts(box);
            break;
        case BYTES_FM_ASCII('s', 't', 's', 'd')://解析configRecord的关键，这个是特殊的containerBox 上层是stbl
            parse_box_stsd(box);// ->mp4a->esds | avc1->avcc
            break;
        case BYTES_FM_ASCII('s', 't', 's', 'c')://记录的是chunk号，和每个chunk中有几个sample，
            parse_box_stsc(box);
            break;
        case BYTES_FM_ASCII('s', 't', 's', 's'):
            parse_box_stss(box);//记录了所有关键帧sample
            break;
        case BYTES_FM_ASCII('s', 't', 's', 'z'):
            parse_box_stsz(box);//记录了每一个sample的size
            break;
        case BYTES_FM_ASCII('s', 't', 'c', 'o'):
            parse_box_stco(box);//记录了所有chunk的offset
            break;
        case BYTES_FM_ASCII('e', 's', 'd', 's')://包含了audio 中必不可少的configRecord flv中很重要 stsd.mp4a.esds
            parse_box_esds(box);
            break;
        case BYTES_FM_ASCII('a', 'v', 'c', 'C')://包含了video 中必不可少的configRecord flv中很重要 stsd.avc1.avcc
            parse_box_avcc(box);
            break;
        case BYTES_FM_ASCII('m', 'p', '4', 'a')://特殊的containerBox
            parse_box_mp4a(box);
            break;
        case BYTES_FM_ASCII('c', 'h', 'a', 'n'):
            parse_box_chan(box);
            break;
        case BYTES_FM_ASCII('a', 'v', 'c', '1')://特殊的containerBox
            parse_box_avc1(box);
            break;
        default:
            is_handled = 0;
            break;
    }
}

#include "aw_file.h"

static void test_free_mp4_box_for_dict(aw_mp4_box *box, int extra){
    free_aw_mp4_box(&box);
}

extern void aw_test_mp4_box(void *data, int32_t len){
    aw_uninit_debug_alloc();
    aw_init_debug_alloc();
    aw_data *mp4_data = (aw_data *)alloc_aw_data(len);
    memcpy_aw_data(&mp4_data, data, len);
    
    aw_dict *mp4box_dict = alloc_aw_dict();
    
    data_reader.start_read(mp4_data);
    while (data_reader.remain_count(mp4_data)) {
        aw_mp4_box *mp4_box = alloc_aw_mp4_box();
        aw_mp4box_parse_data(mp4_box, mp4_data);
        aw_dict_set_release_pointer(mp4box_dict, mp4_box->type_string, mp4_box, (void (*)(void *, int))test_free_mp4_box_for_dict, 0, 1);
    }
    
    AWLog("解析完成 呵呵");
    
    //test find
    aw_mp4_box *moov_box = aw_dict_get_pointer(mp4box_dict, "moov");
    aw_mp4_box *hdlr_box = aw_mp4box_find_box(moov_box, "trak@0.mdia.hdlr");
    aw_mp4_box *edts_box = aw_mp4box_find_box(moov_box, "trak@1.edts");
    aw_mp4_box *elst_box = aw_mp4box_find_box(moov_box, "trak@1.edts.elst");
    
    AWLog("moov_box=%p, hdlr_box=%p, edts_box=%p, elst_box=%p", moov_box, hdlr_box, edts_box, elst_box);
    
    //释放资源
    free_aw_data(&mp4_data);
    free_aw_dict(&mp4box_dict);
    
    AWLog("alloc size= %ld, free size=%ld", aw_total_alloc_size(), aw_total_free_size());
}
