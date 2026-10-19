/* multi-file C link: extern function, extern variable, static library */
extern int b_counter;
int b_add(int x);
int c_mul(int x);   /* from the static library */
int main(void)
{
    if (b_add(5) != 105) return 1;
    if (b_counter != 1) return 2;
    if (c_mul(6) != 42) return 3;
    return 0;
}
