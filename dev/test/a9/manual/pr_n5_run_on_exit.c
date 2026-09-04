extern int puts(const char *);
extern int on_exit(void *, void *);

static int token;

static void callback(int ret, void *arg)
{
    if (ret == 0 && arg == &token)
        puts("ON");
    else
        puts("ON_BAD");
}

int main(void)
{
    if (on_exit((void *)callback, &token) != 0)
        return 1;
    puts("M");
    return 0;
}
