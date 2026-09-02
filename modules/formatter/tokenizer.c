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
token_result parse_ident(parsing_context * context);

tokenizer_result parse_tokens(string_view sv, vector * vec){
    tokenizer_result result = {
        .success = 1
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

    while(context.index < sv_length(sv)){
        // printf("parsing... %d/%d\n",context.index,sv_length(sv));
        // 跳过空格
        char peek = context_peek(&context);
        if(ch_space(peek)){
            context_fetch(&context);
            continue;
        }else if(ch_token_begin(peek)){
            *((Token*)vec_push_back(vec)) = parse_ident(&context).token;
        }else context_fetch(&context); // 暂时不报错
    }

    return result;
}

void delete_tokenizer_result(tokenizer_result * result){
    if(result->message) free(result->message);
}

char context_peek(parsing_context * context){
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

    while(ch_token_middle(context_peek(context))){
        context_fetch(context);
        // printf("fetched %c\n",context_fetch(context));
    }
    
    // printf("%d %d %d\n",pos_begin, context->index , context->sv.length);

    ret.success = 1;
    ret.token.type = T_IDENTIFIER;
    ret.token.data = sv_substr(context->sv,pos_begin,context->index - pos_begin);

    return ret;
}