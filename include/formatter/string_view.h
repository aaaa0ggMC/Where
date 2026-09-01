/**
 * @file string_view.h
 * @author aaaa0ggmc (lovelinux@yslwd.eu.org)
 * @brief 简单模拟一下c++的string_view
 * @version 5.0
 * @date 2026-09-01
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef STRING_VIEW_H_INCLUDED
#define STRING_VIEW_H_INCLUDED

// 强行绑定生命周期
typedef struct {
    char ** buffer;

    int begin;
    int length;
} string_view;

/// 这个函数需要保证你给的内容是安全的，内部不会再strlen给你计算一次
string_view sv_build(char ** str, int subbegin , int length);

char * sv_begin(string_view sv);
char * sv_end(string_view sv);
int sv_length(string_view sv);
string_view sv_substr(string_view sv,int begin, int length);
/// 相等返回1, 不相等返回0
int sv_equals(string_view a, string_view b);
void sv_puts(string_view sv);

#endif