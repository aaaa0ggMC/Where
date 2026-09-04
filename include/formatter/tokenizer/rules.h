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

// 点号
static inline int ch_dot(char ch){
    return ch == '.';
}

// 是否为注释开头 以及除法
static inline int ch_operand_div(char ch){
    return ch == '/';
}

// 是否是加法开头
static inline int ch_operand_add(char ch){
    return ch == '+';
}

// 减法
static inline int ch_operand_minus(char ch){
    return ch == '-';
}

// 乘法
static inline int ch_operand_mul(char ch){
    return ch == '*';
}

// 赋值 与 比较/逻辑/位运算相关的运算符字符
static inline int ch_operand_assign(char ch){
    return ch == '=';
}
static inline int ch_operand_not(char ch){
    return ch == '!';
}
static inline int ch_operand_lt(char ch){
    return ch == '<';
}
static inline int ch_operand_gt(char ch){
    return ch == '>';
}
static inline int ch_operand_mod(char ch){
    return ch == '%';
}
static inline int ch_operand_and(char ch){
    return ch == '&';
}
static inline int ch_operand_or(char ch){
    return ch == '|';
}
static inline int ch_operand_xor(char ch){
    return ch == '^';
}
static inline int ch_operand_bnot(char ch){
    return ch == '~';
}

// 括号
static inline int ch_parentheses_begin(char ch){
    return ch == '(';
}

static inline int ch_parentheses_end(char ch){
    return ch == ')';
}

// Block
static inline int ch_brace_begin(char ch){
    return ch == '{';
}

static inline int ch_brace_end(char ch){
    return ch == '}';
}

// 中括号
static inline int ch_bracket_begin(char ch){
    return ch == '[';
}

static inline int ch_bracket_end(char ch){
    return ch == ']';
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

static inline int ch_number_sep(char ch){
    return ch == '\'';
}

// 8 base
static inline int ch_number_base_8(char ch){
    return ch_in_range(ch, '0', '7');
}

// 某进制下字符对应的数值，非法返回-1，分隔符校验会用到它
static inline int ch_number_value(char ch, int base){
    int value;
    if(ch_digit(ch)){
        value = ch - '0';
    }else if(base == 16){
        if(ch_in_range(ch, 'a', 'f')){
            value = ch - 'a' + 10;
        }else if(ch_in_range(ch, 'A', 'F')){
            value = ch - 'A' + 10;
        }else{
            return -1;
        }
    }else{
        return -1;
    }

    return value < base ? value : -1;
}
// 16 base
static inline int ch_number_base_16_clue(char ch){
    return ch == 'x' || ch == 'X';
}
static inline int ch_number_base_16(char ch){
    return ch_in_range(ch, '0', '9') ||
        ch_in_range(ch,'a', 'f')     ||
        ch_in_range(ch,'A', 'F');
}
// 2 base
static inline int ch_number_base_2_clue(char ch){
    return ch == 'b' || ch == 'B';
}
static inline int ch_number_base_2(char ch){
    return ch == '0' || ch == '1';
}

// 数字后缀字符（其组合和具体类型见 scan_number_suffix）
static inline int ch_number_suffix_unsigned(char ch){
    return ch == 'u' || ch == 'U';
}
static inline int ch_number_suffix_long(char ch){
    return ch == 'l' || ch == 'L';
}
static inline int ch_number_suffix_float(char ch){
    return ch == 'f' || ch == 'F';
}

#endif