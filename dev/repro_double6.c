#include <stdio.h>
void test6(double a, double b, double c, double d, double e, double f) {
    printf("a=%f b=%f c=%f d=%f e=%f f=%f\n", a, b, c, d, e, f);
}
int main(void) {
    float fa=-15.96f, fb=15.96f, fc=-1.815f, fd=21.025f, fe=-45.72f, ff=45.64f;
    test6(fa, fb + 0.0f, fc, fd, fe, ff);
    return 0;
}
