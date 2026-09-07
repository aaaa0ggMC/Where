/**
 * @file parser.h
 * @author aaaa0ggmc (lovelinux@yslwd.eu.org)
 * @brief 语法分析器 (基于 chibicc 递归下降模型)
 * @version 1.0
 * @date 2026-09-07
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include <formatter/tokenizer.h>
#include <formatter/ast.h>
#include <formatter/diagnoses.h>

/// 语法分析总入口，返回 AST 根节点
Node * parse_ast(Token * tok, stage_diagnoses * diagnoses);

#endif
