#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// 帮助内容
#include <formatter/help_message.h>
#include <formatter/tokenizer.h>

/// 预留的token解析大小
#define RESERVE_TOKENS 1024
#define ERROR_CLEANUP {puts(help_message);exit(-1);}
#define LOG(...) printf(__VA_ARGS__)
#define LOG_SV(SV) sv_puts(SV);


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
    if(parse_tokens(input_sv, &tokens)) goto parse_tokens_failed;
    
    for(
        void * data = vec_begin(&tokens); 
        data != vec_end(&tokens); 
        data = vec_next(&tokens,data)
    ){
        Token * token = (Token *)data;
        LOG("Got token \"");
        LOG_SV(token->data);
        LOG("\" \n");
    }

    // 语法分析

    // 输出AST结构

    // 如果有格式化需求进行格式化

parse_ast_failed:

parse_tokens_failed:
    vec_delete(&tokens);
    free(file_buffer);
    file_buffer = NULL;
    return 0;
}

#undef ERROR_CLEANUP