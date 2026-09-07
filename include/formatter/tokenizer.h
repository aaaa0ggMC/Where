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

#include <formatter/tokenizer/types.h>
#include <formatter/tokenizer/rules.h>

#include <stdlib.h>
#include <string.h>

enum TokenIValType{
    I_VOID,
    I_INT,
    I_LONG,
    I_UINT,
    I_ULONG,
    I_LONG_LONG,
    I_ULONG_LONG,
    I_DOUBLE,
    I_FLOAT,
    I_LONG_DOUBLE
};


typedef struct {
    enum TokenType type;
    // 这里的话即使是比如T_WHILE这种简单的也可以进行存储，因为反正不多占空间
    string_view data;
    TokenLocation location;

    // 这里是解析出来的内建常量，用什么类型存储由 itype 决定
    enum TokenIValType itype;
    union {
        unsigned long long ull;
        long long ll;
        long double ld;
    } ival;
} Token;

typedef struct {
    int success;
    // 报错信息
    stage_diagnoses diagnoses;
} tokenizer_result;

tokenizer_result parse_tokens(string_view sv, vector * vec);
void terminate_tokenizer();


// 返回未知token
static inline Token token_null(string_view sv){
    Token t = {
        .type = T_UNKNOWN,
        .data = sv_substr(sv,0,0),
        .location = {
            .row = -1,
            .col = -1
        },
        .itype = I_VOID,
        .ival = {
            .ld = 0
        }
    };
    return t;
}

#endif