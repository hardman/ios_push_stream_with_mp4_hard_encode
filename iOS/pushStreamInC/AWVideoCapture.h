//
//  AWVideoCapture.h
//  TestRecordVideo
//
//  Created by kaso on 23/5/16.
//  Copyright © 2016年 xcyo. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "AWVideoMp4File.h"
#include "aw_pushstream.h"

@class AWVideoCapture;
@protocol AWVideoCaptureDelegate <NSObject>
-(void) videoCapture:(AWVideoCapture *)capture stateChangeFrom:(aw_rtmp_state) fromState toState:(aw_rtmp_state) toState;
@end

@interface AWVideoCapture : NSObject
//状态变化回调
@property (nonatomic, weak) id<AWVideoCaptureDelegate> delegate;

//是否将数据收集起来
@property (nonatomic, readonly, unsafe_unretained) BOOL isCapturing;

//预览view
@property (nonatomic, readonly, strong) UIView *preview;

//VideoFile
@property (nonatomic, readonly, strong) AWVideoMp4File *videoMp4File;

@property (nonatomic, copy) NSString *rtmpUrl;

//切换摄像头
-(void) switchCamera;

//停止capture
-(void) stopCapture;

//开始capture
-(BOOL) startCapture;

//开始capture
-(BOOL) startCaptureWithCMTime:(CMTime) time;

@end
