/**
 * @file format.h
 * @author aaaa0ggmc (lovelinux@yslwd.eu.org)
 * @brief 基于抽象语法树的源程序格式化输出
 * @version 1.0
 * @date 2026-09-07
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef FORMAT_H_INCLUDED
#define FORMAT_H_INCLUDED

#include <formatter/ast.h>
#include <stdio.h>

/// 格式化整棵抽象语法树并输出到指定的 FILE 流
void format_ast(Node * root, FILE * out);

/// 格式化整棵抽象语法树并返回动态分配的字符串（调用者需 free）
char * format_ast_to_string(Node * root);

#endif
