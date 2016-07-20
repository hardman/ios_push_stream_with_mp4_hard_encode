//
//  AWVideoCapture.m
//  TestRecordVideo
//
//  Created by kaso on 23/5/16.
//  Copyright © 2016年 xcyo. All rights reserved.
//

#import "AWVideoCapture.h"
#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>
#import "AWVideoFifoList.h"
#include "aw_pushstream.h"

//保存sample数据
@interface AWVideoFrame : NSObject
@property (nonatomic, unsafe_unretained) CMSampleBufferRef sampleBuff;
@property (nonatomic, unsafe_unretained) BOOL isVideo;
@end

@implementation AWVideoFrame
@end

@interface AWVideoCapture ()<AVCaptureVideoDataOutputSampleBufferDelegate, AVCaptureAudioDataOutputSampleBufferDelegate>
//前后摄像头
@property (nonatomic, strong) AVCaptureDeviceInput *frontCamera;
@property (nonatomic, strong) AVCaptureDeviceInput *backCamera;
//当前使用的视频设备
@property (nonatomic, weak) AVCaptureDeviceInput *videoInputDevice;

//麦克风
@property (nonatomic, strong) AVCaptureDeviceInput *audioInputDevice;

//输出文件
@property (nonatomic, strong) AVCaptureVideoDataOutput *videoDataOutput;
@property (nonatomic, strong) AVCaptureAudioDataOutput *audioDataOutput;

//会话
@property (nonatomic, strong) AVCaptureSession *captureSession;

//预览
@property (nonatomic, strong) AVCaptureVideoPreviewLayer *previewLayer;
//预览结果view
@property (nonatomic, strong) UIView *preview;

//保存sampleList
@property (nonatomic, strong) AWVideoFifoList *sampleList;

//VideoFile
@property (nonatomic, strong) AWVideoMp4File *videoMp4File;

//处理videFile
@property (nonatomic, strong) dispatch_source_t videoFileSource;

//是否正在capture
@property (nonatomic, unsafe_unretained) BOOL isCapturing;
@end

__weak static AWVideoCapture *sAwVideoCapture = nil;

@implementation AWVideoCapture

//属性实现
-(AWVideoFifoList *)sampleList{
    if (!_sampleList) {
        _sampleList = [AWVideoFifoList new];
    }
    return _sampleList;
}

-(AWVideoMp4File *)videoMp4File{
    if (!_videoMp4File) {
        _videoMp4File = [AWVideoMp4File new];
    }
    return _videoMp4File;
}

-(void)setisCapturing:(BOOL)isCapturing{
    if (_isCapturing == isCapturing) {
        return;
    }
    
    if (!isCapturing) {
        [self onPauseCapture];
    }else{
        [self onStartCapture];
    }
    
    _isCapturing = isCapturing;
}

-(void)switchCamera{
    if ([self.videoInputDevice isEqual: self.frontCamera]) {
        self.videoInputDevice = self.backCamera;
    }else{
        self.videoInputDevice = self.frontCamera;
    }
}

-(UIView *)preview{
    if (!_preview) {
        _preview = [UIView new];
        _preview.bounds = [UIScreen mainScreen].bounds;
    }
    return _preview;
}

-(dispatch_source_t)videoFileSource{
    if (!_videoFileSource) {
        [self createVideoFileSource];
    }
    return _videoFileSource;
}

- (instancetype)init
{
    self = [super init];
    if (self) {
        [self createCaptureDevice];
        [self createCaptureSession];
        [self createPreviewLayer];
        sAwVideoCapture = self;
    }
    return self;
}

//初始化视频设备
-(void) createCaptureDevice{
    //创建视频设备
    NSArray *videoDevices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    //初始化摄像头
    self.frontCamera = [AVCaptureDeviceInput deviceInputWithDevice:videoDevices.firstObject error:nil];
    self.backCamera =[AVCaptureDeviceInput deviceInputWithDevice:videoDevices.lastObject error:nil];
    
    //麦克风
    AVCaptureDevice *audioDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeAudio];
    self.audioInputDevice = [AVCaptureDeviceInput deviceInputWithDevice:audioDevice error:nil];
    
    [self createOutput];
    
    self.videoInputDevice = self.frontCamera;
}

//切换摄像头
-(void)setVideoInputDevice:(AVCaptureDeviceInput *)videoInputDevice{
    //modifyinput
    [self.captureSession beginConfiguration];
    if (_videoInputDevice) {
        [self.captureSession removeInput:_videoInputDevice];
    }
    if (videoInputDevice) {
        [self.captureSession addInput:videoInputDevice];
    }
    
    [self setVideoOutConfig];
    
    [self.captureSession commitConfiguration];
    
    _videoInputDevice = videoInputDevice;
}

//创建预览
-(void) createPreviewLayer{
    self.previewLayer = [AVCaptureVideoPreviewLayer layerWithSession:self.captureSession];
    self.previewLayer.frame = self.preview.bounds;
    [self.preview.layer addSublayer:self.previewLayer];
}

-(void) setVideoOutConfig{
    for (AVCaptureConnection *conn in self.videoDataOutput.connections) {
        if (conn.isVideoStabilizationSupported) {
            [conn setPreferredVideoStabilizationMode:AVCaptureVideoStabilizationModeAuto];
        }
        if (conn.isVideoOrientationSupported) {
            [conn setVideoOrientation:AVCaptureVideoOrientationPortrait];
        }
        if (conn.isVideoMirrored) {
            [conn setVideoMirrored: YES];
        }
    }
}

//创建会话
-(void) createCaptureSession{
    self.captureSession = [AVCaptureSession new];
    
    [self.captureSession beginConfiguration];
    
    if ([self.captureSession canAddInput:self.videoInputDevice]) {
        [self.captureSession addInput:self.videoInputDevice];
    }
    
    if ([self.captureSession canAddInput:self.audioInputDevice]) {
        [self.captureSession addInput:self.audioInputDevice];
    }
    
    if([self.captureSession canAddOutput:self.videoDataOutput]){
        [self.captureSession addOutput:self.videoDataOutput];
        [self setVideoOutConfig];
    }
    
    if([self.captureSession canAddOutput:self.audioDataOutput]){
        [self.captureSession addOutput:self.audioDataOutput];
    }
    
    self.captureSession.sessionPreset = AVCaptureSessionPresetHigh;
    
    [self.captureSession commitConfiguration];
    
    [self.captureSession startRunning];
}

//销毁会话
-(void) destroyCaptureSession{
    if (self.captureSession) {
        [self.captureSession removeInput:self.audioInputDevice];
        [self.captureSession removeInput:self.videoInputDevice];
        [self.captureSession removeOutput:self.self.videoDataOutput];
        [self.captureSession removeOutput:self.self.audioDataOutput];
    }
    self.captureSession = nil;
}

-(void) createOutput{
    self.videoDataOutput = [[AVCaptureVideoDataOutput alloc] init];
    [self.videoDataOutput setSampleBufferDelegate:self queue:dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0)];
    [self.videoDataOutput setAlwaysDiscardsLateVideoFrames:YES];
    [self.videoDataOutput setVideoSettings:@{
                                             (__bridge NSString *)kCVPixelBufferPixelFormatTypeKey:@(kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange)
                                             }];
    
    self.audioDataOutput = [[AVCaptureAudioDataOutput alloc] init];
    [self.audioDataOutput setSampleBufferDelegate:self queue:dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0)];
}

-(void) createVideoFileSource{
    dispatch_source_t source = dispatch_source_create(DISPATCH_SOURCE_TYPE_DATA_ADD, 0, 0, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));
    dispatch_source_set_event_handler(source, ^{
        @synchronized (_videoFileSource) {
            while (!self.sampleList.empty) {
                AWVideoFrame *frame = self.sampleList.popElement;
                if (!self.videoMp4File.isWriting) {
                    if([self.videoMp4File startWritingWithTime:CMSampleBufferGetPresentationTimeStamp(frame.sampleBuff)]){
                        NSLog(@"start writing with time is succ!!!");
                    }else{
                        NSLog(@"start writing with time is Error!!!");
                    }
                }
                if (self.videoMp4File.isWriting) {
                    BOOL isSucc = NO;
                    if (frame.isVideo) {
                        isSucc = [self.videoMp4File writeVideoSample:frame.sampleBuff];
                    }else{
                        isSucc = [self.videoMp4File writeAudioSample:frame.sampleBuff];
                    }
                    if (isSucc) {
                        CFRelease(frame.sampleBuff);
                    }else{
                        //这么处理可能会丢帧，但是并不影响，最多少2秒
                        [self.sampleList addElementToHeader:frame];
                        [self.videoMp4File newFileAssetWriterWithOutSaveFile];
                        break;
                    }
                }else{
                    [self.sampleList addElementToHeader:frame];
                    [self.videoMp4File newFileAssetWriterWithOutSaveFile];
                }
            }
        }
    });
    
    _videoFileSource = source;
    
    [self resumeVideoFileSource];
}

-(void) resumeVideoFileSource{
    if (_videoFileSource) {
        dispatch_resume(_videoFileSource);
    }
}

-(void) pauseVideoFileSource{
    if (_videoFileSource) {
        dispatch_suspend(_videoFileSource);
    }
}

-(void) stopVideoFileSource{
    if (_videoFileSource) {
        dispatch_source_cancel(_videoFileSource);
    }
}

-(void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection{
    if (self.isCapturing) {
        AWVideoFrame *frame = [AWVideoFrame new];
        frame.sampleBuff = sampleBuffer;
        if ([self.videoDataOutput isEqual:captureOutput]) {
            frame.isVideo = YES;
        }else if([self.audioDataOutput isEqual:captureOutput]){
            frame.isVideo = NO;
        }else{
            NSLog(@"!!!!!!!!!!! is not video and not audio ");
        }
        
        CFRetain(frame.sampleBuff);
        [self.sampleList addElement:frame];
        
        dispatch_source_merge_data(self.videoFileSource, 1);
    }
}

-(void) onPauseCapture{
    [self pauseVideoFileSource];
    [self.sampleList clean];
}

-(void) onStartCapture{
    [self resumeVideoFileSource];
}

static void aw_rtmp_state_changed_cb_in_oc(aw_rtmp_state old_state, aw_rtmp_state new_state){
    NSLog(@"[OC] rtmp state changed from(%d), to(%d)", old_state, new_state);
    [sAwVideoCapture.delegate videoCapture:sAwVideoCapture stateChangeFrom:old_state toState:new_state];
}

-(BOOL) startCapture{
    if (!self.rtmpUrl || self.rtmpUrl.length < 8) {
        NSLog(@"rtmpUrl is nil when start capture");
        return NO;
    }
    int retcode = aw_open_rtmp_context_for_parsed_mp4(self.rtmpUrl.UTF8String, aw_rtmp_state_changed_cb_in_oc);
    if(retcode){
        self.isCapturing = YES;
    }else{
        NSLog(@"startCapture rtmpOpen error!!! retcode=%d", retcode);
        return NO;
    }
    return YES;
}

-(void) stopCapture{
    self.isCapturing = NO;
    [self.videoMp4File stopWritingWithFinishHandler:^{
        NSLog(@"capture stoped");
    }];
    aw_close_rtmp_context_for_parsed_mp4();
}

-(BOOL) startCaptureWithCMTime:(CMTime) time{
    if([self.videoMp4File startWritingWithTime:time]){
        self.isCapturing = YES;
        return YES;
    }
    return NO;
}

@end
