#include <formatter/parser.h>
#include <formatter/ast.h>
#include <formatter/tokenizer.h>
#include <formatter/diagnoses.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/// 语法分析内部游标上下文（仅在本文件内使用）
typedef struct {
    vector * tokens;
    int index;
    int success;
    stage_diagnoses * diagnoses;
} parser_context;

/// 获取当前 Token（不移动游标）
static inline Token * parser_peek(parser_context * ctx){
    if(!ctx || !ctx->tokens || ctx->index < 0 || ctx->index >= ctx->tokens->size) return NULL;
    return (Token *)ctx->tokens->data + ctx->index;
}

/// 前瞻第 n 个 Token（n=0 为当前 Token）
static inline Token * parser_peek_n(parser_context * ctx, int n){
    if(!ctx || !ctx->tokens) return NULL;
    int idx = ctx->index + n;
    if(idx < 0 || idx >= ctx->tokens->size) return NULL;
    return (Token *)ctx->tokens->data + idx;
}

/// 获取当前 Token 并将游标向后移动一位
static inline Token * parser_fetch(parser_context * ctx){
    Token * tok = parser_peek(ctx);
    if(tok) ctx->index++;
    return tok;
}

static inline int is_comment_token(enum TokenType type){
    return (type == T_COMMENT_SINGLE_LINE ||
            type == T_BEGIN_COMMENT_MULTIPLE_LINES ||
            type == T_COMMENT_BODY ||
            type == T_END_COMMENT_MULTIPLE_LINES);
}

static inline int check_comment(parser_context * ctx){
    Token * tok = parser_peek(ctx);
    return tok && (tok->type == T_COMMENT_SINGLE_LINE ||
                   tok->type == T_BEGIN_COMMENT_MULTIPLE_LINES);
}

static inline void skip_inline_comments(parser_context * ctx){
    while(1){
        Token * tok = parser_peek(ctx);
        if(!tok) break;
        if(tok->type == T_COMMENT_SINGLE_LINE){
            parser_fetch(ctx);
            Token * body = parser_peek(ctx);
            if(body && body->type == T_COMMENT_BODY){
                parser_fetch(ctx);
            }
        }else if(tok->type == T_BEGIN_COMMENT_MULTIPLE_LINES){
            parser_fetch(ctx);
            while(parser_peek(ctx)){
                Token * cur = parser_fetch(ctx);
                if(cur->type == T_END_COMMENT_MULTIPLE_LINES){
                    break;
                }
            }
        }else if(tok->type == T_COMMENT_BODY || tok->type == T_END_COMMENT_MULTIPLE_LINES){
            parser_fetch(ctx);
        }else{
            break;
        }
    }
}

static Node * parse_comment(parser_context * ctx){
    Token * lead = parser_fetch(ctx);
    if(!lead) return NULL;

    Node * node = new_node(ND_COMMENT, lead);

    if(lead->type == T_COMMENT_SINGLE_LINE){
        Token * body = parser_peek(ctx);
        if(body && body->type == T_COMMENT_BODY){
            parser_fetch(ctx);
            int len = (body->data.begin + body->data.length) - lead->data.begin;
            node->str_val = sv_build(lead->data.buffer, lead->data.begin, len);
        }else{
            node->str_val = lead->data;
        }
    }else if(lead->type == T_BEGIN_COMMENT_MULTIPLE_LINES){
        int total_len = lead->data.length;
        while(parser_peek(ctx)){
            Token * cur = parser_fetch(ctx);
            total_len = (cur->data.begin + cur->data.length) - lead->data.begin;
            if(cur->type == T_END_COMMENT_MULTIPLE_LINES){
                break;
            }
        }
        node->str_val = sv_build(lead->data.buffer, lead->data.begin, total_len);
    }else{
        node->str_val = lead->data;
    }
    return node;
}

/// 检查当前 Token 类型是否匹配
static inline int check(parser_context * ctx, enum TokenType type){
    Token * tok = parser_peek(ctx);
    return tok && tok->type == type;
}

/// 匹配并消耗指定类型的 Token，符合返回 1 并后移游标，否则返回 0
static inline int consume(parser_context * ctx, enum TokenType type){
    skip_inline_comments(ctx);
    Token * tok = parser_peek(ctx);
    if(tok && tok->type == type){
        ctx->index++;
        return 1;
    }
    return 0;
}

static const char * token_type_name(enum TokenType type){
    switch(type){
        case T_END_STATEMENT: return ";";
        case T_COMMA: return ",";
        case T_OP_COLON: return ":";
        case T_PARENTHESE_BEGIN: return "(";
        case T_PARENTHESE_END: return ")";
        case T_BRACE_BEGIN: return "{";
        case T_BRACE_END: return "}";
        case T_BRACKET_BEGIN: return "[";
        case T_BRACKET_END: return "]";
        case T_OP_ASSIGN: return "=";
        case T_KW_IF: return "if";
        case T_KW_ELSE: return "else";
        case T_KW_WHILE: return "while";
        case T_KW_DO: return "do";
        case T_KW_FOR: return "for";
        case T_KW_SWITCH: return "switch";
        case T_KW_CASE: return "case";
        case T_KW_DEFAULT: return "default";
        case T_KW_RETURN: return "return";
        case T_KW_BREAK: return "break";
        case T_KW_CONTINUE: return "continue";
        case T_KW_GOTO: return "goto";
        default: return token_string(type);
    }
}

/// 跳过指定类型的 Token，若不匹配则记录诊断信息
static Token * skip(parser_context * ctx, enum TokenType type){
    skip_inline_comments(ctx);
    Token * tok = parser_peek(ctx);
    if(!tok || tok->type != type){
        if(tok){
            sd_printf(ctx->diagnoses, "Expected '%s' at row %d col %d, but got \"%.*s\"",
                token_type_name(type), tok->location.row, tok->location.col,
                sv_length(tok->data), sv_begin(tok->data));
            ctx->index++;
        }else{
            sd_printf(ctx->diagnoses, "Expected '%s', but reached EOF", token_type_name(type));
        }
        ctx->success = 0;
        return NULL;
    }
    ctx->index++;
    return tok;
}

/// AST层面判别内置类型
static inline int is_typename(Token * tok){
    if(!tok) return 0;
    if(tok->type != T_IDENTIFIER) return 0;
    return check_builtin_type(tok->data) != TY_NONE;
}

static inline int check_typename(parser_context * ctx){
    return is_typename(parser_peek(ctx));
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

static inline int check_preprocessor(parser_context * ctx){
    return is_preprocessor(parser_peek(ctx));
}

static inline int is_number_token(enum TokenType type){
    return (type == T_NUMBER || type == T_NUMBER_HEX ||
            type == T_NUMBER_OCT || type == T_NUMBER_BIN ||
            type == T_NUMBER_FLOAT);
}

// 声明递归下降函数
static Node * expr(parser_context * ctx);
static Node * assign(parser_context * ctx);
static Node * logor(parser_context * ctx);
static Node * logand(parser_context * ctx);
static Node * bitor(parser_context * ctx);
static Node * bitxor(parser_context * ctx);
static Node * bitand(parser_context * ctx);
static Node * equality(parser_context * ctx);
static Node * relational(parser_context * ctx);
static Node * add(parser_context * ctx);
static Node * mul(parser_context * ctx);
static Node * unary(parser_context * ctx);
static Node * postfix(parser_context * ctx);
static Node * primary(parser_context * ctx);

static Node * stmt(parser_context * ctx);
static Node * compound_stmt(parser_context * ctx);
static Node * expr_stmt(parser_context * ctx);
static Node * declaration(parser_context * ctx);
static Node * parse_preprocessor(parser_context * ctx);

/// primary = "(" expr ")" | NUM | STRING | CHAR | IDENT
static Node * primary(parser_context * ctx){
    skip_inline_comments(ctx);
    Token * tok = parser_peek(ctx);
    if(!tok) return NULL;

    if(consume(ctx, T_PARENTHESE_BEGIN)){
        Node * node = expr(ctx);
        skip(ctx, T_PARENTHESE_END);
        return node;
    }

    if(is_number_token(tok->type)){
        ctx->index++;
        if(tok->type == T_NUMBER_FLOAT){
            return new_float(tok->ival.ld, tok);
        }
        return new_num(tok->ival.ull, tok);
    }

    if(consume(ctx, T_STRING_LITERAL)){
        Node * node = new_node(ND_STRING, tok);
        node->str_val = tok->data;
        return node;
    }

    if(consume(ctx, T_CHAR_LITERAL)){
        Node * node = new_node(ND_CHAR, tok);
        node->str_val = tok->data;
        return node;
    }

    if(consume(ctx, T_IDENTIFIER)){
        return new_var(tok);
    }

    sd_printf(ctx->diagnoses, "Unexpected token \"%.*s\" at row %d col %d in expression",
        sv_length(tok->data), sv_begin(tok->data),
        tok->location.row, tok->location.col);
    ctx->success = 0;
    ctx->index++;
    return NULL;
}

/// postfix = primary ("(" func-args? ")" | "[" expr "]" | "++" | "--")*
static Node * postfix(parser_context * ctx){
    Node * node = primary(ctx);

    while(1){
        Token * tok = parser_peek(ctx);
        if(!tok) break;

        if(consume(ctx, T_PARENTHESE_BEGIN)){
            Node * call = new_node(ND_FUNCALL, tok);
            if(node && node->kind == ND_VAR){
                call->name = node->name;
            }

            Node arg_head = {};
            Node * arg_cur = &arg_head;

            while(!check(ctx, T_PARENTHESE_END) && parser_peek(ctx)){
                Node * a = assign(ctx);
                if(a){
                    arg_cur->next = a;
                    arg_cur = a;
                }
                if(!consume(ctx, T_COMMA)){
                    break;
                }
            }
            skip(ctx, T_PARENTHESE_END);
            call->args = arg_head.next;
            node = call;
            continue;
        }

        if(consume(ctx, T_BRACKET_BEGIN)){
            Node * idx = expr(ctx);
            skip(ctx, T_BRACKET_END);
            node = new_binary(ND_INDEX, node, idx, tok);
            continue;
        }

        if(consume(ctx, T_OP_INCREMENT)){
            node = new_unary(ND_POST_INC, node, tok);
            continue;
        }

        if(consume(ctx, T_OP_DECREMENT)){
            node = new_unary(ND_POST_DEC, node, tok);
            continue;
        }

        break;
    }

    return node;
}

/// unary = ("+" | "-" | "!" | "~" | "++" | "--") unary | postfix
static Node * unary(parser_context * ctx){
    Token * tok = parser_peek(ctx);
    if(!tok) return NULL;

    if(consume(ctx, T_OP_ADD)) return new_unary(ND_POS, unary(ctx), tok);
    if(consume(ctx, T_OP_MINUS)) return new_unary(ND_NEG, unary(ctx), tok);
    if(consume(ctx, T_OP_NOT)) return new_unary(ND_NOT, unary(ctx), tok);
    if(consume(ctx, T_OP_BIT_NOT)) return new_unary(ND_BIT_NOT, unary(ctx), tok);
    if(consume(ctx, T_OP_INCREMENT)) return new_unary(ND_PRE_INC, unary(ctx), tok);
    if(consume(ctx, T_OP_DECREMENT)) return new_unary(ND_PRE_DEC, unary(ctx), tok);

    return postfix(ctx);
}

/// mul = unary ("*" unary | "/" unary | "%" unary)*
static Node * mul(parser_context * ctx){
    Node * node = unary(ctx);

    while(1){
        Token * tok = parser_peek(ctx);
        if(!tok) break;
        if(consume(ctx, T_OP_MUL)){
            node = new_binary(ND_MUL, node, unary(ctx), tok);
        }else if(consume(ctx, T_OP_DIV)){
            node = new_binary(ND_DIV, node, unary(ctx), tok);
        }else if(consume(ctx, T_OP_MOD)){
            node = new_binary(ND_MOD, node, unary(ctx), tok);
        }else{
            break;
        }
    }
    return node;
}

/// add = mul ("+" mul | "-" mul)*
static Node * add(parser_context * ctx){
    Node * node = mul(ctx);

    while(1){
        Token * tok = parser_peek(ctx);
        if(!tok) break;
        if(consume(ctx, T_OP_ADD)){
            node = new_binary(ND_ADD, node, mul(ctx), tok);
        }else if(consume(ctx, T_OP_MINUS)){
            node = new_binary(ND_SUB, node, mul(ctx), tok);
        }else{
            break;
        }
    }
    return node;
}

/// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node * relational(parser_context * ctx){
    Node * node = add(ctx);

    while(1){
        Token * tok = parser_peek(ctx);
        if(!tok) break;
        if(consume(ctx, T_OP_LT)){
            node = new_binary(ND_LT, node, add(ctx), tok);
        }else if(consume(ctx, T_OP_LE)){
            node = new_binary(ND_LE, node, add(ctx), tok);
        }else if(consume(ctx, T_OP_GT)){
            node = new_binary(ND_GT, node, add(ctx), tok);
        }else if(consume(ctx, T_OP_GE)){
            node = new_binary(ND_GE, node, add(ctx), tok);
        }else{
            break;
        }
    }
    return node;
}

/// equality = relational ("==" relational | "!=" relational)*
static Node * equality(parser_context * ctx){
    Node * node = relational(ctx);

    while(1){
        Token * tok = parser_peek(ctx);
        if(!tok) break;
        if(consume(ctx, T_OP_EQ)){
            node = new_binary(ND_EQ, node, relational(ctx), tok);
        }else if(consume(ctx, T_OP_NE)){
            node = new_binary(ND_NE, node, relational(ctx), tok);
        }else{
            break;
        }
    }
    return node;
}

/// bitand = equality ("&" equality)*
static Node * bitand(parser_context * ctx){
    Node * node = equality(ctx);

    while(1){
        Token * tok = parser_peek(ctx);
        if(!tok) break;
        if(consume(ctx, T_OP_BIT_AND)){
            node = new_binary(ND_BIT_AND, node, equality(ctx), tok);
        }else{
            break;
        }
    }
    return node;
}

/// bitxor = bitand ("^" bitand)*
static Node * bitxor(parser_context * ctx){
    Node * node = bitand(ctx);

    while(1){
        Token * tok = parser_peek(ctx);
        if(!tok) break;
        if(consume(ctx, T_OP_BIT_XOR)){
            node = new_binary(ND_BIT_XOR, node, bitand(ctx), tok);
        }else{
            break;
        }
    }
    return node;
}

/// bitor = bitxor ("|" bitxor)*
static Node * bitor(parser_context * ctx){
    Node * node = bitxor(ctx);

    while(1){
        Token * tok = parser_peek(ctx);
        if(!tok) break;
        if(consume(ctx, T_OP_BIT_OR)){
            node = new_binary(ND_BIT_OR, node, bitxor(ctx), tok);
        }else{
            break;
        }
    }
    return node;
}

/// logand = bitor ("&&" bitor)*
static Node * logand(parser_context * ctx){
    Node * node = bitor(ctx);

    while(1){
        Token * tok = parser_peek(ctx);
        if(!tok) break;
        if(consume(ctx, T_OP_LOGICAL_AND)){
            node = new_binary(ND_LOGICAL_AND, node, bitor(ctx), tok);
        }else{
            break;
        }
    }
    return node;
}

/// logor = logand ("||" logand)*
static Node * logor(parser_context * ctx){
    Node * node = logand(ctx);

    while(1){
        Token * tok = parser_peek(ctx);
        if(!tok) break;
        if(consume(ctx, T_OP_LOGICAL_OR)){
            node = new_binary(ND_LOGICAL_OR, node, logand(ctx), tok);
        }else{
            break;
        }
    }
    return node;
}

/// assign = logor ("=" assign)?
static Node * assign(parser_context * ctx){
    Node * node = logor(ctx);
    Token * tok = parser_peek(ctx);
    if(tok && consume(ctx, T_OP_ASSIGN)){
        node = new_binary(ND_ASSIGN, node, assign(ctx), tok);
    }
    return node;
}

/// expr = assign
static Node * expr(parser_context * ctx){
    return assign(ctx);
}

/// expr_stmt = expr? ";"
static Node * expr_stmt(parser_context * ctx){
    Token * tok = parser_peek(ctx);
    Node * node = new_node(ND_EXPR_STMT, tok);
    if(!check(ctx, T_END_STATEMENT)){
        node->lhs = expr(ctx);
    }
    skip(ctx, T_END_STATEMENT);
    return node;
}

/// declaration = typename ident ("=" expr)? ("," ident ("=" expr)?)* ";"
static Node * declaration(parser_context * ctx){
    Token * ty_tok = parser_fetch(ctx);
    Node * first_var = NULL;
    Node * last_sub = NULL;

    while(1){
        skip_inline_comments(ctx);
        Token * var_tok = parser_peek(ctx);
        if(!check(ctx, T_IDENTIFIER)){
            sd_printf(ctx->diagnoses, "Expected variable name at row %d col %d",
                var_tok ? var_tok->location.row : -1, var_tok ? var_tok->location.col : -1);
            ctx->success = 0;
            break;
        }
        parser_fetch(ctx);

        Node * var = new_node(ND_VAR_DECL, var_tok);
        var->type_name = ty_tok->data;
        var->builtin_type = check_builtin_type(ty_tok->data);
        var->name = var_tok->data;

        if(consume(ctx, T_OP_ASSIGN)){
            var->lhs = assign(ctx);
        }

        if(!first_var){
            first_var = var;
        }else{
            if(!last_sub){
                first_var->args = var;
            }else{
                last_sub->next = var;
            }
            last_sub = var;
        }

        if(!consume(ctx, T_COMMA)){
            break;
        }
    }

    skip(ctx, T_END_STATEMENT);
    return first_var;
}

/// compound_stmt = "{" (declaration | stmt)* "}"
static Node * compound_stmt(parser_context * ctx){
    Token * start = parser_peek(ctx);
    Node * block = new_node(ND_COMPOUND_STMT, start);
    skip(ctx, T_BRACE_BEGIN);

    Node head = {};
    Node * cur = &head;

    while(!check(ctx, T_BRACE_END) && parser_peek(ctx)){
        if(check_comment(ctx)){
            Node * c = parse_comment(ctx);
            if(c){
                cur->next = c;
                cur = c;
            }
            continue;
        }

        if(consume(ctx, T_END_STATEMENT)){
            continue;
        }

        Node * s = NULL;
        if(check_preprocessor(ctx)){
            s = parse_preprocessor(ctx);
        }else if(check_typename(ctx)){
            s = declaration(ctx);
        }else{
            s = stmt(ctx);
        }

        if(s){
            cur->next = s;
            while(cur->next) cur = cur->next;
        }else{
            if(parser_peek(ctx)) ctx->index++;
        }
    }

    skip(ctx, T_BRACE_END);
    block->body = head.next;
    return block;
}

/// stmt
static Node * stmt(parser_context * ctx){
    Token * tok = parser_peek(ctx);
    if(!tok) return NULL;

    if(check_comment(ctx)){
        return parse_comment(ctx);
    }

    switch(tok->type){
        case T_KW_RETURN: {
            Node * node = new_node(ND_RETURN, tok);
            ctx->index++;
            if(!check(ctx, T_END_STATEMENT)){
                node->lhs = expr(ctx);
            }
            skip(ctx, T_END_STATEMENT);
            return node;
        }

        case T_KW_IF: {
            Node * node = new_node(ND_IF, tok);
            ctx->index++;
            skip(ctx, T_PARENTHESE_BEGIN);
            node->cond = expr(ctx);
            skip(ctx, T_PARENTHESE_END);
            node->then = stmt(ctx);
            if(consume(ctx, T_KW_ELSE)){
                node->els = stmt(ctx);
            }
            return node;
        }

        case T_KW_WHILE: {
            Node * node = new_node(ND_WHILE, tok);
            ctx->index++;
            skip(ctx, T_PARENTHESE_BEGIN);
            node->cond = expr(ctx);
            skip(ctx, T_PARENTHESE_END);
            node->then = stmt(ctx);
            return node;
        }

        case T_KW_DO: {
            Node * node = new_node(ND_DO, tok);
            ctx->index++;
            node->then = stmt(ctx);
            skip(ctx, T_KW_WHILE);
            skip(ctx, T_PARENTHESE_BEGIN);
            node->cond = expr(ctx);
            skip(ctx, T_PARENTHESE_END);
            skip(ctx, T_END_STATEMENT);
            return node;
        }

        case T_KW_FOR: {
            Node * node = new_node(ND_FOR, tok);
            ctx->index++;
            skip(ctx, T_PARENTHESE_BEGIN);

            if(!check(ctx, T_END_STATEMENT)){
                if(check_typename(ctx)){
                    node->init = declaration(ctx);
                }else{
                    node->init = expr_stmt(ctx);
                }
            }else{
                ctx->index++;
            }

            if(!check(ctx, T_END_STATEMENT)){
                node->cond = expr(ctx);
            }
            skip(ctx, T_END_STATEMENT);

            if(!check(ctx, T_PARENTHESE_END)){
                node->inc = expr(ctx);
            }
            skip(ctx, T_PARENTHESE_END);
            node->then = stmt(ctx);
            return node;
        }

        case T_KW_SWITCH: {
            Node * node = new_node(ND_SWITCH, tok);
            ctx->index++;
            skip(ctx, T_PARENTHESE_BEGIN);
            node->cond = expr(ctx);
            skip(ctx, T_PARENTHESE_END);
            node->then = stmt(ctx);
            return node;
        }

        case T_KW_CASE: {
            Node * node = new_node(ND_CASE, tok);
            ctx->index++;
            node->lhs = expr(ctx);
            skip(ctx, T_OP_COLON);
            node->then = stmt(ctx);
            return node;
        }

        case T_KW_DEFAULT: {
            Node * node = new_node(ND_DEFAULT, tok);
            ctx->index++;
            skip(ctx, T_OP_COLON);
            node->then = stmt(ctx);
            return node;
        }

        case T_KW_BREAK: {
            Node * node = new_node(ND_BREAK, tok);
            ctx->index++;
            skip(ctx, T_END_STATEMENT);
            return node;
        }

        case T_KW_CONTINUE: {
            Node * node = new_node(ND_CONTINUE, tok);
            ctx->index++;
            skip(ctx, T_END_STATEMENT);
            return node;
        }

        case T_KW_GOTO: {
            Node * node = new_node(ND_GOTO, tok);
            ctx->index++;
            Token * name_tok = parser_peek(ctx);
            if(consume(ctx, T_IDENTIFIER)){
                node->name = name_tok->data;
            }
            skip(ctx, T_END_STATEMENT);
            return node;
        }

        case T_BRACE_BEGIN:
            return compound_stmt(ctx);

        default:
            if(check_comment(ctx)){
                return parse_comment(ctx);
            }
            if(check_preprocessor(ctx)){
                return parse_preprocessor(ctx);
            }
            if(check_typename(ctx)){
                return declaration(ctx);
            }
            if(check(ctx, T_IDENTIFIER) && parser_peek_n(ctx, 1) && parser_peek_n(ctx, 1)->type == T_OP_COLON){
                Token * lbl_tok = parser_fetch(ctx);
                parser_fetch(ctx); // consume ':'
                Node * lbl = new_node(ND_LABEL, lbl_tok);
                lbl->name = lbl_tok->data;
                return lbl;
            }
            return expr_stmt(ctx);
    }
}

/// 预处理指令解析
static Node * parse_preprocessor(parser_context * ctx){
    Token * tok = parser_fetch(ctx);
    if(!tok) return NULL;

    Node * pp = new_node(ND_PREPROCESS, tok);
    Token * cur = parser_peek(ctx);

    if(tok->type == T_PP_INCLUDE){
        if(cur && (cur->type == T_HEADER_NAME || cur->type == T_STRING_LITERAL)){
            pp->rhs = new_node(ND_STRING, cur);
            pp->rhs->str_val = cur->data;
            ctx->index++;
        }
    }else if(tok->type == T_PP_DEFINE){
        if(cur && cur->type == T_IDENTIFIER){
            pp->lhs = new_var(cur);
            ctx->index++;
            cur = parser_peek(ctx);
            if(cur && (is_number_token(cur->type) || cur->type == T_STRING_LITERAL || cur->type == T_IDENTIFIER)){
                pp->rhs = new_var(cur);
                ctx->index++;
            }
        }
    }else if(tok->type == T_PP_UNDEF || tok->type == T_PP_IFDEF || tok->type == T_PP_IFNDEF){
        if(cur && cur->type == T_IDENTIFIER){
            pp->lhs = new_var(cur);
            ctx->index++;
        }
    }

    // 确保同一行的剩余 token 归属于该预处理指令，跳过防止干扰后续语句
    while(1){
        Token * next = parser_peek(ctx);
        if(next && next->location.row == tok->location.row){
            ctx->index++;
        }else{
            break;
        }
    }

    return pp;
}

static inline int is_function(parser_context * ctx){
    Token * t1 = parser_peek_n(ctx, 1);
    Token * t2 = parser_peek_n(ctx, 2);
    return (t1 && t1->type == T_IDENTIFIER && t2 && t2->type == T_PARENTHESE_BEGIN);
}

/// 函数定义与函数声明
static Node * parse_function(parser_context * ctx){
    Token * ty_tok = parser_fetch(ctx);
    Token * name_tok = parser_fetch(ctx);
    skip(ctx, T_PARENTHESE_BEGIN);

    Node param_head = {};
    Node * param_cur = &param_head;

    while(!check(ctx, T_PARENTHESE_END) && parser_peek(ctx)){
        skip_inline_comments(ctx);
        if(check(ctx, T_PARENTHESE_END)) break;
        if(check_typename(ctx)){
            Token * p_ty = parser_fetch(ctx);
            Token * p_name = NULL;
            skip_inline_comments(ctx);
            if(check(ctx, T_IDENTIFIER)){
                p_name = parser_fetch(ctx);
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
            Token * cur = parser_peek(ctx);
            sd_printf(ctx->diagnoses, "Expected parameter type at row %d col %d", 
                cur ? cur->location.row : -1, cur ? cur->location.col : -1);
            ctx->success = 0;
            if(cur) ctx->index++;
        }

        if(!consume(ctx, T_COMMA)){
            break;
        }
    }
    skip(ctx, T_PARENTHESE_END);

    if(consume(ctx, T_END_STATEMENT)){
        Node * decl = new_node(ND_FUNC_DECL, name_tok);
        decl->type_name = ty_tok->data;
        decl->builtin_type = check_builtin_type(ty_tok->data);
        decl->name = name_tok->data;
        decl->args = param_head.next;
        return decl;
    }else if(check(ctx, T_BRACE_BEGIN)){
        Node * def = new_node(ND_FUNC_DEF, name_tok);
        def->type_name = ty_tok->data;
        def->builtin_type = check_builtin_type(ty_tok->data);
        def->name = name_tok->data;
        def->args = param_head.next;
        def->body = compound_stmt(ctx);
        return def;
    }else{
        Token * tok = parser_peek(ctx);
        sd_printf(ctx->diagnoses, "Expected ';' or '{' after function declarator at row %d col %d",
            tok ? tok->location.row : -1, tok ? tok->location.col : -1);
        ctx->success = 0;
        return NULL;
    }
}

/// 语法分析总入口
parser_result parse_ast(vector * tokens){
    parser_result result = {
        .success = 1,
        .root = NULL,
        .diagnoses = vec_new(sizeof(stage_diagnosis), 8)
    };
    if(!tokens || tokens->size == 0) return result;

    parser_context context = {
        .tokens = tokens,
        .index = 0,
        .success = 1,
        .diagnoses = &(result.diagnoses)
    };
    parser_context * ctx = &context;

    Token * first_tok = parser_peek(ctx);
    Node * prog = new_node(ND_PROGRAM, first_tok);
    Node head = {};
    Node * cur = &head;

    while(parser_peek(ctx)){
        if(check_comment(ctx)){
            Node * c = parse_comment(ctx);
            if(c){
                cur->next = c;
                cur = c;
            }
            continue;
        }

        if(consume(ctx, T_END_STATEMENT)){
            continue;
        }

        if(check_preprocessor(ctx)){
            Node * pp = parse_preprocessor(ctx);
            if(pp){
                cur->next = pp;
                cur = pp;
            }
            continue;
        }

        if(check_typename(ctx)){
            if(is_function(ctx)){
                Node * fn = parse_function(ctx);
                if(fn){
                    cur->next = fn;
                    cur = fn;
                }
            }else{
                Node * gvar = declaration(ctx);
                if(gvar){
                    cur->next = gvar;
                    while(cur->next) cur = cur->next;
                }
            }
            continue;
        }

        Node * s = stmt(ctx);
        if(s){
            cur->next = s;
            while(cur->next) cur = cur->next;
        }else{
            if(parser_peek(ctx)) ctx->index++;
        }
    }

    prog->body = head.next;
    result.root = prog;
    result.success = ctx->success && (result.diagnoses.size == 0);
    return result;
}

void parser_result_delete(parser_result * res){
    if(!res) return;
    sd_delete(&(res->diagnoses));
    if(res->root){
        ast_free(res->root);
        res->root = NULL;
    }
}
