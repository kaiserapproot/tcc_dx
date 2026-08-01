/* Stage 3 エッジケース: double 引数メソッドの ABI、ローカルによるメンバ隠蔽 */
#include <stdio.h>

class Calc {
    double total;
    int count;
public:
    void reset() { total = 0.0; count = 0; }
    void add3(double a, double b, double c) { total = total + a + b + c; count = count + 3; }
    double avg() { return total / count; }
    int shadow() { int count = 7; return count; }  /* ローカルがメンバを隠す */
};

int main(void)
{
    class Calc c;
    c.reset();
    c.add3(1.5, 2.5, 6.0);      /* this=RCX, a=XMM1, b=XMM2, c=XMM3 */
    printf("avg=%.3f\n", c.avg());   /* 10.0/3 = 3.333 */
    printf("shadow=%d\n", c.shadow());
    printf("OK\n");
    return 0;
}
