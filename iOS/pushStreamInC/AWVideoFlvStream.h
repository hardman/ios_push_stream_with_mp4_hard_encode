//
//  VideoFlvStreamVideoFlvStream.h
//  TestRecordVideo
//
//  Created by kaso on 23/5/16.
//  Copyright © 2016年 xcyo. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "AWVideoMp4File.h"
#import "AWVideoFifoList.h"

@interface AWVideoFlvStream : NSObject
@property (nonatomic, copy) NSString *rtmpUrl;
-(void) notifyMp4FileListChanged:(AWVideoFifoList *)list;
-(void) startStream;
-(void) stopStream;
@end
