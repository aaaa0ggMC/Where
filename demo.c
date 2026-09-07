/**
 * @file demo.c
 * @author aaaa0ggmc (lovelinux@yslwd.eu.org)
 * @brief 简单演示文件
 * @version 5.0
 * @date 2026-09-07
 * 
 * @copyright Copyright (c) 2026
 * 
 */

/// 预处理指令 ，保持0缩进
#include <stdio.h>
#include "custom_header.h"
#define MAX_BUFFER 1024
#define PI 3.14159f
#define ENABLE_FEATURE 1
#undef OBSOLETE_FLAG

/// 全局变量声明与多变量合并
/// int, float, double, char, void, long
int g_counter = 0, g_status = 1;
float g_ratio = 1.0f, g_scale = 0.1e2;
double g_epsilon = 1.5e-4;
char g_tag = 'V';
long g_large = 100000;

/// 函数声明
int calculate_sum(int a, int b);
void log_message(int code, float val);

/// 函数定义
int process_algorithm(int base, float factor){
    int dec_num = 42; // 十进制整数
    int hex_num = 0x2A; // 十六进制整数
    int oct_num = 052; // 八进制整数
    int bin_num = 0b101010; // 二进制整数
    float float_val = 3.14f; // 浮点数带单精度后缀
    double sci_val = 0.25e3; // 科学计数法浮点数
    char ch = 'A'; // 字符常量
    char newline = '\n'; // 转义字符常量

    /// 表达式与运算符优先级层级
    // 算术运算: +, -, *, /, %
    int math_res = (dec_num + hex_num * 2) / (oct_num % 10);

    // 位运算: &, ^, |, ~
    int bit_res = (dec_num & 0xFF) | (~bin_num ^ 1);

    // 关系与判等运算: <, <=, >, >=, ==, !=
    // 逻辑运算: &&, ||, !
    int logic_res = !(base < 0) && (factor >= 1.0f || base == 42) && (hex_num != oct_num);

    // 前置与后置自增自减: ++, --
    ++base;
    base++;
    --dec_num;
    dec_num--;

    // 数组下标运算与函数调用
    int item = base[0];
    int call_res = calculate_sum(base, math_res);

    // 条件语句
    if(base > 100){
        base = base - 10;
    }else if(base == 100){
        base = 0;
    }else{
        base = base + 10;
    }

    // while循环与break
    while(math_res > 0){
        math_res--;
        if(math_res == 5){
            break;
        }
    }

    // do-while循环与continue
    do{
        base = base + 1;
        if(base % 2 == 0){
            continue;
        }
    }while(base < 20);

    // for循环
    for(int idx = 0; idx < 10; ++idx){
        dec_num = dec_num + idx;
    }

    // switch控制流
    switch(dec_num){
        case 1:
            base = base + 1;
            break;
        case 2: {
            base = base + 2;
            break;
        }
        default: {
            base = 0;
            break;
        }
    }

    // 多层嵌套代码块与局部预处理指令
    {
        int level1 = 10;
        {
            // 这个依据我的习惯(或者说是我以前看过的标准上这么说的，建议预处理指令0缩进)
            #define INNER_THRESHOLD 64
            int level2 = 20;
            if(level2 > INNER_THRESHOLD){
                level2 = INNER_THRESHOLD;
            }
        }
    }

    // goto语句与标签定义
    goto skip_step;
    base = base + 999;
skip_step:

    // return语句
    return base + dec_num;
}

// void返回类型函数定义与空return
void log_message(int code, float val){
    printf("Code: %d, Value: %f\n", code, val);
    return;
}
