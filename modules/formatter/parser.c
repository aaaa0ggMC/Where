#include <formatter/parser.h>
#include <formatter/ast.h>
#include <formatter/tokenizer.h>
#include <formatter/diagnoses.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/// 辅助比较函数（参考 chibicc 设计）
static inline int equal(Token * tok, const char * op){
    if(!tok) return 0;
    int len = strlen(op);
    if(sv_length(tok->data) != len) return 0;
    return memcmp(sv_begin(tok->data), op, len) == 0;
}

static inline int consume(Token ** rest, Token * tok, const char * op){
    if(equal(tok, op)){
        *rest = tok->next;
        return 1;
    }
    *rest = tok;
    return 0;
}

static Token * skip(Token ** rest, Token * tok, const char * op, stage_diagnoses * diagnoses){
    if(!equal(tok, op)){
        if(tok){
            sd_printf(diagnoses, "Expected '%s' at row %d col %d, but got \"%.*s\"",
                op, tok->location.row, tok->location.col,
                sv_length(tok->data), sv_begin(tok->data));
        }else{
            sd_printf(diagnoses, "Expected '%s', but reached EOF", op);
        }
        *rest = tok ? tok->next : NULL;
        return *rest;
    }
    *rest = tok->next;
    return tok->next;
}

/// AST层面判别内置类型
static inline int is_typename(Token * tok){
    if(!tok) return 0;
    if(tok->type != T_IDENTIFIER) return 0;
    return check_builtin_type(tok->data) != TY_NONE;
}

static inline int is_preprocessor(Token * tok){
    if(!tok) return 0;
    return (tok->type == T_PP_INCLUDE ||
            tok->type == T_PP_DEFINE ||
            tok->type == T_PP_UNDEF ||
            tok->type == T_PP_IF ||
            tok->type == T_PP_IFDEF ||
            tok->type == T_PP_IFNDEF ||
            tok->type == T_PP_ELSE ||
            tok->type == T_PP_ELIF ||
            tok->type == T_PP_ENDIF);
}

// 声明递归下降函数
static Node * expr(Token ** rest, Token * tok, stage_diagnoses * diagnoses);
static Node * assign(Token ** rest, Token * tok, stage_diagnoses * diagnoses);
static Node * logor(Token ** rest, Token * tok, stage_diagnoses * diagnoses);
static Node * logand(Token ** rest, Token * tok, stage_diagnoses * diagnoses);
static Node * bitor(Token ** rest, Token * tok, stage_diagnoses * diagnoses);
static Node * bitxor(Token ** rest, Token * tok, stage_diagnoses * diagnoses);
static Node * bitand(Token ** rest, Token * tok, stage_diagnoses * diagnoses);
static Node * equality(Token ** rest, Token * tok, stage_diagnoses * diagnoses);
static Node * relational(Token ** rest, Token * tok, stage_diagnoses * diagnoses);
static Node * add(Token ** rest, Token * tok, stage_diagnoses * diagnoses);
static Node * mul(Token ** rest, Token * tok, stage_diagnoses * diagnoses);
static Node * unary(Token ** rest, Token * tok, stage_diagnoses * diagnoses);
static Node * postfix(Token ** rest, Token * tok, stage_diagnoses * diagnoses);
static Node * primary(Token ** rest, Token * tok, stage_diagnoses * diagnoses);

static Node * stmt(Token ** rest, Token * tok, stage_diagnoses * diagnoses);
static Node * compound_stmt(Token ** rest, Token * tok, stage_diagnoses * diagnoses);
static Node * expr_stmt(Token ** rest, Token * tok, stage_diagnoses * diagnoses);
static Node * declaration(Token ** rest, Token * tok, stage_diagnoses * diagnoses);

/// primary = "(" expr ")" | NUM | STRING | CHAR | IDENT
static Node * primary(Token ** rest, Token * tok, stage_diagnoses * diagnoses){
    if(!tok){
        *rest = NULL;
        return NULL;
    }

    if(equal(tok, "(")){
        Node * node = expr(&tok, tok->next, diagnoses);
        tok = skip(&tok, tok, ")", diagnoses);
        *rest = tok;
        return node;
    }

    if(tok->type == T_NUMBER || tok->type == T_NUMBER_HEX ||
       tok->type == T_NUMBER_OCT || tok->type == T_NUMBER_BIN ||
       tok->type == T_NUMBER_FLOAT){
        Node * node = new_num(tok->ival.ull, tok);
        node->fval = tok->ival.ld;
        *rest = tok->next;
        return node;
    }

    if(tok->type == T_STRING_LITERAL){
        Node * node = new_node(ND_STRING, tok);
        node->str_val = tok->data;
        *rest = tok->next;
        return node;
    }

    if(tok->type == T_CHAR_LITERAL){
        Node * node = new_node(ND_CHAR, tok);
        node->str_val = tok->data;
        *rest = tok->next;
        return node;
    }

    if(tok->type == T_IDENTIFIER){
        Node * node = new_var(tok);
        *rest = tok->next;
        return node;
    }

    sd_printf(diagnoses, "Unexpected token \"%.*s\" at row %d col %d in expression",
        sv_length(tok->data), sv_begin(tok->data),
        tok->location.row, tok->location.col);
    *rest = tok->next;
    return NULL;
}

/// postfix = primary ("(" func-args? ")" | "[" expr "]" | "++" | "--")*
static Node * postfix(Token ** rest, Token * tok, stage_diagnoses * diagnoses){
    Node * node = primary(&tok, tok, diagnoses);

    while(tok){
        if(equal(tok, "(")){
            // 函数调用
            Node * call = new_node(ND_FUNCALL, tok);
            if(node && node->kind == ND_VAR){
                call->name = node->name;
            }
            tok = tok->next;

            Node arg_head = {};
            Node * arg_cur = &arg_head;

            while(tok && !equal(tok, ")")){
                Node * a = assign(&tok, tok, diagnoses);
                if(a){
                    arg_cur->next = a;
                    arg_cur = a;
                }
                if(!consume(&tok, tok, ",")){
                    break;
                }
            }
            tok = skip(&tok, tok, ")", diagnoses);
            call->args = arg_head.next;
            node = call;
            continue;
        }

        if(equal(tok, "[")){
            // 数组下标访问
            Token * start = tok;
            tok = tok->next;
            Node * idx = expr(&tok, tok, diagnoses);
            tok = skip(&tok, tok, "]", diagnoses);
            node = new_binary(ND_ADD, node, idx, start);
            continue;
        }

        if(equal(tok, "++")){
            node = new_unary(ND_POST_INC, node, tok);
            tok = tok->next;
            continue;
        }

        if(equal(tok, "--")){
            node = new_unary(ND_POST_DEC, node, tok);
            tok = tok->next;
            continue;
        }

        break;
    }

    *rest = tok;
    return node;
}

/// unary = ("+" | "-" | "!" | "~" | "++" | "--") unary | postfix
static Node * unary(Token ** rest, Token * tok, stage_diagnoses * diagnoses){
    if(equal(tok, "+")){
        return new_unary(ND_POS, unary(rest, tok->next, diagnoses), tok);
    }
    if(equal(tok, "-")){
        return new_unary(ND_NEG, unary(rest, tok->next, diagnoses), tok);
    }
    if(equal(tok, "!")){
        return new_unary(ND_NOT, unary(rest, tok->next, diagnoses), tok);
    }
    if(equal(tok, "~")){
        return new_unary(ND_BIT_NOT, unary(rest, tok->next, diagnoses), tok);
    }
    if(equal(tok, "++")){
        return new_unary(ND_PRE_INC, unary(rest, tok->next, diagnoses), tok);
    }
    if(equal(tok, "--")){
        return new_unary(ND_PRE_DEC, unary(rest, tok->next, diagnoses), tok);
    }

    return postfix(rest, tok, diagnoses);
}

/// mul = unary ("*" unary | "/" unary | "%" unary)*
static Node * mul(Token ** rest, Token * tok, stage_diagnoses * diagnoses){
    Node * node = unary(&tok, tok, diagnoses);

    while(tok){
        if(equal(tok, "*")){
            node = new_binary(ND_MUL, node, unary(&tok, tok->next, diagnoses), tok);
            continue;
        }
        if(equal(tok, "/")){
            node = new_binary(ND_DIV, node, unary(&tok, tok->next, diagnoses), tok);
            continue;
        }
        if(equal(tok, "%")){
            node = new_binary(ND_MOD, node, unary(&tok, tok->next, diagnoses), tok);
            continue;
        }
        break;
    }

    *rest = tok;
    return node;
}

/// add = mul ("+" mul | "-" mul)*
static Node * add(Token ** rest, Token * tok, stage_diagnoses * diagnoses){
    Node * node = mul(&tok, tok, diagnoses);

    while(tok){
        if(equal(tok, "+")){
            node = new_binary(ND_ADD, node, mul(&tok, tok->next, diagnoses), tok);
            continue;
        }
        if(equal(tok, "-")){
            node = new_binary(ND_SUB, node, mul(&tok, tok->next, diagnoses), tok);
            continue;
        }
        break;
    }

    *rest = tok;
    return node;
}

/// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node * relational(Token ** rest, Token * tok, stage_diagnoses * diagnoses){
    Node * node = add(&tok, tok, diagnoses);

    while(tok){
        if(equal(tok, "<=")){
            node = new_binary(ND_LE, node, add(&tok, tok->next, diagnoses), tok);
            continue;
        }
        if(equal(tok, ">=")){
            node = new_binary(ND_GE, node, add(&tok, tok->next, diagnoses), tok);
            continue;
        }
        if(equal(tok, "<")){
            node = new_binary(ND_LT, node, add(&tok, tok->next, diagnoses), tok);
            continue;
        }
        if(equal(tok, ">")){
            node = new_binary(ND_GT, node, add(&tok, tok->next, diagnoses), tok);
            continue;
        }
        break;
    }

    *rest = tok;
    return node;
}

/// equality = relational ("==" relational | "!=" relational)*
static Node * equality(Token ** rest, Token * tok, stage_diagnoses * diagnoses){
    Node * node = relational(&tok, tok, diagnoses);

    while(tok){
        if(equal(tok, "==")){
            node = new_binary(ND_EQ, node, relational(&tok, tok->next, diagnoses), tok);
            continue;
        }
        if(equal(tok, "!=")){
            node = new_binary(ND_NE, node, relational(&tok, tok->next, diagnoses), tok);
            continue;
        }
        break;
    }

    *rest = tok;
    return node;
}

/// bitand = equality ("&" equality)*
static Node * bitand(Token ** rest, Token * tok, stage_diagnoses * diagnoses){
    Node * node = equality(&tok, tok, diagnoses);

    while(tok){
        if(equal(tok, "&")){
            node = new_binary(ND_BIT_AND, node, equality(&tok, tok->next, diagnoses), tok);
            continue;
        }
        break;
    }

    *rest = tok;
    return node;
}

/// bitxor = bitand ("^" bitand)*
static Node * bitxor(Token ** rest, Token * tok, stage_diagnoses * diagnoses){
    Node * node = bitand(&tok, tok, diagnoses);

    while(tok){
        if(equal(tok, "^")){
            node = new_binary(ND_BIT_XOR, node, bitand(&tok, tok->next, diagnoses), tok);
            continue;
        }
        break;
    }

    *rest = tok;
    return node;
}

/// bitor = bitxor ("|" bitxor)*
static Node * bitor(Token ** rest, Token * tok, stage_diagnoses * diagnoses){
    Node * node = bitxor(&tok, tok, diagnoses);

    while(tok){
        if(equal(tok, "|")){
            node = new_binary(ND_BIT_OR, node, bitxor(&tok, tok->next, diagnoses), tok);
            continue;
        }
        break;
    }

    *rest = tok;
    return node;
}

/// logand = bitor ("&&" bitor)*
static Node * logand(Token ** rest, Token * tok, stage_diagnoses * diagnoses){
    Node * node = bitor(&tok, tok, diagnoses);

    while(tok){
        if(equal(tok, "&&")){
            node = new_binary(ND_LOGICAL_AND, node, bitor(&tok, tok->next, diagnoses), tok);
            continue;
        }
        break;
    }

    *rest = tok;
    return node;
}

/// logor = logand ("||" logand)*
static Node * logor(Token ** rest, Token * tok, stage_diagnoses * diagnoses){
    Node * node = logand(&tok, tok, diagnoses);

    while(tok){
        if(equal(tok, "||")){
            node = new_binary(ND_LOGICAL_OR, node, logand(&tok, tok->next, diagnoses), tok);
            continue;
        }
        break;
    }

    *rest = tok;
    return node;
}

/// assign = logor ("=" assign)?
static Node * assign(Token ** rest, Token * tok, stage_diagnoses * diagnoses){
    Node * node = logor(&tok, tok, diagnoses);

    if(equal(tok, "=")){
        node = new_binary(ND_ASSIGN, node, assign(&tok, tok->next, diagnoses), tok);
    }

    *rest = tok;
    return node;
}

/// expr = assign
static Node * expr(Token ** rest, Token * tok, stage_diagnoses * diagnoses){
    return assign(rest, tok, diagnoses);
}

/// expr_stmt = expr? ";"
static Node * expr_stmt(Token ** rest, Token * tok, stage_diagnoses * diagnoses){
    Node * node = new_node(ND_EXPR_STMT, tok);
    if(!equal(tok, ";")){
        node->lhs = expr(&tok, tok, diagnoses);
    }
    tok = skip(&tok, tok, ";", diagnoses);
    *rest = tok;
    return node;
}

/// declaration = typename ident ("=" assign)? ("," ident ("=" assign)?)* ";"
static Node * declaration(Token ** rest, Token * tok, stage_diagnoses * diagnoses){
    Token * ty_tok = tok;
    tok = tok->next;

    Node head = {};
    Node * cur = &head;

    while(tok && tok->type == T_IDENTIFIER){
        Token * name_tok = tok;
        tok = tok->next;

        Node * var_decl = new_node(ND_VAR_DECL, name_tok);
        var_decl->type_name = ty_tok->data;
        var_decl->builtin_type = check_builtin_type(ty_tok->data);
        var_decl->name = name_tok->data;

        if(consume(&tok, tok, "=")){
            var_decl->lhs = assign(&tok, tok, diagnoses);
        }

        cur->next = var_decl;
        cur = var_decl;

        if(!consume(&tok, tok, ",")){
            break;
        }
    }

    tok = skip(&tok, tok, ";", diagnoses);
    *rest = tok;
    return head.next;
}

/// compound_stmt = "{" (declaration | stmt)* "}"
static Node * compound_stmt(Token ** rest, Token * tok, stage_diagnoses * diagnoses){
    Node * block = new_node(ND_COMPOUND_STMT, tok);
    tok = skip(&tok, tok, "{", diagnoses);

    Node head = {};
    Node * cur = &head;

    while(tok && !equal(tok, "}")){
        if(equal(tok, ";")){
            tok = tok->next;
            continue;
        }

        Node * s = NULL;
        if(is_typename(tok)){
            s = declaration(&tok, tok, diagnoses);
        }else{
            s = stmt(&tok, tok, diagnoses);
        }

        if(s){
            cur->next = s;
            while(cur->next) cur = cur->next;
        }else{
            if(tok) tok = tok->next;
        }
    }

    tok = skip(&tok, tok, "}", diagnoses);
    block->body = head.next;
    *rest = tok;
    return block;
}

/// stmt
static Node * stmt(Token ** rest, Token * tok, stage_diagnoses * diagnoses){
    if(!tok){
        *rest = NULL;
        return NULL;
    }

    if(equal(tok, "return")){
        Node * node = new_node(ND_RETURN, tok);
        tok = tok->next;
        if(!equal(tok, ";")){
            node->lhs = expr(&tok, tok, diagnoses);
        }
        tok = skip(&tok, tok, ";", diagnoses);
        *rest = tok;
        return node;
    }

    if(equal(tok, "if")){
        Node * node = new_node(ND_IF, tok);
        tok = skip(&tok, tok->next, "(", diagnoses);
        node->cond = expr(&tok, tok, diagnoses);
        tok = skip(&tok, tok, ")", diagnoses);
        node->then = stmt(&tok, tok, diagnoses);
        if(equal(tok, "else")){
            node->els = stmt(&tok, tok->next, diagnoses);
        }
        *rest = tok;
        return node;
    }

    if(equal(tok, "while")){
        Node * node = new_node(ND_WHILE, tok);
        tok = skip(&tok, tok->next, "(", diagnoses);
        node->cond = expr(&tok, tok, diagnoses);
        tok = skip(&tok, tok, ")", diagnoses);
        node->then = stmt(&tok, tok, diagnoses);
        *rest = tok;
        return node;
    }

    if(equal(tok, "do")){
        Node * node = new_node(ND_DO, tok);
        node->then = stmt(&tok, tok->next, diagnoses);
        tok = skip(&tok, tok, "while", diagnoses);
        tok = skip(&tok, tok, "(", diagnoses);
        node->cond = expr(&tok, tok, diagnoses);
        tok = skip(&tok, tok, ")", diagnoses);
        tok = skip(&tok, tok, ";", diagnoses);
        *rest = tok;
        return node;
    }

    if(equal(tok, "for")){
        Node * node = new_node(ND_FOR, tok);
        tok = skip(&tok, tok->next, "(", diagnoses);

        if(!equal(tok, ";")){
            if(is_typename(tok)){
                node->init = declaration(&tok, tok, diagnoses);
            }else{
                node->init = expr_stmt(&tok, tok, diagnoses);
            }
        }else{
            tok = tok->next;
        }

        if(!equal(tok, ";")){
            node->cond = expr(&tok, tok, diagnoses);
        }
        tok = skip(&tok, tok, ";", diagnoses);

        if(!equal(tok, ")")){
            node->inc = expr(&tok, tok, diagnoses);
        }
        tok = skip(&tok, tok, ")", diagnoses);
        node->then = stmt(&tok, tok, diagnoses);
        *rest = tok;
        return node;
    }

    if(equal(tok, "switch")){
        Node * node = new_node(ND_SWITCH, tok);
        tok = skip(&tok, tok->next, "(", diagnoses);
        node->cond = expr(&tok, tok, diagnoses);
        tok = skip(&tok, tok, ")", diagnoses);
        node->then = stmt(&tok, tok, diagnoses);
        *rest = tok;
        return node;
    }

    if(equal(tok, "case")){
        Node * node = new_node(ND_CASE, tok);
        node->lhs = expr(&tok, tok->next, diagnoses);
        tok = skip(&tok, tok, ":", diagnoses);
        node->then = stmt(&tok, tok, diagnoses);
        *rest = tok;
        return node;
    }

    if(equal(tok, "default")){
        Node * node = new_node(ND_DEFAULT, tok);
        tok = skip(&tok, tok->next, ":", diagnoses);
        node->then = stmt(&tok, tok, diagnoses);
        *rest = tok;
        return node;
    }

    if(equal(tok, "break")){
        Node * node = new_node(ND_BREAK, tok);
        tok = skip(&tok, tok->next, ";", diagnoses);
        *rest = tok;
        return node;
    }

    if(equal(tok, "continue")){
        Node * node = new_node(ND_CONTINUE, tok);
        tok = skip(&tok, tok->next, ";", diagnoses);
        *rest = tok;
        return node;
    }

    if(equal(tok, "goto")){
        Node * node = new_node(ND_GOTO, tok);
        tok = tok->next;
        if(tok && tok->type == T_IDENTIFIER){
            node->name = tok->data;
            tok = tok->next;
        }
        tok = skip(&tok, tok, ";", diagnoses);
        *rest = tok;
        return node;
    }

    if(equal(tok, "{")){
        return compound_stmt(rest, tok, diagnoses);
    }

    if(is_typename(tok)){
        return declaration(rest, tok, diagnoses);
    }

    return expr_stmt(rest, tok, diagnoses);
}

/// 预处理指令解析
static Node * parse_preprocessor(Token ** rest, Token * tok, stage_diagnoses * diagnoses){
    (void)diagnoses;
    Node * pp = new_node(ND_PREPROCESS, tok);
    Token * cur = tok->next;

    if(tok->type == T_PP_INCLUDE){
        if(cur && (cur->type == T_HEADER_NAME || cur->type == T_STRING_LITERAL)){
            pp->rhs = new_node(ND_STRING, cur);
            pp->rhs->str_val = cur->data;
            cur = cur->next;
        }
    }else if(tok->type == T_PP_DEFINE){
        if(cur && cur->type == T_IDENTIFIER){
            pp->lhs = new_var(cur);
            cur = cur->next;
            if(cur && (cur->type == T_NUMBER || cur->type == T_NUMBER_HEX ||
                       cur->type == T_STRING_LITERAL || cur->type == T_IDENTIFIER)){
                pp->rhs = new_var(cur);
                cur = cur->next;
            }
        }
    }else if(tok->type == T_PP_UNDEF){
        if(cur && cur->type == T_IDENTIFIER){
            pp->lhs = new_var(cur);
            cur = cur->next;
        }
    }

    *rest = cur;
    return pp;
}

static inline int is_function(Token * tok){
    if(!tok || !tok->next || !tok->next->next) return 0;
    return (tok->next->type == T_IDENTIFIER && equal(tok->next->next, "("));
}

/// 函数定义与函数声明
static Node * parse_function(Token ** rest, Token * tok, stage_diagnoses * diagnoses){
    Token * ty_tok = tok;
    tok = tok->next;
    Token * name_tok = tok;
    tok = tok->next;
    tok = skip(&tok, tok, "(", diagnoses);

    Node param_head = {};
    Node * param_cur = &param_head;

    while(tok && !equal(tok, ")")){
        if(is_typename(tok)){
            Token * p_ty = tok;
            tok = tok->next;
            Token * p_name = NULL;
            if(tok && tok->type == T_IDENTIFIER){
                p_name = tok;
                tok = tok->next;
            }
            Node * p_node = new_node(ND_VAR_DECL, p_name ? p_name : p_ty);
            p_node->type_name = p_ty->data;
            p_node->builtin_type = check_builtin_type(p_ty->data);
            if(p_name){
                p_node->name = p_name->data;
            }
            param_cur->next = p_node;
            param_cur = p_node;
        }else{
            sd_printf(diagnoses, "Expected parameter type at row %d col %d", 
                tok->location.row, tok->location.col);
            tok = tok->next;
        }

        if(!consume(&tok, tok, ",")){
            break;
        }
    }
    tok = skip(&tok, tok, ")", diagnoses);

    if(equal(tok, ";")){
        Node * decl = new_node(ND_FUNC_DECL, name_tok);
        decl->type_name = ty_tok->data;
        decl->builtin_type = check_builtin_type(ty_tok->data);
        decl->name = name_tok->data;
        decl->args = param_head.next;
        tok = tok->next;
        *rest = tok;
        return decl;
    }else if(equal(tok, "{")){
        Node * def = new_node(ND_FUNC_DEF, name_tok);
        def->type_name = ty_tok->data;
        def->builtin_type = check_builtin_type(ty_tok->data);
        def->name = name_tok->data;
        def->args = param_head.next;
        def->body = compound_stmt(&tok, tok, diagnoses);
        *rest = tok;
        return def;
    }else{
        sd_printf(diagnoses, "Expected ';' or '{' after function declarator at row %d col %d",
            tok ? tok->location.row : -1, tok ? tok->location.col : -1);
        *rest = tok;
        return NULL;
    }
}

/// 语法分析总入口
Node * parse_ast(Token * tok, stage_diagnoses * diagnoses){
    Node * prog = new_node(ND_PROGRAM, tok);
    Node head = {};
    Node * cur = &head;

    while(tok){
        if(equal(tok, ";")){
            tok = tok->next;
            continue;
        }

        if(is_preprocessor(tok)){
            Node * pp = parse_preprocessor(&tok, tok, diagnoses);
            if(pp){
                cur->next = pp;
                cur = pp;
            }
            continue;
        }

        if(is_typename(tok)){
            if(is_function(tok)){
                Node * fn = parse_function(&tok, tok, diagnoses);
                if(fn){
                    cur->next = fn;
                    cur = fn;
                }
            }else{
                Node * gvar = declaration(&tok, tok, diagnoses);
                if(gvar){
                    cur->next = gvar;
                    while(cur->next) cur = cur->next;
                }
            }
            continue;
        }

        Node * s = stmt(&tok, tok, diagnoses);
        if(s){
            cur->next = s;
            while(cur->next) cur = cur->next;
        }else{
            if(tok) tok = tok->next;
        }
    }

    prog->body = head.next;
    return prog;
}
