#include <formatter/tokenizer.h>
#include <stdio.h>

typedef struct {
    int index;
    int end;
    TokenLocation location;
    bool recovering;
} tokenizer_recovery_context;

typedef struct {
    string_view sv;
    vector * vec;

    int index;
    TokenLocation location;

    stage_diagnoses * diagnoses;
} parsing_context;

typedef struct {
    int success;
} token_result;

char context_peek(parsing_context * context);
char context_peek_n(parsing_context * context, int n);
char context_fetch(parsing_context * context);
char* context_fetch_n(parsing_context * context, int n);
// 进入分支后一定是success的
token_result parse_ident(parsing_context * context);
token_result parse_single_line_comment(parsing_context * context);
token_result parse_multiple_lines_comment(parsing_context * context);

#define GEN_TOKEN(NAME,TYPE,LEN) \
    Token NAME = { \
        .type = TYPE, \
        .data = sv_substr(context.sv,context.index, LEN), \
        .location = context.location \
    }

tokenizer_result parse_tokens(string_view sv, vector * vec){
    tokenizer_result result = {
        .success = 1,
        .diagnoses = vec_new(sizeof(stage_diagnosis), 8)
    };
    
    parsing_context context = {
        .sv = sv,
        .vec = vec,
        .index = 0,
        .location = {
            .col = 0,
            .row = 0
        },
        .diagnoses = &(result.diagnoses)
    };

    tokenizer_recovery_context recovery = {
        .index = -1,
        .end = 0,
        .location = context.location
    }; 

    while(context.index < sv_length(sv)){
        // printf("parsing... %d/%d\n",context.index,sv_length(sv));
        // 跳过空格
        char peek = context_peek(&context);
        
        // 初始化恢复的context
        recovery.recovering = false;
        recovery.end = context.index;

        if(ch_space(peek)){
            context_fetch(&context);
        }else if(ch_token_begin(peek)){
            result.success &= parse_ident(&context).success;
        }else if(ch_statement_end(peek)){
            GEN_TOKEN(token,T_END_STATEMENT, 1);
            context_fetch(&context);

            *((Token*)vec_push_back(vec)) = token;
        }else if(ch_operand_div(peek)){ 
            char peek2 = context_peek_n(&context,1); // 继续往下看

            if(ch_comment_2_single_line(peek2)){
                GEN_TOKEN(token,T_COMMENT_SINGLE_LINE,2);
                // 单行注释，进入专门的解析
                *((Token*)vec_push_back(vec)) = token;

                result.success &= parse_single_line_comment(&context).success;
            }else if(ch_comment_2_multiple_lines(peek2)){
                GEN_TOKEN(token,T_BEGIN_COMMENT_MULTIPLE_LINES, 2);
                // 多行，进入专门的解析
                *((Token*)vec_push_back(vec)) = token;

                result.success &= parse_multiple_lines_comment(&context).success;
            }else {
                context_fetch(&context);
            }
        }else{
            recovery.recovering = true;
            if(recovery.index < 0){
                recovery.index = context.index;
                recovery.location = context.location;
            }
            context_fetch(&context);
        }

        if(!recovery.recovering && recovery.index >= 0){
            string_view sv = sv_substr(context.sv,recovery.index,recovery.end - recovery.index);
            sd_printf(
                &(result.diagnoses),
                "Unrecognized token \"%.*s\" at row %d col %d.",
                sv_length(sv),
                sv_begin(sv),
                recovery.location.row, recovery.location.col
            );
            recovery.index = -1;
            // 失败了
            result.success = 0;
        }
    }

    return result;
}

#undef GEN_TOKEN

token_result parse_single_line_comment(parsing_context * context){
    context_fetch_n(context,2);
    
    token_result ret = {
        .success = 0
    };
    int pos_begin = context->index;
    Token token = token_null(context->sv);
    token.location = context->location;

    char prev = '\0';
    while(true){
        char peek = context_peek(context);
        if(ch_line_break(peek) && !ch_comment_single_line_renew(prev) || ch_eof(peek)) break;
        context_fetch(context);

        if(peek != '\r')prev = peek;
        // printf("fetched %c\n",context_fetch(context));
    }
    
    // printf("%d %d %d\n",pos_begin, context->index , context->sv.length);

    ret.success = 1;
    token.type = T_COMMENT_BODY;
    token.data = sv_substr(context->sv,pos_begin,context->index - pos_begin);

    *((Token*)vec_push_back(context->vec)) = token;

    return ret;
}

token_result parse_multiple_lines_comment(parsing_context * context){
    token_result ret = {
        .success = 0
    };
    TokenLocation begin = context->location;
    context_fetch_n(context,2);

    int pos_begin = context->index;
    Token token = token_null(context->sv);
    token.location = context->location;

    char prev = '\0';
    bool ended = false;
    while(true){
        char peek = context_peek(context);
        if(ch_comment_2_multiple_lines(prev) && ch_operand_div(peek)){
            ended = true;
            context_fetch(context); // 读取完毕目前的operand
            break;
        }else if(ch_eof(peek)) break;

        context_fetch(context);

        if(peek != '\r')prev = peek;
        // printf("fetched %c\n",context_fetch(context));
    }
    
    // printf("%d %d %d\n",pos_begin, context->index , context->sv.length);

    token.type = T_COMMENT_BODY;
    token.data = sv_substr(context->sv,pos_begin,context->index - pos_begin - ended * 2);\
    // 注释内容
    *((Token*)vec_push_back(context->vec)) = token;

    if(ended){
        ret.success = 1;
        token.type = T_END_COMMENT_MULTIPLE_LINES;
        token.data = sv_substr(context->sv,context->index - 2,2);
        token.location = context->location;
        token.location.col -= 2; // */ 一定是在一行里面的，因此就可以直接读取
        // 注释结尾
        *((Token*)vec_push_back(context->vec)) = token;
    }else{
        sd_printf(
            context->diagnoses,
            "Unclosed multiple-lines comment at row %d col %d!",
            begin.row, begin.col
        );
    }

    return ret;
}

const char * token_string(enum TokenType type){
    return token_strings[type];
}

char context_peek(parsing_context * context){
    if(context->index >= sv_length(context->sv)) return '\0'; 
    return sv_at(context->sv,context->index);
}

char context_peek_n(parsing_context * context,int n){
    if(context->index >= sv_length(context->sv)) return '\0'; 
    return sv_at(context->sv,context->index + n);
}


char context_fetch(parsing_context * context){
    char ch = sv_at(context->sv,context->index);
    /// Very important?
    if(ch_eof(ch))return ch;
    
    if(ch_line_break(ch)){
        ++(context->location.row);
        context->location.col = 0;
    }else ++(context->location.col);

    ++(context->index);
    return ch;
}


char* context_fetch_n(parsing_context * context, int n){
    char * begin = sv_begin(context->sv) + context->index;
    for(int i = 0;i < n;++i)context_fetch(context);
    return begin;
}

token_result parse_ident(parsing_context * context){
    token_result ret = {
        .success = 0
    };
    int pos_begin = context->index;
    Token token = token_null(context->sv);
    token.location = context->location;

    while(true){
        char peek = context_peek(context);
        if(!ch_token_middle(peek) || ch_eof(peek)) break;
        context_fetch(context);
        // printf("fetched %c\n",context_fetch(context));
    }
    
    // printf("%d %d %d\n",pos_begin, context->index , context->sv.length);

    ret.success = 1;
    token.type = T_IDENTIFIER;
    token.data = sv_substr(context->sv,pos_begin,context->index - pos_begin);

    *((Token*)vec_push_back(context->vec)) = token;
}