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
    X(T_IDENTIFIER), /* 标识符 */ \
    X(T_END_STATEMENT), /* 句子结尾，比如分毫 */ \
    X(T_COMMENT_SINGLE_LINE), \
    X(T_COMMENT_BODY), \
    X(T_BEGIN_COMMENT_MULTIPLE_LINES), \
    X(T_END_COMMENT_MULTIPLE_LINES), \
    X(T_PARENTHESE_BEGIN), /* (*/ \
    X(T_PARENTHESE_END), /* ) */ \
    X(T_BRACE_BEGIN), /* {*/ \
    X(T_BRACE_END), /* } */ \
    X(T_BRACKET_BEGIN), /* [*/ \
    X(T_BRACKET_END), /* ] */ \
    X(T_OP_ADD), /* + */ \
    X(T_OP_MINUS), /* - */ \
    X(T_OP_MUL), /* * */ \
    X(T_OP_DIV), /* / */ \
    X(T_OP_INCREMENT), /* ++ */ \
    X(T_OP_DECREMENT), /* -- */ \
    X(T_OP_ASSIGN), /* = */ \
    X(T_OP_EQ), /* == */ \
    X(T_OP_NE), /* != */ \
    X(T_OP_LT), /* < */ \
    X(T_OP_LE), /* <= */ \
    X(T_OP_GT), /* > */ \
    X(T_OP_GE), /* >= */ \
    X(T_OP_MOD), /* % */ \
    X(T_OP_NOT), /* ! */ \
    X(T_OP_BIT_AND), /* & */ \
    X(T_OP_LOGICAL_AND), /* && */ \
    X(T_OP_BIT_OR), /* | */ \
    X(T_OP_LOGICAL_OR), /* || */ \
    X(T_OP_BIT_XOR), /* ^ */ \
    X(T_OP_BIT_NOT), /* ~ */ \
    X(T_DOT), /* . */ \
    X(T_NUMBER), /* 十进制整数 */ \
    X(T_NUMBER_HEX), /* 十六进制 使用 int*/ \
    X(T_NUMBER_OCT), /* 八进制 */ \
    X(T_NUMBER_BIN), /* 二进制 */ \
    X(T_NUMBER_FLOAT), /* 浮点 */ \
    X(T_UNKNOWN) /* 未知符号，占位 */ 

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