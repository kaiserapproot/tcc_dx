extern int puts(const char *);
extern int atexit(void (*)(void));
extern void exit(int);

static void bye(void)
{
    puts("A");
}

static void fini(void) __attribute__((destructor));

static void fini(void)
{
    puts("F");
}

int main(void)
{
    if (atexit(bye) != 0)
        return 1;
    puts("M");
    exit(7);
    return 9;
}
