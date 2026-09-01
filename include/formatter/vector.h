/**
 * @file vector.h
 * @author aaaa0ggmc (lovelinux@yslwd.eu.org)
 * @brief 简单的子扩容线性表（语法分析可能会常常调用&回溯啥的，因此我使用顺序存储）
 *   部分功能不会实现，因为tokenizer用不到
 * @version 5.0
 * @date 2026-09-01
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef VECTOR_H_INCLUDED
#define VECTOR_H_INCLUDED

/// 自己解析类型？ 内部是void *
/// 然后便是这个结构不支持共享，如果共享了很可能会损害结构
typedef struct {
    void * data;
    int sizeof_data;

    int size;
    int capacity;
} vector;

vector vec_new(int sizeof_data, int reserve_size);
void* vec_push_back(vector * vec);
void vec_delete(vector * vec);
void* vec_begin(vector * vec);
void* vec_end(vector * vec);
void * vec_next(vector * vec,void* data);

#endif