//
//  AWVideoMp4File.h
//  TestRecordVideo
//
//  Created by kaso on 23/5/16.
//  Copyright © 2016年 xcyo. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

@interface AWVideoMp4File : NSObject
@property (nonatomic, readonly, unsafe_unretained) BOOL isWriting;
//视频属性：
@property (nonatomic, unsafe_unretained) NSInteger vWidth;
@property (nonatomic, unsafe_unretained) NSInteger vHeight;
@property (nonatomic, copy) NSString *vCodec;
@property (nonatomic, unsafe_unretained) NSInteger vBitRate;
@property (nonatomic, unsafe_unretained) NSInteger vMaxFrames;
//音频属性
@property (nonatomic, unsafe_unretained) NSInteger aCodec;
@property (nonatomic, unsafe_unretained) CGFloat aSampleRate;
@property (nonatomic, unsafe_unretained) NSInteger aBitRate;
@property (nonatomic, unsafe_unretained) NSInteger aChanlesNum;
@property (nonatomic, copy) NSData *aChannelLayoutData;
//视频写入
@property (nonatomic, readonly, strong) AVAssetWriter *fileAssetWriter;
//video 输入
@property (nonatomic, readonly, strong) AVAssetWriterInput *videoAssetWriterInput;
//audio 输入
@property (nonatomic, readonly, strong) AVAssetWriterInput *audioAssetWriterInput;

-(BOOL) startWritingWithTime:(CMTime) time;

-(void) stopWritingWithFinishHandler:(void(^)())finishHandler;

-(BOOL) writeAudioSample:(CMSampleBufferRef) audioSample;
-(BOOL) writeVideoSample:(CMSampleBufferRef) videoSample;

//当前文件路径
@property (nonatomic, readonly, copy) NSString *currentFilePath;

-(NSString *)defaultPath;

//重建一个新的fileAssetWriter
-(void) newFileAssetWriter:(void (^)())preFileFinishHandler;

-(void) newFileAssetWriterWithOutSaveFile;

-(void)clean;

@end
