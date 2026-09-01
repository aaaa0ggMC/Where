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

enum TokenType {
    T_IDENTIFIIER
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

#endif