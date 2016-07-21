//
//  AWVideoMp4File.m
//  TestRecordVideo
//
//  Created by kaso on 23/5/16.
//  Copyright © 2016年 xcyo. All rights reserved.
//

#import "AWVideoMp4File.h"
#import "AWVideoFifoList.h"
#import <AssetsLibrary/AssetsLibrary.h>
#import "AWVideoFlvStream.h"

@interface AWVideoMp4File ()
//视频写入
@property (nonatomic, strong) AVAssetWriter *fileAssetWriter;
//video 输入
@property (nonatomic, strong) AVAssetWriterInput *videoAssetWriterInput;
//audio 输入
@property (nonatomic, strong) AVAssetWriterInput *audioAssetWriterInput;

@property (nonatomic, unsafe_unretained) BOOL isWriting;

@property (nonatomic, copy) NSString *currentFilePath;

//一个文件中写入多少个samples
@property (nonatomic, unsafe_unretained) NSInteger samplesCountInEveryFile;

@property (nonatomic, unsafe_unretained) NSInteger samplesCountCurrentFile;

//已经完成的文件列表
@property (nonatomic, strong) AWVideoFifoList *completedFileList;

@property (nonatomic, strong) AWVideoFlvStream *flvStream;

@end

@implementation AWVideoMp4File
//视频属性
-(NSInteger)vWidth{
    if (_vWidth == 0) {
        _vWidth = 320;
    }
    return _vWidth;
}

-(NSInteger)vHeight{
    if (_vHeight == 0) {
        _vHeight = 576;
    }
    return _vHeight;
}

-(NSString *)vCodec{
    if (_vCodec == 0) {
        _vCodec = AVVideoCodecH264;
    }
    return _vCodec;
}

-(NSInteger)vBitRate{
    if (_vBitRate == 0) {
        _vBitRate = 600000;
    }
    return _vBitRate;
}

-(NSInteger)vMaxFrames{
    if (_vMaxFrames == 0) {
        _vMaxFrames = 100;
    }
    return _vMaxFrames;
}

//音频属性
-(NSInteger)aCodec{
    if (_aCodec == 0) {
        _aCodec = kAudioFormatMPEG4AAC;
    }
    return _aCodec;
}

-(NSInteger)aBitRate{
    if (_aBitRate == 0) {
        _aBitRate = 256000;
    }
    return _aBitRate;
}

-(CGFloat)aSampleRate{
    if (_aSampleRate == 0) {
        _aSampleRate = 44100;
    }
    return _aSampleRate;
}

-(NSInteger)aChanlesNum{
    if (_aChanlesNum == 0) {
        _aChanlesNum = 1;
    }
    return _aChanlesNum;
}

-(NSData *)aChannelLayoutData{
    if (!_aChannelLayoutData) {
        _aChannelLayoutData = [NSData data];
    }
    return _aChannelLayoutData;
}

-(BOOL)isWriting{
    return _isWriting && _fileAssetWriter && _fileAssetWriter.status == AVAssetWriterStatusWriting;
}

-(NSInteger)samplesCountInEveryFile{
    //这个数值比较重要，因为浮点数的原因，所以取音视频刚好是整数时间的值。
    //因为视频刚好是0.033333的帧率，所以0.033333*60刚好是2，这就保证视频不会卡顿
    //音频的话，音频帧率为0.023220
    if (_samplesCountInEveryFile == 0) {
        _samplesCountInEveryFile = 60;
    }
    return _samplesCountInEveryFile;
}

-(AWVideoFifoList *)completedFileList{
    if (!_completedFileList) {
        _completedFileList = [AWVideoFifoList new];
    }
    return _completedFileList;
}

-(AWVideoFlvStream *)flvStream{
    if (!_flvStream) {
        _flvStream = [AWVideoFlvStream new];
    }
    return _flvStream;
}

-(void) createAssetWriterWithFileName:(NSString *)path{
    NSError *error = nil;
    
    if([[NSFileManager defaultManager] fileExistsAtPath:path]){
        [[NSFileManager defaultManager]removeItemAtPath:path error:&error];
    }
    
    self.currentFilePath = path;
    
    _fileAssetWriter = [[AVAssetWriter alloc] initWithURL:[NSURL fileURLWithPath:path isDirectory:NO] fileType:AVFileTypeMPEG4 error:&error];
    
    if (!_videoAssetWriterInput) {
        _videoAssetWriterInput =
        [AVAssetWriterInput assetWriterInputWithMediaType: AVMediaTypeVideo
                                           outputSettings:@{
                                                            AVVideoCodecKey:self.vCodec,
                                                            AVVideoWidthKey:@(self.vWidth),
                                                            AVVideoHeightKey:@(self.vHeight),
                                                            AVVideoCompressionPropertiesKey:@{
                                                                    AVVideoAverageBitRateKey: @(self.vBitRate),
                                                                    AVVideoMaxKeyFrameIntervalKey:@(self.vMaxFrames)
                                                                    }
                                                            }];
        _videoAssetWriterInput.expectsMediaDataInRealTime = YES;
    }
    
    if ([_fileAssetWriter canAddInput:_videoAssetWriterInput]) {
        [_fileAssetWriter addInput:_videoAssetWriterInput];
    }
    
    if (!_audioAssetWriterInput) {
        _audioAssetWriterInput =
        [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeAudio
                                           outputSettings:@{
                                                            AVFormatIDKey: @(self.aCodec),
                                                            AVSampleRateKey: @(self.aSampleRate),
                                                            AVEncoderBitRatePerChannelKey:@(self.aBitRate),
                                                            AVNumberOfChannelsKey: @(self.aChanlesNum),
                                                            AVChannelLayoutKey:self.aChannelLayoutData
                                                            }];
        _audioAssetWriterInput.expectsMediaDataInRealTime = YES;
    }
    
    if ([_fileAssetWriter canAddInput:_audioAssetWriterInput]) {
        [_fileAssetWriter addInput:_audioAssetWriterInput];
    }
}

-(AVAssetWriter *)fileAssetWriter{
    if (!_fileAssetWriter) {
        [self createAssetWriterWithFileName: self.randomPath];
    }
    return _fileAssetWriter;
}

-(BOOL) startWritingWithTime:(CMTime) time{
    if (self.isWriting) {
        return NO;
    }
    if (self.fileAssetWriter.status == AVAssetWriterStatusUnknown) {
        if([self.fileAssetWriter startWriting]){
            if (0 == CMTimeCompare(time, kCMTimeInvalid)) {
                time = kCMTimeZero;
            }
            [self.fileAssetWriter startSessionAtSourceTime:time];
        }else{
            return NO;
        }
    }else if(self.fileAssetWriter.status != AVAssetWriterStatusWriting){
        return NO;
    }
    
    self.isWriting = YES;
    
    [self.flvStream startStream];
    
    return YES;
}

-(void) completeOneFile:(void(^)())finishHandler{
    [self.audioAssetWriterInput markAsFinished];
    [self.videoAssetWriterInput markAsFinished];
    [self.fileAssetWriter finishWritingWithCompletionHandler:finishHandler];
    
    // samples count current file set to 0
    self.samplesCountCurrentFile = 0;
    // add current file to completed File List
    [self.completedFileList addElement:self.currentFilePath];
}

-(void) stopWritingWithFinishHandler:(void(^)())finishHandler{
    if (!self.isWriting) {
        return;
    }
    
    @try{
        [self completeOneFile:finishHandler];
    }@catch(id exception){
        NSLog(@"complete one file exception %@", exception);
    }
    
    self.fileAssetWriter = nil;
    self.isWriting = NO;
    
    [self.flvStream stopStream];
    self.flvStream = nil;
}

-(BOOL) writeAudioSample:(CMSampleBufferRef) audioSample{
    if (self.isWriting && self.audioAssetWriterInput.isReadyForMoreMediaData) {
        //        NSLog(@"write one audio sample to file");
        @try{
            [self.audioAssetWriterInput appendSampleBuffer:audioSample];
            return YES;
        }@catch(NSException *exception){
            NSLog(@"write audioSample exception: %@", exception);
        }
    }else{
        NSLog(@"[ERROR] writeAudioSample failed! writing=%d isReady=%d", self.isWriting, self.audioAssetWriterInput.readyForMoreMediaData);
    }
    return NO;
}

-(void) savePathToLibrary:(NSString *)path{
    NSURL *url = [NSURL fileURLWithPath:path];
    NSFileManager *fileManager = [NSFileManager defaultManager];
    
    if ([[NSFileManager defaultManager] fileExistsAtPath:path]) {
        ALAssetsLibrary *assetsLib = [ALAssetsLibrary new];
        [assetsLib writeVideoAtPathToSavedPhotosAlbum:url completionBlock:^(NSURL *assetURL, NSError *error) {
            NSLog(@"save file path=%@ assetUrl=%@ error=%@", path, assetURL, error);
            [fileManager removeItemAtPath:path error:nil];
        }];
    }else{
        NSLog(@"没有找到文件！%@", path);
    }
}

-(BOOL) writeVideoSample:(CMSampleBufferRef) videoSample{
    if (self.isWriting && self.videoAssetWriterInput.isReadyForMoreMediaData) {
        //        NSLog(@"write one video sample to file");
        @try{
            [self.videoAssetWriterInput appendSampleBuffer:videoSample];
            if (++self.samplesCountCurrentFile >= self.samplesCountInEveryFile) {
                [self newFileAssetWriter:^{
                    NSLog(@"完成一个文件：self.completedFileList=%@", self.completedFileList);
                    [self.flvStream notifyMp4FileListChanged:self.completedFileList];
                }];
            }
            return YES;
        }@catch(NSException *exception){
            NSLog(@"write videoSample exception: %@", exception);
        }
    }else{
        NSLog(@"[ERROR] writeVideoSample failed! writing=%d isReady=%d", self.isWriting, self.videoAssetWriterInput.readyForMoreMediaData);
    }
    return NO;
}

-(NSString *)defaultPath{
    return [NSTemporaryDirectory() stringByAppendingPathComponent:@"tmp.mp4"];
}

-(NSString *)randomPath{
    return [NSTemporaryDirectory() stringByAppendingPathComponent:[[NSUUID UUID].UUIDString stringByAppendingString:@".mp4"]];
}

-(void) newFileAssetWriter:(void (^)())preFileFinishHandler{
    [self completeOneFile:preFileFinishHandler];
    [self newFileAssetWriterWithOutSaveFile];
}

-(void) newFileAssetWriterWithOutSaveFile{
    self.fileAssetWriter = nil;
    [self fileAssetWriter];
}

-(void)clean{
    [self completeOneFile:nil];
    self.fileAssetWriter = nil;
    //移除所有文件
    while(!self.completedFileList.empty) {
        NSString *mp4File = self.completedFileList.popElement;
        [[NSFileManager defaultManager] removeItemAtPath:mp4File error:nil];
    }
}
@end
