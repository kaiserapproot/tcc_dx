/* ラベル/goto のリグレッション（block() のラベル検出変更の確認） */
#include <stdio.h>

int main(void)
{
    int i = 0, n = 0;

loop:                       /* 通常ラベル */
    i = i + 1;
    if (i < 5)
        goto loop;

    n = i > 3 ? 10 : 20;    /* 三項演算子 */

    {
        int x = 0;
    inner:
        x = x + 1;
        if (x < 3)
            goto inner;
        n = n + x;
    }

    switch (n) {
    case 13:
        printf("case13\n");
        break;
    default:
        printf("default n=%d\n", n);
        break;
    }

    printf("i=%d n=%d\n", i, n);
    printf("OK\n");
    return 0;
}
