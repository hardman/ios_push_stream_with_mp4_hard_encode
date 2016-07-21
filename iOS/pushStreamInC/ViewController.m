//
//  ViewController.m
//  pushStreamInC
//
//  Created by kaso on 26/5/16.
//  Copyright © 2016年 xcyo. All rights reserved.
//

#import "ViewController.h"
#import <AVFoundation/AVFoundation.h>
#import <AssetsLibrary/AssetsLibrary.h>

#import "AWVideoCapture.h"

@interface ViewController ()<AWVideoCaptureDelegate>
//按钮
@property (nonatomic, strong) UIButton *startBtn;
@property (nonatomic, strong) UIButton *switchBtn;
@property (nonatomic, strong) UIButton *saveVideoBtn;

@property (nonatomic, strong) UIView *preview;
@property (nonatomic, strong) AWVideoCapture *capture;
@end

@implementation ViewController

-(void)videoCapture:(AWVideoCapture *)capture stateChangeFrom:(aw_rtmp_state)fromState toState:(aw_rtmp_state)toState{
    switch (toState) {
        case aw_rtmp_state_idle: {
            self.startBtn.enabled = YES;
            [self.startBtn setTitle:@"开始录制" forState:UIControlStateNormal];
            break;
        }
        case aw_rtmp_state_connecting: {
            self.startBtn.enabled = NO;
            [self.startBtn setTitle:@"连接中" forState:UIControlStateNormal];
            break;
        }
        case aw_rtmp_state_opened: {
            self.startBtn.enabled = YES;
            break;
        }
        case aw_rtmp_state_closed: {
            self.startBtn.enabled = YES;
            break;
        }
        case aw_rtmp_state_error_write: {
            break;
        }
        case aw_rtmp_state_error_open: {
            break;
        }
        case aw_rtmp_state_error_net: {
            break;
        }
    }
}

- (void)viewDidLoad {
    [super viewDidLoad];
    [self.preview addSubview: self.capture.preview];
    self.capture.preview.center = self.preview.center;
    [self createUI];
    [self test];
}

-(AWVideoCapture *) capture{
    if (!_capture) {
        _capture = [AWVideoCapture new];
        _capture.delegate = self;
    }
    return _capture;
}

-(UIView *)preview{
    if (!_preview) {
        _preview = [UIView new];
        _preview.frame = self.view.bounds;
        [self.view addSubview:_preview];
        [self.view sendSubviewToBack:_preview];
    }
    return _preview;
}

-(void) test{
}

-(void) createUI{
    self.startBtn = [[UIButton alloc] initWithFrame:CGRectMake(100, 100, 100, 30)];
    [self.startBtn setTitle:@"开始录制！" forState:UIControlStateNormal];
    [self.startBtn addTarget:self action:@selector(onStartClick) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:self.startBtn];
    
    self.switchBtn = [[UIButton alloc] initWithFrame:CGRectMake(230, 100, 100, 30)];
    [self.switchBtn setTitle:@"换摄像头！" forState:UIControlStateNormal];
    [self.switchBtn addTarget:self action:@selector(onSwitchClick) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:self.switchBtn];
    
    self.saveVideoBtn = [[UIButton alloc] initWithFrame:CGRectMake(100, 150, 150, 30)];
    [self.saveVideoBtn setTitle:@"保存到相册！" forState:UIControlStateNormal];
    [self.saveVideoBtn addTarget:self action:@selector(onSaveVideo) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:self.saveVideoBtn];
}

-(void) onStartClick{
    if (self.capture.isCapturing) {
        [self.startBtn setTitle:@"开始录制！" forState:UIControlStateNormal];
        [self.capture stopCapture];
    }else{
        self.capture.rtmpUrl = @"rtmp://192.168.1.124/live/test";
        if ([self.capture startCapture]) {
            [self.startBtn setTitle:@"停止录制！" forState:UIControlStateNormal];
        }
    }
}

-(void) onSwitchClick{
    [self.capture switchCamera];
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
        NSLog(@"没有找到文件！%@",path);
    }
}

-(void) onSaveVideo{
    [self savePathToLibrary:self.capture.videoMp4File.defaultPath];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


@end
