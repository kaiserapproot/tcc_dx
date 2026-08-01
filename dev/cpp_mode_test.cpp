/* C++ モード検証: __cplusplus / extern "C" / class */
#include <stdio.h>

#ifndef __cplusplus
#error "__cplusplus が定義されていません"
#endif

extern "C" {
    int add_c(int a, int b);
    extern int c_linked_var;
}

extern "C" int mul_c(int a, int b);

int add_c(int a, int b) { return a + b; }
int mul_c(int a, int b) { return a * b; }
int c_linked_var = 55;

class Obj {
public:
    int v;
    Obj(int x) { v = x; }
};

int main(void)
{
    class Obj o(9);
    printf("cplusplus=%ld\n", (long)__cplusplus);
    printf("add=%d mul=%d var=%d obj=%d\n",
        add_c(2, 3), mul_c(4, 5), c_linked_var, o.v);
    printf("OK\n");
    return 0;
}
