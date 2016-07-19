//
//  ViewController.m
//  pushStreamInC
//
//  Created by kaso on 26/5/16.
//  Copyright © 2016年 xcyo. All rights reserved.
//

#import "ViewController.h"
#include "aw_data.h"
#include "aw_dict.h"
#include "aw_file.h"
#include "aw_mp4box.h"
#include "aw_array.h"
#include "aw_parse_mp4.h"
#include "aw_convert_mp4_to_flv.h"

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
    
    [self test];
}

static void write_flvdata_to_file(aw_data *flv_data){
    NSString *documentPath = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
    NSString *flvPath = [documentPath stringByAppendingPathComponent:@"test.flv"];
    
    NSData *flvData = [NSData dataWithBytes:flv_data->data length:flv_data->size];
    
    [flvData writeToFile:flvPath atomically:YES];
}

-(void) test{
    //    aw_data_test();
    //    aw_dict_test();
    
    //test file
    //    aw_test_file("");
    //    NSString *path = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
    //    aw_test_file(path.UTF8String);
    //    
    //    //test mp4 file
    NSString *movPath = [[NSBundle mainBundle] pathForResource:@"test.mov" ofType:nil];
    NSData *data = [NSData dataWithContentsOfFile:movPath];
    
    //    aw_test_mp4_box((void *)data.bytes, (uint32_t)data.length);
    //    aw_parse_mp4_file_data((void *)data.bytes, (uint32_t)data.length);
    //    aw_parse_mp4_test((void *)data.bytes, (uint32_t)data.length);
    
    //    test_aw_array();
    
    aw_convert_mp4_to_flv_test((void *)data.bytes, (uint32_t)data.length, write_flvdata_to_file);
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
