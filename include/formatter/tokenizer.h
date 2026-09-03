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
#include <formatter/diagnoses.h>
#include <stdlib.h>
#include <string.h>

#define TOKEN_LIST(X) \
    X(T_UNKNOWN), /* 未知符号，占位 */ \
    X(T_IDENTIFIER), /* 标识符 */ \
    X(T_END_STATEMENT), /* 句子结尾，比如分毫 */ \
    X(T_RECOVERY) /* 处于恢复模式的符号，占位 */

#define XTOKEN(X) X
enum TokenType {
    TOKEN_LIST(XTOKEN)
};
#undef XTOKEN

#define XTOKEN(X) #X
static const char* token_strings[] = {
    TOKEN_LIST(XTOKEN)
};
#undef XTOKEN

const char * token_string(enum TokenType type);
#undef TOKEN_LIST

typedef struct {
    enum TokenType type;
    // 这里的话即使是比如T_WHILE这种简单的也可以进行存储，因为反正不多占空间
    string_view data;
    TokenLocation location;
} Token;

typedef struct {
    int success;
    // 报错信息
    stage_diagnoses diagnoses;
} tokenizer_result;

tokenizer_result parse_tokens(string_view sv, vector * vec);

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

#endif