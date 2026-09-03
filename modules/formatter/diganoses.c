#include <formatter/diagnoses.h>
#include <stdio.h>
#include <stdlib.h>

void sd_delete(stage_diagnoses * result){
    // 删除消息缓存
    for(
        stage_diagnosis * diagnosis = vec_begin(result);
        diagnosis != vec_end(result);
        diagnosis = vec_next(result, diagnosis)
    ){
        if(diagnosis->message) free(diagnosis->message);
    }
    // 删除vector
    vec_delete(result);
}

int sd_printf(stage_diagnoses * diagnoses, char * fmt,...){
    va_list ap;
    va_start(ap,fmt);

    stage_diagnosis* info = vec_push_back(diagnoses);

    va_list ap2;
    va_copy(ap2, ap);
    int length = vsnprintf(NULL, 0 , fmt, ap2);
    va_end(ap2);

    info->message = malloc(length + 1);
    vsnprintf(info->message, length, fmt, ap);
    va_end(ap);

    return length;
}