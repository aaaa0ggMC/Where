#include <formatter/vector.h>
#include <formatter/helper.h>
#include <stdlib.h>

vector vec_new(int sizeof_data, int reserve_size){
    vector vec = {
        .data = reserve_size ? malloc(sizeof_data * reserve_size) : NULL,
        .sizeof_data = sizeof_data,
        .size = 0,
        .capacity = reserve_size
    };
    return vec;
}

void* vec_push_back(vector * vec){
    if(vec->size == vec->capacity){
        // 扩容，同时防止 0*2 = 0
        int new_capacity = imax(4, vec->capacity) * 2;
        void * new_data = realloc(vec->data, vec->sizeof_data * new_capacity);
        vec->data = new_data;
        vec->capacity = new_capacity;
    }

    // 返回 vec->data + vec->size ， 之后size加1
    return vec->data + ((vec->size)++);
}

void vec_delete(vector * vec){
    if(vec->data) free(vec->data);
    // 这个相当于重置
    *vec = vec_new(0, 0);
}

void* vec_begin(vector * vec){
    return vec->data;
}

void* vec_end(vector * vec){
    return vec->data + vec->size;
}

void * vec_next(vector * vec,void* data){
    char * ch_data = (char *)data;
    ch_data += vec->sizeof_data;
    return (void*)ch_data;
}