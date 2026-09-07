#include <formatter/ast.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Node * new_node(NodeKind kind, Token * tok){
    Node * node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}

Node * new_binary(NodeKind kind, Node * lhs, Node * rhs, Token * tok){
    Node * node = new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node * new_unary(NodeKind kind, Node * expr, Token * tok){
    Node * node = new_node(kind, tok);
    node->lhs = expr;
    return node;
}

Node * new_num(long long val, Token * tok){
    Node * node = new_node(ND_NUM, tok);
    node->val = val;
    return node;
}

Node * new_float(long double fval, Token * tok){
    Node * node = new_node(ND_NUM, tok);
    node->fval = fval;
    return node;
}

Node * new_var(Token * tok){
    Node * node = new_node(ND_VAR, tok);
    if(tok){
        node->name = tok->data;
    }
    return node;
}

/// 由 AST 层面进行内建类型的判别
BuiltinType check_builtin_type(string_view sv){
    if(sv_equals(sv, sv_build("int", 0, 3))){
        return TY_INT;
    }else if(sv_equals(sv, sv_build("float", 0, 5))){
        return TY_FLOAT;
    }else if(sv_equals(sv, sv_build("char", 0, 4))){
        return TY_CHAR;
    }else if(sv_equals(sv, sv_build("void", 0, 4))){
        return TY_VOID;
    }else if(sv_equals(sv, sv_build("double", 0, 6))){
        return TY_DOUBLE;
    }else if(sv_equals(sv, sv_build("long", 0, 4))){
        return TY_LONG;
    }
    return TY_NONE;
}

const char * builtin_type_name(BuiltinType ty){
    switch(ty){
        case TY_INT: return "int";
        case TY_FLOAT: return "float";
        case TY_CHAR: return "char";
        case TY_VOID: return "void";
        case TY_DOUBLE: return "double";
        case TY_LONG: return "long";
        default: return "unknown";
    }
}

const char * node_kind_name(NodeKind kind){
    switch(kind){
        case ND_NULL_EXPR: return "NULL_EXPR";
        case ND_ADD: return "+";
        case ND_SUB: return "-";
        case ND_MUL: return "*";
        case ND_DIV: return "/";
        case ND_MOD: return "%";
        case ND_BIT_AND: return "&";
        case ND_BIT_OR: return "|";
        case ND_BIT_XOR: return "^";
        case ND_BIT_NOT: return "~";
        case ND_LOGICAL_AND: return "&&";
        case ND_LOGICAL_OR: return "||";
        case ND_NOT: return "!";
        case ND_EQ: return "==";
        case ND_NE: return "!=";
        case ND_LT: return "<";
        case ND_LE: return "<=";
        case ND_GT: return ">";
        case ND_GE: return ">=";
        case ND_ASSIGN: return "=";
        case ND_NEG: return "NEG(-)";
        case ND_POS: return "POS(+)";
        case ND_PRE_INC: return "PRE_++";
        case ND_PRE_DEC: return "PRE_--";
        case ND_POST_INC: return "POST_++";
        case ND_POST_DEC: return "POST_--";
        case ND_NUM: return "NUM";
        case ND_STRING: return "STRING";
        case ND_CHAR: return "CHAR";
        case ND_VAR: return "VAR";
        case ND_COMPOUND_STMT: return "COMPOUND_STMT";
        case ND_EXPR_STMT: return "EXPR_STMT";
        case ND_IF: return "IF";
        case ND_WHILE: return "WHILE";
        case ND_DO: return "DO_WHILE";
        case ND_FOR: return "FOR";
        case ND_SWITCH: return "SWITCH";
        case ND_CASE: return "CASE";
        case ND_DEFAULT: return "DEFAULT";
        case ND_RETURN: return "RETURN";
        case ND_BREAK: return "BREAK";
        case ND_CONTINUE: return "CONTINUE";
        case ND_GOTO: return "GOTO";
        case ND_LABEL: return "LABEL";
        case ND_FUNCALL: return "FUNCALL";
        case ND_VAR_DECL: return "VAR_DECL";
        case ND_FUNC_DEF: return "FUNC_DEF";
        case ND_FUNC_DECL: return "FUNC_DECL";
        case ND_PREPROCESS: return "PREPROCESS";
        case ND_PROGRAM: return "PROGRAM";
    }
    return "?";
}

static void print_indent(int depth){
    for(int i = 0;i < depth;++i){
        printf("  ");
    }
}

/// 先根遍历打印抽象语法树
void print_ast(Node * node, int depth){
    if(!node) return;

    print_indent(depth);
    printf("[%s]", node_kind_name(node->kind));

    switch(node->kind){
        case ND_NUM:
            if(node->tok && node->tok->type == T_NUMBER_FLOAT){
                printf(" %.*s\n", sv_length(node->tok->data), sv_begin(node->tok->data));
            }else{
                printf(" %lld\n", node->val);
            }
            break;
        case ND_STRING:
        case ND_CHAR:
            printf(" \"%.*s\"\n", sv_length(node->str_val), sv_begin(node->str_val));
            break;
        case ND_VAR:
            printf(" %.*s\n", sv_length(node->name), sv_begin(node->name));
            break;
        case ND_VAR_DECL:
            printf(" type: %.*s name: %.*s\n", 
                sv_length(node->type_name), sv_begin(node->type_name),
                sv_length(node->name), sv_begin(node->name));
            if(node->lhs){
                print_indent(depth + 1);
                printf("init:\n");
                print_ast(node->lhs, depth + 2);
            }
            break;
        case ND_FUNC_DEF:
        case ND_FUNC_DECL:
            printf(" %s %.*s (name: %.*s)\n",
                node->kind == ND_FUNC_DEF ? "DEF" : "DECL",
                sv_length(node->type_name), sv_begin(node->type_name),
                sv_length(node->name), sv_begin(node->name));
            if(node->args){
                print_indent(depth + 1);
                printf("params:\n");
                for(Node * a = node->args; a; a = a->next){
                    print_ast(a, depth + 2);
                }
            }
            if(node->body){
                print_indent(depth + 1);
                printf("body:\n");
                print_ast(node->body, depth + 2);
            }
            break;
        case ND_FUNCALL:
            printf(" %.*s()\n", sv_length(node->name), sv_begin(node->name));
            if(node->args){
                print_indent(depth + 1);
                printf("args:\n");
                for(Node * a = node->args; a; a = a->next){
                    print_ast(a, depth + 2);
                }
            }
            break;
        case ND_IF:
            printf("\n");
            print_indent(depth + 1);
            printf("cond:\n");
            print_ast(node->cond, depth + 2);
            print_indent(depth + 1);
            printf("then:\n");
            print_ast(node->then, depth + 2);
            if(node->els){
                print_indent(depth + 1);
                printf("else:\n");
                print_ast(node->els, depth + 2);
            }
            break;
        case ND_WHILE:
        case ND_DO:
            printf("\n");
            print_indent(depth + 1);
            printf("cond:\n");
            print_ast(node->cond, depth + 2);
            print_indent(depth + 1);
            printf("body:\n");
            print_ast(node->then, depth + 2);
            break;
        case ND_FOR:
            printf("\n");
            if(node->init){
                print_indent(depth + 1);
                printf("init:\n");
                print_ast(node->init, depth + 2);
            }
            if(node->cond){
                print_indent(depth + 1);
                printf("cond:\n");
                print_ast(node->cond, depth + 2);
            }
            if(node->inc){
                print_indent(depth + 1);
                printf("inc:\n");
                print_ast(node->inc, depth + 2);
            }
            if(node->then){
                print_indent(depth + 1);
                printf("body:\n");
                print_ast(node->then, depth + 2);
            }
            break;
        case ND_SWITCH:
            printf("\n");
            print_indent(depth + 1);
            printf("cond:\n");
            print_ast(node->cond, depth + 2);
            print_indent(depth + 1);
            printf("body:\n");
            print_ast(node->then, depth + 2);
            break;
        case ND_CASE:
            printf("\n");
            if(node->lhs){
                print_indent(depth + 1);
                printf("val:\n");
                print_ast(node->lhs, depth + 2);
            }
            if(node->then){
                print_ast(node->then, depth + 1);
            }
            break;
        case ND_DEFAULT:
            printf("\n");
            if(node->then){
                print_ast(node->then, depth + 1);
            }
            break;
        case ND_COMPOUND_STMT:
            printf("\n");
            for(Node * cur = node->body; cur; cur = cur->next){
                print_ast(cur, depth + 1);
            }
            break;
        case ND_PROGRAM:
            printf("\n");
            for(Node * cur = node->body; cur; cur = cur->next){
                print_ast(cur, depth + 1);
            }
            break;
        case ND_PREPROCESS:
            if(node->tok){
                printf(" %.*s", sv_length(node->tok->data), sv_begin(node->tok->data));
            }
            if(node->lhs && node->lhs->tok){
                printf(" %.*s", sv_length(node->lhs->tok->data), sv_begin(node->lhs->tok->data));
            }
            if(node->rhs && node->rhs->tok){
                printf(" %.*s", sv_length(node->rhs->tok->data), sv_begin(node->rhs->tok->data));
            }
            printf("\n");
            break;
        case ND_RETURN:
            printf("\n");
            if(node->lhs){
                print_ast(node->lhs, depth + 1);
            }
            break;
        case ND_EXPR_STMT:
            printf("\n");
            if(node->lhs){
                print_ast(node->lhs, depth + 1);
            }
            break;
        default:
            printf("\n");
            if(node->lhs) print_ast(node->lhs, depth + 1);
            if(node->rhs) print_ast(node->rhs, depth + 1);
            break;
    }
}

/// 递归释放 AST 节点
void ast_free(Node * node){
    if(!node) return;

    // 先递归释放链表后继
    if(node->next) ast_free(node->next);

    // 释放子节点
    if(node->lhs) ast_free(node->lhs);
    if(node->rhs) ast_free(node->rhs);
    if(node->cond) ast_free(node->cond);
    if(node->then) ast_free(node->then);
    if(node->els) ast_free(node->els);
    if(node->init) ast_free(node->init);
    if(node->inc) ast_free(node->inc);
    if(node->body) ast_free(node->body);
    if(node->args) ast_free(node->args);

    free(node);
}
