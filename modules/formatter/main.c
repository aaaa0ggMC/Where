#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// 帮助内容
#include <formatter/help_message.h>


#define ERROR_CLEANUP {puts(help_message);exit(-1);}
#define LOG(...) printf(__VA_ARGS__)


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

    if(!input_file){
        fprintf(stderr, "Fatal Error: unable to open file \"%s\"! \n", input_file_path);
        ERROR_CLEANUP;
    }
    {
        fseek(input_file,0,SEEK_END);

        long buffer_size = ftell(input_file);
        file_buffer = (char *)malloc(sizeof(char) * (buffer_size + 1));
        fseek(input_file,0,SEEK_SET);
        
        fread(file_buffer,sizeof(char),buffer_size,input_file);
        // 确保是一个合法的字符串
        file_buffer[buffer_size] = '\0';
    
        LOG("Read input:\n```c\n%s\n```\n", file_buffer);
    }
    fclose(input_file);
    // 后面基本不会出现致命错误了，因此这里不需要goto，保证都能正常释放就行

    // 词法分析

    // 语法分析

    // 输出AST结构

    // 如果有格式化需求进行格式化

    free(file_buffer);
    file_buffer = NULL;
    return 0;
}

#undef ERROR_CLEANUP