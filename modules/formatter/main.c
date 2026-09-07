#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// 帮助内容
#include <formatter/help_message.h>
#include <formatter/tokenizer.h>
#include <formatter/ast.h>
#include <formatter/parser.h>
#include <formatter/format.h>

/// 预留的token解析大小
#define RESERVE_TOKENS 1024
#define ERROR_CLEANUP {puts(help_message);exit(-1);}
#define LOG(...) printf(__VA_ARGS__)

static int is_number_token(enum TokenType type){
    return type == T_NUMBER ||
        type == T_NUMBER_HEX ||
        type == T_NUMBER_OCT ||
        type == T_NUMBER_BIN ||
        type == T_NUMBER_FLOAT;
}

static const char * ival_type_string(enum TokenIValType itype){
    switch(itype){
        case I_VOID: return "I_VOID";
        case I_INT: return "I_INT";
        case I_LONG: return "I_LONG";
        case I_UINT: return "I_UINT";
        case I_ULONG: return "I_ULONG";
        case I_LONG_LONG: return "I_LONG_LONG";
        case I_ULONG_LONG: return "I_ULONG_LONG";
        case I_DOUBLE: return "I_DOUBLE";
        case I_FLOAT: return "I_FLOAT";
        case I_LONG_DOUBLE: return "I_LONG_DOUBLE";
    }
    return "?";
}

/// 打印时对不可见字符做转义，避免多行 token（如注释）把调试输出弄乱
static void log_token_data(string_view sv){
    for(char * ptr = sv_begin(sv); ptr != sv_end(sv); ++ptr){
        switch(*ptr){
            case '\n': fputs("\\n", stdout); break;
            case '\t': fputs("\\t", stdout); break;
            case '\r': fputs("\\r", stdout); break;
            default: putchar(*ptr);
        }
    }
}

static void log_token(Token * token){
    LOG("type=%-22s data=\"", token_string(token->type));
    log_token_data(token->data);
    LOG("\"");
    if(is_number_token(token->type)){
        LOG(" itype=%-14s ival=", ival_type_string(token->itype));
        switch(token->itype){
            case I_UINT: case I_ULONG: case I_ULONG_LONG:
            case I_INT: case I_LONG: case I_LONG_LONG:
                LOG("%llu", token->ival.ull); break;
            case I_FLOAT: case I_DOUBLE: case I_LONG_DOUBLE:
                LOG("%Lg", token->ival.ld); break;
            default: LOG("?"); break;
        }
    }
    LOG(" loc=(%d,%d)\n", token->location.row, token->location.col);
}


int main(int argc, const char * argv[]){
    const char * input_file_path = NULL;
    const char * output_file_path = NULL;

    // 这里进行简单的flags查找然后设置输出文件/输出帮助
    int analyse_index = 1;
    while(analyse_index < argc){
        if(!strcmp(argv[analyse_index], "--help")){
            puts(help_message);
            return 0;
        }else if(!strcmp(argv[analyse_index], "--format")){
            // 比如 fmt --format 这里argc是2，如果argc <= 1+1 说明没有输出文件，这是不对的
            if(argc <= analyse_index + 1){
                fprintf(stderr, "Fatal Error: no output file specified. \n");
                ERROR_CLEANUP;
            }
            
            output_file_path = argv[++analyse_index];
        }else{
            if(input_file_path){
                fprintf(stderr, "Fatal Error: bad trailing \"%s\".\n", argv[analyse_index]);
                ERROR_CLEANUP;
            }
            input_file_path = argv[analyse_index];
        }

        ++analyse_index;
    }

    if(!input_file_path){
        fprintf(stderr, "Fatal Error: no input file specified. \n");
        ERROR_CLEANUP;
    }

    // 汇总一下作为进度条
    LOG("Got in=\"%s\" out=\"%s\" \n", input_file_path , output_file_path ? output_file_path : "<none>");
    
    // 读取文件
    FILE * input_file = fopen(input_file_path, "r");
    // !IMPORTANT sv我的选择是指向指针的指针，因此这里必须保证生命周期足够长
    // 虽然这里代码显然可以看出来生命周期很长
    char * file_buffer = NULL;
    string_view input_sv;

    if(!input_file){
        fprintf(stderr, "Fatal Error: unable to open file \"%s\"! \n", input_file_path);
        ERROR_CLEANUP;
    }
    {
        fseek(input_file,0,SEEK_END);

        long buffer_size = ftell(input_file);
        file_buffer = (char *)malloc(sizeof(char) * (buffer_size + 1));
        // 确保是一个合法的字符串
        memset(file_buffer,0,sizeof(char) * (buffer_size+1));

        fseek(input_file,0,SEEK_SET);
        
        fread(file_buffer,sizeof(char),buffer_size,input_file);
        // 不算后面的0
        input_sv = sv_build(file_buffer, 0 , buffer_size);
    
        LOG("Read input:\n```c\n%s\n```\n", file_buffer);
    }
    fclose(input_file);
    // 后面基本不会定义变量了，而且workflow很清晰，因此为了进行资源清理，使用goto
    vector tokens = vec_new(sizeof(Token), RESERVE_TOKENS);

    // 词法分析
    tokenizer_result t_result = parse_tokens(input_sv, &tokens);
    for(
        void * data = vec_begin(&(t_result.diagnoses)); 
        data != vec_end(&(t_result.diagnoses)); 
        data = vec_next(&(t_result.diagnoses),data)
    ){
        stage_diagnosis * ana = data;
        LOG("%s \n",ana->message);
    }
    sd_delete(&(t_result.diagnoses));

    for(
        void * data = vec_begin(&tokens); 
        data != vec_end(&tokens); 
        data = vec_next(&tokens,data)
    ){
        log_token((Token *)data);
    }

    if(!t_result.success) goto parse_tokens_failed;

    // 语法分析
    parser_result p_result = parse_ast(&tokens);

    for(
        void * data = vec_begin(&(p_result.diagnoses)); 
        data != vec_end(&(p_result.diagnoses)); 
        data = vec_next(&(p_result.diagnoses), data)
    ){
        stage_diagnosis * ana = data;
        LOG("%s \n", ana->message);
    }

    if(!p_result.success){
        parser_result_delete(&p_result);
        goto parse_ast_failed;
    }

    // 输出AST结构
    if(p_result.root){
        LOG("\nAST Structure:\n");
        print_ast(p_result.root, 0);
    }

    // 如果有格式化需求进行格式化
    if(p_result.root){
        if(output_file_path){
            FILE * out_fp = fopen(output_file_path, "w");
            if(!out_fp){
                fprintf(stderr, "Fatal Error: unable to open output file \"%s\" for writing.\n", output_file_path);
            }else{
                format_ast(p_result.root, out_fp);
                fclose(out_fp);
                LOG("\nFormatted code written to \"%s\"\n", output_file_path);
            }
        }else{
            LOG("\nFormatted Code:\n```c\n");
            format_ast(p_result.root, stdout);
            LOG("```\n");
        }
    }

    parser_result_delete(&p_result);

parse_ast_failed:
parse_tokens_failed:
    vec_delete(&tokens);
    free(file_buffer);
    terminate_tokenizer();
    file_buffer = NULL;
    return 0;
}

#undef ERROR_CLEANUP