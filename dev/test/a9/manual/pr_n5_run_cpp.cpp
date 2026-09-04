extern "C" int puts(const char *);
extern "C" int atexit(void (*)(void));
#ifdef PR_N5_EXIT
extern "C" void exit(int);
#endif

struct Global {
    Global()
    {
        puts("GC");
    }

    ~Global()
    {
        puts("GD");
    }
};

Global global_object;

struct Local {
    Local()
    {
        puts("LC");
    }

    ~Local()
    {
        puts("LD");
    }
};

Local &get_local()
{
    static Local local_object;
    return local_object;
}

static void bye(void)
{
    puts("A");
}

int main()
{
    if (atexit(bye) != 0)
        return 1;
    get_local();
    puts("M");
#ifdef PR_N5_EXIT
    exit(0);
#else
    return 0;
#endif
}
