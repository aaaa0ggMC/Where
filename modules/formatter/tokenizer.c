#include <formatter/tokenizer.h>
#include <stdio.h>

typedef struct {
    string_view sv;
    vector * vec;

    int index;
    TokenLocation location;
} parsing_context;

typedef struct {
    int success;
    Token token;
} token_result;

char context_peek(parsing_context * context);
char context_fetch(parsing_context * context);
// 进入分支后一定是success的
token_result parse_ident(parsing_context * context);

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
        }
    };


    int recovery_index = -1;
    int recovery_end = 0;
    TokenLocation recovery_location;

    while(context.index < sv_length(sv)){
        // printf("parsing... %d/%d\n",context.index,sv_length(sv));
        // 跳过空格
        char peek = context_peek(&context);
        bool recovering = false;


        recovery_end = context.index;
        if(ch_space(peek)){
            context_fetch(&context);
        }else if(ch_token_begin(peek)){
            *((Token*)vec_push_back(vec)) = parse_ident(&context).token;
        }else if(ch_statement_end(peek)){
            Token token = {
                .type = T_END_STATEMENT,
                .data = sv_substr(context.sv,context.index,1),
                .location = context.location
            };
            context_fetch(&context);

            *((Token*)vec_push_back(vec)) = token;
        }else{
            recovering = true;
            if(recovery_index < 0){
                recovery_index = context.index;
                recovery_location = context.location;
            }
            context_fetch(&context);
        }

        if(!recovering && recovery_index >= 0){
            string_view sv = sv_substr(context.sv,recovery_index,recovery_end - recovery_index);
            sd_printf(
                &(result.diagnoses),
                "Unrecognized token \"%.*s\" at row %d col %d.",
                sv_length(sv),
                sv_begin(sv),
                recovery_location.row, recovery_location.col
            );
            recovery_index = -1;
        }
    }

    return result;
}

const char * token_string(enum TokenType type){
    return token_strings[type];
}

char context_peek(parsing_context * context){
    if(context->index >= sv_length(context->sv)) return '\0'; 
    return sv_at(context->sv,context->index);
}

char context_fetch(parsing_context * context){
    char ch = sv_at(context->sv,context->index);
    if(ch_line_break(ch)){
        ++(context->location.row);
        context->location.col = 0;
    }else ++(context->location.col);

    ++(context->index);
    return ch;
}

token_result parse_ident(parsing_context * context){
    token_result ret = {
        .success = 0,
        .token = token_null(context->sv)
    };
    int pos_begin = context->index;
    ret.token.location = context->location;

    while(true){
        char peek = context_peek(context);
        if(!ch_token_middle(peek) || ch_eof(peek)) break;
        context_fetch(context);
        // printf("fetched %c\n",context_fetch(context));
    }
    
    // printf("%d %d %d\n",pos_begin, context->index , context->sv.length);

    ret.success = 1;
    ret.token.type = T_IDENTIFIER;
    ret.token.data = sv_substr(context->sv,pos_begin,context->index - pos_begin);

    return ret;
}