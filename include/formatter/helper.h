/**
 * @file helper.h
 * @author aaaa0ggmc (lovelinux@yslwd.eu.org)
 * @brief 一些帮助函数，内联
 * @version 5.0
 * @date 2026-09-01
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef HELPER_H_INCLUDED
#define HELPER_H_INCLUDED

inline int imax(int a,int b){
    return (a > b) ? a : b;
}

inline int imin(int a,int b){
    return (a < b) ? a : b;
}

#endif