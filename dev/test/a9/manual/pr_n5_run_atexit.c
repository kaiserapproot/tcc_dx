extern int puts(const char *);
extern int atexit(void (*)(void));

static void first(void)
{
    puts("A1");
}

static void second(void)
{
    puts("A2");
}

static void fini(void) __attribute__((destructor));

static void fini(void)
{
    puts("F");
}

int main(void)
{
    if (atexit(first) != 0)
        return 1;
    if (atexit(second) != 0)
        return 2;
    puts("M");
    return 0;
}
