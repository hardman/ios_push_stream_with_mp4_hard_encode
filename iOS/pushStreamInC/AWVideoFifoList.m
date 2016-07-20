//
//  AWVideoFifoList.m
//  TestRecordVideo
//
//  Created by kaso on 23/5/16.
//  Copyright © 2016年 xcyo. All rights reserved.
//

#import "AWVideoFifoList.h"

@interface AWVideoFifoList ()
@property (nonatomic, strong) NSMutableArray *mutableArray;
@end

@implementation AWVideoFifoList

-(NSMutableArray *)mutableArray{
    if (!_mutableArray) {
        _mutableArray = [NSMutableArray new];
    }
    return _mutableArray;
}

-(NSInteger)eleLimit{
    if (!_eleLimit) {
        _eleLimit = 8 * 25;
    }
    return _eleLimit;
}

-(void) addElement:(id)ele{
    @synchronized (self) {
        if (self.mutableArray.count >= self.eleLimit) {
            NSLog(@"drop one frame!!!!");
            [self popElement];
        }
        [self.mutableArray addObject:ele];
    }
}

-(void) addElementToHeader:(id)ele{
    [self.mutableArray insertObject:ele atIndex:0];
}

-(id) popElement{
    id ret = nil;
    @synchronized (self) {
        ret = self.mutableArray.firstObject;
        [self.mutableArray removeObjectAtIndex:0];
    }
    return ret;
}

-(void) clean{
    @synchronized (self) {
        self.mutableArray = nil;
    }
}

-(BOOL) empty{
    return self.mutableArray.count == 0;
}

-(NSInteger) count{
    return self.mutableArray.count;
}

-(NSString *)description{
    return _mutableArray.description;
}

@end