//
//  aw_alloc.h
//  pushStreamInC
//
//  Created by kaso on 15/7/16.
//  Copyright © 2016年 xcyo. All rights reserved.
//

#ifndef aw_alloc_h
#define aw_alloc_h

#include <stdio.h>

//可以监视内存的分配和释放，便于调试内存泄漏

//自定义 alloc
extern void * aw_alloc(size_t size);

//自定义 free
extern void aw_free(void *);

//开启debug alloc
extern void aw_init_debug_alloc();

//关闭debug alloc
extern void aw_uninit_debug_alloc();

//返回总共alloc的size
extern size_t aw_total_alloc_size();

//返回总共free的size
extern size_t aw_total_free_size();

//打印内存alloc/free状况
extern void aw_print_alloc_description();

#endif /* aw_alloc_h */
