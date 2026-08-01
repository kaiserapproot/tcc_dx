/* C モード検証: __cplusplus が未定義であること */
#include <stdio.h>

#ifdef __cplusplus
#error "C ファイルで __cplusplus が定義されています"
#endif

struct pair {
    int a;
    int b;
};

int main(void)
{
    struct pair p;
    p.a = 10;
    p.b = 7;
    printf("sum=%d\n", p.a + p.b);
    printf("OK\n");
    return 0;
}
