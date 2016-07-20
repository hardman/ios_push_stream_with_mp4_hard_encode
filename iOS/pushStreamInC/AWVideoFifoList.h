//
//  AWVideoFifoList.h
//  TestRecordVideo
//
//  Created by kaso on 23/5/16.
//  Copyright © 2016年 xcyo. All rights reserved.
//

#import <Foundation/Foundation.h>
@interface AWVideoFifoList : NSObject
//数量限制
@property (nonatomic, unsafe_unretained) NSInteger eleLimit;
-(void) addElement:(id)ele;
-(id) popElement;
-(void) addElementToHeader:(id)ele;
-(void) clean;
-(BOOL) empty;
-(NSInteger) count;
@end
