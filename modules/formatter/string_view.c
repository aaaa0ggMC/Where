#include <formatter/string_view.h>
#include <math.h>

char * sv_begin(string_view sv){
    if(*sv.buffer) return (*sv.buffer + sv.begin);
    else return ""; 
}

char * sv_end(string_view sv){
    if(*sv.buffer) return (*sv.buffer + sv.begin + sv.length);
    else return ""; 
}

int sv_length(string_view sv){
    if(*sv.buffer) return sv.length;
    else return 0;
}

string_view sv_build(char ** str, int subbegin , int length){
    return {
        .buffer = str,
        .begin = subbegin,
        .length = length 
    };
}

string_view sv_substr(string_view sv,int begin, int length){
    return {
        .buffer = sv.buffer,
        .begin = sv.begin + begin,
        .length = imin(sv.length - begin, length);
    };
}

int sv_equals(string_view a, string_view b){
    if(a.length != b.length) return 0;
    return !strncmp(sv_begin(a),sv_begin(b),imin(a.length,b.length));
}