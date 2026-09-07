/**
 * @file parser.h
 * @author aaaa0ggmc (lovelinux@yslwd.eu.org)
 * @brief 语法分析器
 * @version 1.0
 * @date 2026-09-07
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include <formatter/ast.h>
#include <formatter/tokenizer.h>
#include <formatter/diagnoses.h>

/// 语法分析结果（值传递返回，所有权转移类似于 C++ move）
typedef struct {
    int success;
    Node * root;
    stage_diagnoses diagnoses;
} parser_result;

/// 语法分析总入口，解析 tokens 并构建 AST
parser_result parse_ast(vector * tokens);

/// 销毁语法分析结果（释放 diagnoses 与 AST 树内存）
void parser_result_delete(parser_result * res);

#endif
