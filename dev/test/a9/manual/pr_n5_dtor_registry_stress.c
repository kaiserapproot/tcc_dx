extern int puts(const char *);
extern void __tcc_cpp_register_dtor(void (*)(void));

#ifndef PR_N5_STRESS_COUNT
#define PR_N5_STRESS_COUNT 4096
#endif

static int dtor_count;

static void bump(void)
{
    ++dtor_count;
}

static void check(void)
{
    if (dtor_count == PR_N5_STRESS_COUNT)
        puts("STRESS_OK");
    else
        puts("STRESS_BAD");
}

int main(void)
{
    int i;

    __tcc_cpp_register_dtor(check);
    for (i = 0; i < PR_N5_STRESS_COUNT; ++i)
        __tcc_cpp_register_dtor(bump);
    return 0;
}
