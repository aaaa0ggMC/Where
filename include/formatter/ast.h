/**
 * @file ast.h
 * @author aaaa0ggmc (lovelinux@yslwd.eu.org)
 * @brief 抽象语法树节点定义
 * @version 1.0
 * @date 2026-09-07
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef AST_H_INCLUDED
#define AST_H_INCLUDED

#include <formatter/tokenizer.h>
#include <stdint.h>
#include <stdbool.h>

/// AST层面的内建类型
typedef enum {
    TY_NONE,
    TY_VOID,
    TY_INT,
    TY_FLOAT,
    TY_CHAR,
    TY_DOUBLE,
    TY_LONG
} BuiltinType;

/// 节点种类（参考 chibicc 设计并针对格式化器简化）
typedef enum {
    ND_NULL_EXPR,     // 空表达式
    ND_ADD,           // +
    ND_SUB,           // -
    ND_MUL,           // *
    ND_DIV,           // /
    ND_MOD,           // %
    ND_BIT_AND,       // &
    ND_BIT_OR,        // |
    ND_BIT_XOR,       // ^
    ND_BIT_NOT,       // ~
    ND_LOGICAL_AND,   // &&
    ND_LOGICAL_OR,    // ||
    ND_NOT,           // !
    ND_EQ,            // ==
    ND_NE,            // !=
    ND_LT,            // <
    ND_LE,            // <=
    ND_GT,            // >
    ND_GE,            // >=
    ND_ASSIGN,        // =
    ND_NEG,           // 负号 -
    ND_POS,           // 正号 +
    ND_PRE_INC,       // 前置 ++
    ND_PRE_DEC,       // 前置 --
    ND_POST_INC,      // 后置 ++
    ND_POST_DEC,      // 后置 --
    ND_NUM,           // 数值常量
    ND_STRING,        // 字符串字面量
    ND_CHAR,          // 字符常量
    ND_VAR,           // 标识符/变量
    ND_COMPOUND_STMT, // { ... } 复合语句块
    ND_EXPR_STMT,     // 表达式语句
    ND_IF,            // if 语句 (cond, then, els)
    ND_WHILE,         // while 循环 (cond, then)
    ND_DO,            // do ... while 循环 (cond, then)
    ND_FOR,           // for 循环 (init, cond, inc, then)
    ND_SWITCH,        // switch 语句 (cond, then)
    ND_CASE,          // case (lhs, then)
    ND_DEFAULT,       // default (then)
    ND_RETURN,        // return 语句 (lhs)
    ND_BREAK,         // break
    ND_CONTINUE,      // continue
    ND_GOTO,          // goto
    ND_LABEL,         // 标签定义
    ND_FUNCALL,       // 函数调用 (args 为实参链表)
    ND_VAR_DECL,      // 变量声明 (类型, 变量名, 可选初值)
    ND_FUNC_DEF,      // 函数定义 (返回值类型, 函数名, 参数, 函数体)
    ND_FUNC_DECL,     // 函数声明 (返回值类型, 函数名, 参数)
    ND_PREPROCESS,    // 预处理指令节点 (#include, #define 等)
    ND_PROGRAM        // 顶层程序节点 (连接外部定义序列)
} NodeKind;

typedef struct Node Node;

struct Node {
    NodeKind kind;
    Node * next;       // 链表指针（语句序列、外部定义序列、形参/实参链表、case链等）
    Token * tok;       // 代表性 Token（用于位置追踪和报错）

    Node * lhs;        // 左子树
    Node * rhs;        // 右子树

    // 控制流语句
    Node * cond;       // 条件表达式
    Node * then;       // 满足条件时的分支或循环体
    Node * els;        // else 分支
    Node * init;       // for 初始化
    Node * inc;        // for 步进递增

    // 块 / 函数体
    Node * body;

    // 变量声明、函数声明/定义
    string_view name;      // 标识符名称
    BuiltinType builtin_type; // AST识别出的内建类型
    string_view type_name; // 原始类型字符串

    // 函数实参或形参列表
    Node * args;

    // Switch / Case
    Node * case_next;
    Node * default_case;

    // 常量值
    long long val;
    long double fval;
    string_view str_val;
};

// 构造函数
Node * new_node(NodeKind kind, Token * tok);
Node * new_binary(NodeKind kind, Node * lhs, Node * rhs, Token * tok);
Node * new_unary(NodeKind kind, Node * expr, Token * tok);
Node * new_num(long long val, Token * tok);
Node * new_var(Token * tok);

// 类型判断 helper（由 AST 层面判别）
BuiltinType check_builtin_type(string_view sv);
const char * builtin_type_name(BuiltinType ty);
const char * node_kind_name(NodeKind kind);

// 树的打印与内存释放
void print_ast(Node * node, int depth);
void ast_free(Node * node);

#endif
