/**
 * @file tokenizer.h
 * @author aaaa0ggmc (lovelinux@yslwd.eu.org)
 * @brief 词法分析
 * @version 1.0
 * @date 2026-08-31
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef TOKENIZER_H_INCLUDED
#define TOKENIZER_H_INCLUDED
#include <formatter/string_view.h>
#include <formatter/vector.h>
#include <stdlib.h>
#include <string.h>

enum TokenType {
    T_UNKNOWN,
    T_IDENTIFIER
};

/// 每个Token所处的行号列号
typedef struct {
    int row;
    int col;
} TokenLocation;

typedef struct {
    enum TokenType type;
    // 这里的话即使是比如T_WHILE这种简单的也可以进行存储，因为反正不多占空间
    string_view data;
    TokenLocation location;
} Token;

int parse_tokens(string_view sv, vector * vec);

// 返回未知token
static inline Token token_null(string_view sv){
    Token t = {
        .type = T_UNKNOWN,
        .data = sv_substr(sv,0,0),
        .location = {
            .row = -1,
            .col = -1
        }
    };
    return t;
}


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

// 是否为空白字符
static inline int ch_space(char ch){
    return ch_in_pattern(ch," \t",2) || ch_line_break(ch);
}

// 是否为预处理
static inline int ch_begin_prepocessor(char ch){
    return ch == '#';
}


#endif