/**
 * @file defines.h
 * @author aaaa0ggmc (lovelinux@yslwd.eu.org)
 * @brief 具体到每个token
 * @version 5.0
 * @date 2026-09-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef TOKENIZER_DEFINES_H_INCLUDED
#define TOKENIZER_DEFINES_H_INCLUDED

#define TOKEN_LIST(X) \
    X(T_UNKNOWN), /* 未知符号，占位 */ \
    X(T_IDENTIFIER), /* 标识符 */ \
    X(T_END_STATEMENT), /* 句子结尾，比如分毫 */ \
    X(T_COMMENT_SINGLE_LINE), \
    X(T_COMMENT_BODY), \
    X(T_BEGIN_COMMENT_MULTIPLE_LINES), \
    X(T_END_COMMENT_MULTIPLE_LINES)

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

#endif