#include <formatter/string_view.h>
#include <formatter/helper.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

char * sv_begin(string_view sv){
    return (sv.buffer + sv.begin);
}

char * sv_end(string_view sv){
    return (sv.buffer + sv.begin + sv.length);
}

int sv_length(string_view sv){
    return sv.length;
}

string_view sv_build(char * str, int subbegin , int length){
    string_view ret = {
        .buffer = str,
        .begin = subbegin,
        .length = length 
    };
    return ret;
}

string_view sv_substr(string_view sv,int begin, int length){
    string_view ret = {
        .buffer = sv.buffer,
        .begin = sv.begin + begin,
        .length = imin(sv.length - begin, length)
    };
    return ret;
}

int sv_equals(string_view a, string_view b){
    if(a.length != b.length) return 0;
    return !strncmp(sv_begin(a),sv_begin(b),imin(a.length,b.length));
}

void sv_puts(string_view sv){
    for(
        char * data = sv_begin(sv);
        data != sv_end(sv);
        ++data
    ){
        putchar(*data);
    }
}

char sv_at(string_view sv, int pos){
    return sv_begin(sv)[pos];
}