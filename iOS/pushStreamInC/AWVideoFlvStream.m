//
//  VideoFlvStreamVideoFlvStream.m
//  TestRecordVideo
//
//  Created by kaso on 23/5/16.
//  Copyright © 2016年 xcyo. All rights reserved.
//

#import "AWVideoFlvStream.h"
#import <AssetsLibrary/AssetsLibrary.h>
#include "aw_pushstream.h"

@interface AWVideoFlvStream ()
//是否已经发送了flv header
@property (nonatomic, unsafe_unretained) BOOL isHeaderSend;
//保证不被调用多次
@property (atomic, unsafe_unretained) BOOL isProcessing;

@property (atomic, unsafe_unretained) BOOL isStreaming;

@property (nonatomic, unsafe_unretained) uint32_t startTime;

@property (nonatomic, unsafe_unretained) double_t currentTime;

@property (nonatomic, unsafe_unretained) double_t lastVideoTimeStamp;

@property (nonatomic, unsafe_unretained) double_t lastAudioTimeStamp;
@end

@implementation AWVideoFlvStream

-(void) notifyMp4FileListChanged:(AWVideoFifoList *)list{
    if (!self.isStreaming) {
        NSLog(@"notifyMp4FileListChanged VideoFlvSteam is not streaming");
        return;
    }
    if (self.isProcessing) {
        NSLog(@"AWVideoFlvStream notifyMp4FileListChanged isProcessing!!!");
        return;
    }
    self.isProcessing = YES;
    NSLog(@" -- notify mp4 file list changed list.count=%ld", list.count);
    while (!list.empty) {
        [self handleOneMp4File:list.popElement];
    }
    self.isProcessing = NO;
}

-(void) handleOneMp4File:(NSString *)mp4File{
    NSLog(@"=== handleOneMp4File mp4File=%@", mp4File);
    //解析mp4文件
    
    [self parseMp4AndSendFlvStream:mp4File];
    
    //移除处理过的文件
    [[NSFileManager defaultManager] removeItemAtPath:mp4File error:nil];
}

-(void) parseMp4AndSendFlvStream:(NSString *)mp4File{
    NSData *mp4FileData = [NSData dataWithContentsOfFile:mp4File];
    aw_convert_parsed_mp4_to_flv_stream(mp4FileData.bytes, mp4FileData.length);
}

//开始发送流
-(void) startStream{
    self.isStreaming = YES;
}

//停止发送流
-(void)stopStream{
    self.isStreaming = NO;
    self.isHeaderSend = NO;
}
@end
