/**
 * @file diagnoses.h
 * @author aaaa0ggmc (lovelinux@yslwd.eu.org)
 * @brief 代码报错以及回传
 * @version 5.0
 * @date 2026-09-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef DIAGNOSES_H_INCLUDED
#define DIAGNOSES_H_INCLUDED
#include <formatter/vector.h>
#include <stdarg.h>

/// 每个Token所处的行号列号
typedef struct {
    int row;
    int col;
} TokenLocation;

typedef struct{
    TokenLocation location;
    // 动态大小的消息
    char * message;
} stage_diagnosis;

typedef vector stage_diagnoses;

void sd_delete(stage_diagnoses * result);
int sd_printf(stage_diagnoses * diagnoses, char * fmt,...);

#endif