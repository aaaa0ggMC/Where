#include <stdio.h>
#define MAX 100

int i, j;

float a = 1.0f;

float b = 0.1e2;

int fun(int a, float b){
    int m;
    if(a > b) { m = a; }
    else { m = b; }
    {
#define INNER_DEF 1
        int nested = 10;
    }
    return m;
}
float x, y;
