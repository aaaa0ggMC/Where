/**
 * @file rules.h
 * @author aaaa0ggmc (lovelinux@yslwd.eu.org)
 * @brief 一些定义
 * @version 5.0
 * @date 2026-09-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef TOKENIZER_RULES_H_INCLUDED
#define TOKENIZER_RULES_H_INCLUDED
#include <stdlib.h>
#include <string.h>

// 下面是一些判断函数，使用inline，使用ch开头是为了防止和C库冲突
static inline int ch_in_range(char ch,char beg,char end){
    return ch >= beg && ch <= end;
}

// 使用-1则使用strlen返回
static inline int ch_in_pattern(char ch,char * pattern,int size){
    for(int i = 0;i < (size>=0 ? size : strlen(pattern)); ++i){
        if(pattern[i] == ch)return 1;
    }
    return 0;
}


// 是不是可以开始构成identifier的字符集
static inline int ch_token_begin(char ch){
    return ch_in_range(ch,'a','z') ||
        ch_in_range(ch,'A','Z')    ||
        ch_in_pattern(ch,"$_",2);
}

// 10进制数字
static inline int ch_digit(char ch){
    return ch_in_range(ch,'0','9');
}

// ident中间部分应该满足
static inline int ch_token_middle(char ch){
    return ch_token_begin(ch) || ch_digit(ch);
}

// 是否是换行符
static inline int ch_line_break(char ch){
    return (ch == '\n');
}

// 是否结束了
static inline int ch_eof(char ch){
    return ch == '\0';
}

// 是否为空白字符
static inline int ch_space(char ch){
    return ch_in_pattern(ch," \t",2) || ch_line_break(ch);
}

// 是否为预处理
static inline int ch_begin_preprocessor(char ch){
    return ch == '#';
}

// 是否为语句结束
static inline int ch_statement_end(char ch){
    return ch == ';';
}

// 是否为注释开头 以及除法
static inline int ch_operand_div(char ch){
    return ch == '/';
}

// 具体到每种注释判别
static inline int ch_comment_2_single_line(char ch){
    return ch == '/';
}

// 单行注释续杯，类似这里 \
哈哈哈
static inline int ch_comment_single_line_renew(char ch){
    return ch == '\\';
}

static inline int ch_comment_2_multiple_lines(char ch){
    return ch == '*';
}

#endif