#include <formatter/format.h>
#include <formatter/ast.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// 缩进输出：每层 4 个空格
static void write_indent(FILE * out, int depth){
    for(int i = 0;i < depth;++i){
        fputs("    ", out);
    }
}

// 声明语句和表达式格式化函数
static void format_expr(Node * node, FILE * out);
static void format_stmt(Node * node, FILE * out, int depth);

/// 表达式格式化
static void format_expr(Node * node, FILE * out){
    if(!node) return;

    switch(node->kind){
        case ND_NUM:
            if(node->tok && (node->tok->type == T_NUMBER_FLOAT)){
                fprintf(out, "%Lg", node->fval);
            }else{
                fprintf(out, "%lld", node->val);
            }
            break;

        case ND_VAR:
            fprintf(out, "%.*s", sv_length(node->name), sv_begin(node->name));
            break;

        case ND_STRING:
        case ND_CHAR:
            fprintf(out, "%.*s", sv_length(node->str_val), sv_begin(node->str_val));
            break;

        case ND_ASSIGN:
            format_expr(node->lhs, out);
            fputs(" = ", out);
            format_expr(node->rhs, out);
            break;

        case ND_ADD:
        case ND_SUB:
        case ND_MUL:
        case ND_DIV:
        case ND_MOD:
        case ND_EQ:
        case ND_NE:
        case ND_LT:
        case ND_LE:
        case ND_GT:
        case ND_GE:
        case ND_LOGICAL_AND:
        case ND_LOGICAL_OR:
        case ND_BIT_AND:
        case ND_BIT_OR:
        case ND_BIT_XOR: {
            format_expr(node->lhs, out);
            fprintf(out, " %s ", node_kind_name(node->kind));
            format_expr(node->rhs, out);
            break;
        }

        case ND_POS:
            fputc('+', out);
            format_expr(node->lhs, out);
            break;

        case ND_NEG:
            fputc('-', out);
            format_expr(node->lhs, out);
            break;

        case ND_NOT:
            fputc('!', out);
            format_expr(node->lhs, out);
            break;

        case ND_BIT_NOT:
            fputc('~', out);
            format_expr(node->lhs, out);
            break;

        case ND_PRE_INC:
            fputs("++", out);
            format_expr(node->lhs, out);
            break;

        case ND_PRE_DEC:
            fputs("--", out);
            format_expr(node->lhs, out);
            break;

        case ND_POST_INC:
            format_expr(node->lhs, out);
            fputs("++", out);
            break;

        case ND_POST_DEC:
            format_expr(node->lhs, out);
            fputs("--", out);
            break;

        case ND_FUNCALL:
            fprintf(out, "%.*s(", sv_length(node->name), sv_begin(node->name));
            for(Node * a = node->args; a; a = a->next){
                format_expr(a, out);
                if(a->next) fputs(", ", out);
            }
            fputc(')', out);
            break;

        default:
            if(node->lhs) format_expr(node->lhs, out);
            if(node->rhs) format_expr(node->rhs, out);
            break;
    }
}

/// 语句格式化
static void format_stmt(Node * node, FILE * out, int depth){
    if(!node) return;

    switch(node->kind){
        case ND_PREPROCESS:
            // 预处理指令无论出于哪个深度均不缩进
            if(node->tok){
                fprintf(out, "%.*s", sv_length(node->tok->data), sv_begin(node->tok->data));
            }
            if(node->lhs && node->lhs->tok){
                fprintf(out, " %.*s", sv_length(node->lhs->tok->data), sv_begin(node->lhs->tok->data));
            }
            if(node->rhs && node->rhs->tok){
                fprintf(out, " %.*s", sv_length(node->rhs->tok->data), sv_begin(node->rhs->tok->data));
            }
            fputc('\n', out);
            break;

        case ND_VAR_DECL:
            write_indent(out, depth);
            fprintf(out, "%.*s %.*s", sv_length(node->type_name), sv_begin(node->type_name),
                    sv_length(node->name), sv_begin(node->name));
            if(node->lhs){
                fputs(" = ", out);
                format_expr(node->lhs, out);
            }
            // 同一行的同类型连续变量声明合并（如 int i, j;）
            while(node->next && node->next->kind == ND_VAR_DECL &&
                  sv_equals(node->next->type_name, node->type_name)){
                node = node->next;
                fprintf(out, ", %.*s", sv_length(node->name), sv_begin(node->name));
                if(node->lhs){
                    fputs(" = ", out);
                    format_expr(node->lhs, out);
                }
            }
            fputs(";\n", out);
            break;

        case ND_COMPOUND_STMT:
            write_indent(out, depth);
            fputs("{\n", out);
            for(Node * cur = node->body; cur; cur = cur->next){
                format_stmt(cur, out, depth + 1);
                // 跳过已在 format_stmt 中合并的连续变量声明
                if(cur->kind == ND_VAR_DECL){
                    while(cur->next && cur->next->kind == ND_VAR_DECL &&
                          sv_equals(cur->next->type_name, cur->type_name)){
                        cur = cur->next;
                    }
                }
            }
            write_indent(out, depth);
            fputs("}\n", out);
            break;

        case ND_EXPR_STMT:
            write_indent(out, depth);
            if(node->lhs) format_expr(node->lhs, out);
            fputs(";\n", out);
            break;

        case ND_IF:
            write_indent(out, depth);
            fputs("if (", out);
            format_expr(node->cond, out);
            fputs(")", out);

            if(node->then && node->then->kind == ND_COMPOUND_STMT){
                fputs(" {\n", out);
                for(Node * cur = node->then->body; cur; cur = cur->next){
                    format_stmt(cur, out, depth + 1);
                    if(cur->kind == ND_VAR_DECL){
                        while(cur->next && cur->next->kind == ND_VAR_DECL &&
                              sv_equals(cur->next->type_name, cur->type_name)){
                            cur = cur->next;
                        }
                    }
                }
                write_indent(out, depth);
                fputs("}", out);
            }else{
                fputc('\n', out);
                format_stmt(node->then, out, depth + 1);
            }

            if(node->els){
                if(node->then && node->then->kind == ND_COMPOUND_STMT){
                    fputs(" else", out);
                }else{
                    write_indent(out, depth);
                    fputs("else", out);
                }

                if(node->els->kind == ND_COMPOUND_STMT){
                    fputs(" {\n", out);
                    for(Node * cur = node->els->body; cur; cur = cur->next){
                        format_stmt(cur, out, depth + 1);
                        if(cur->kind == ND_VAR_DECL){
                            while(cur->next && cur->next->kind == ND_VAR_DECL &&
                                  sv_equals(cur->next->type_name, cur->type_name)){
                                cur = cur->next;
                            }
                        }
                    }
                    write_indent(out, depth);
                    fputs("}\n", out);
                }else if(node->els->kind == ND_IF){
                    fputc(' ', out);
                    // else if 同一行且不加额外缩进
                    format_stmt(node->els, out, depth);
                }else{
                    fputc('\n', out);
                    format_stmt(node->els, out, depth + 1);
                }
            }else{
                if(node->then && node->then->kind == ND_COMPOUND_STMT){
                    fputc('\n', out);
                }
            }
            break;

        case ND_WHILE:
            write_indent(out, depth);
            fputs("while (", out);
            format_expr(node->cond, out);
            fputs(")", out);

            if(node->then && node->then->kind == ND_COMPOUND_STMT){
                fputs(" {\n", out);
                for(Node * cur = node->then->body; cur; cur = cur->next){
                    format_stmt(cur, out, depth + 1);
                }
                write_indent(out, depth);
                fputs("}\n", out);
            }else{
                fputc('\n', out);
                format_stmt(node->then, out, depth + 1);
            }
            break;

        case ND_DO:
            write_indent(out, depth);
            fputs("do", out);
            if(node->then && node->then->kind == ND_COMPOUND_STMT){
                fputs(" {\n", out);
                for(Node * cur = node->then->body; cur; cur = cur->next){
                    format_stmt(cur, out, depth + 1);
                }
                write_indent(out, depth);
                fputs("} while (", out);
            }else{
                fputc('\n', out);
                format_stmt(node->then, out, depth + 1);
                write_indent(out, depth);
                fputs("while (", out);
            }
            format_expr(node->cond, out);
            fputs(");\n", out);
            break;

        case ND_FOR:
            write_indent(out, depth);
            fputs("for (", out);
            if(node->init){
                if(node->init->kind == ND_VAR_DECL){
                    fprintf(out, "%.*s %.*s", sv_length(node->init->type_name), sv_begin(node->init->type_name),
                            sv_length(node->init->name), sv_begin(node->init->name));
                    if(node->init->lhs){
                        fputs(" = ", out);
                        format_expr(node->init->lhs, out);
                    }
                    fputs("; ", out);
                }else if(node->init->kind == ND_EXPR_STMT){
                    if(node->init->lhs) format_expr(node->init->lhs, out);
                    fputs("; ", out);
                }
            }else{
                fputs("; ", out);
            }

            if(node->cond){
                format_expr(node->cond, out);
            }
            fputs("; ", out);

            if(node->inc){
                format_expr(node->inc, out);
            }
            fputs(")", out);

            if(node->then && node->then->kind == ND_COMPOUND_STMT){
                fputs(" {\n", out);
                for(Node * cur = node->then->body; cur; cur = cur->next){
                    format_stmt(cur, out, depth + 1);
                }
                write_indent(out, depth);
                fputs("}\n", out);
            }else{
                fputc('\n', out);
                format_stmt(node->then, out, depth + 1);
            }
            break;

        case ND_SWITCH:
            write_indent(out, depth);
            fputs("switch (", out);
            format_expr(node->cond, out);
            fputs(")", out);
            if(node->then && node->then->kind == ND_COMPOUND_STMT){
                fputs(" {\n", out);
                for(Node * cur = node->then->body; cur; cur = cur->next){
                    format_stmt(cur, out, depth + 1);
                }
                write_indent(out, depth);
                fputs("}\n", out);
            }else{
                fputc('\n', out);
                format_stmt(node->then, out, depth + 1);
            }
            break;

        case ND_CASE:
            write_indent(out, depth);
            fputs("case ", out);
            format_expr(node->lhs, out);
            fputs(":\n", out);
            if(node->then) format_stmt(node->then, out, depth + 1);
            break;

        case ND_DEFAULT:
            write_indent(out, depth);
            fputs("default:\n", out);
            if(node->then) format_stmt(node->then, out, depth + 1);
            break;

        case ND_RETURN:
            write_indent(out, depth);
            fputs("return", out);
            if(node->lhs){
                fputc(' ', out);
                format_expr(node->lhs, out);
            }
            fputs(";\n", out);
            break;

        case ND_BREAK:
            write_indent(out, depth);
            fputs("break;\n", out);
            break;

        case ND_CONTINUE:
            write_indent(out, depth);
            fputs("continue;\n", out);
            break;

        case ND_GOTO:
            write_indent(out, depth);
            fprintf(out, "goto %.*s;\n", sv_length(node->name), sv_begin(node->name));
            break;

        case ND_FUNC_DEF:
            write_indent(out, depth);
            fprintf(out, "%.*s %.*s(", sv_length(node->type_name), sv_begin(node->type_name),
                    sv_length(node->name), sv_begin(node->name));
            for(Node * p = node->args; p; p = p->next){
                fprintf(out, "%.*s %.*s", sv_length(p->type_name), sv_begin(p->type_name),
                        sv_length(p->name), sv_begin(p->name));
                if(p->next) fputs(", ", out);
            }
            fputs(")\n", out);
            format_stmt(node->body, out, depth);
            break;

        case ND_FUNC_DECL:
            write_indent(out, depth);
            fprintf(out, "%.*s %.*s(", sv_length(node->type_name), sv_begin(node->type_name),
                    sv_length(node->name), sv_begin(node->name));
            for(Node * p = node->args; p; p = p->next){
                fprintf(out, "%.*s %.*s", sv_length(p->type_name), sv_begin(p->type_name),
                        sv_length(p->name), sv_begin(p->name));
                if(p->next) fputs(", ", out);
            }
            fputs(");\n", out);
            break;

        default:
            break;
    }
}

/// 格式化整棵抽象语法树
void format_ast(Node * root, FILE * out){
    if(!root || !out) return;

    if(root->kind == ND_PROGRAM){
        for(Node * cur = root->body; cur; cur = cur->next){
            format_stmt(cur, out, 0);
            // 跳过已在 format_stmt 中合并的连续变量声明
            if(cur->kind == ND_VAR_DECL){
                while(cur->next && cur->next->kind == ND_VAR_DECL &&
                      sv_equals(cur->next->type_name, cur->type_name)){
                    cur = cur->next;
                }
            }
        }
    }else{
        format_stmt(root, out, 0);
    }
}

char * format_ast_to_string(Node * root){
    char * buffer = NULL;
    size_t size = 0;
    FILE * mem = open_memstream(&buffer, &size);
    if(!mem) return NULL;

    format_ast(root, mem);
    fclose(mem);
    return buffer;
}
