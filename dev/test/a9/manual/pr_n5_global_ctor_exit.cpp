extern "C" int puts(const char *);
extern "C" void exit(int);

struct First {
    First()
    {
        puts("AC");
    }

    ~First()
    {
        puts("AD");
    }
};

First first_object;

struct Stop {
    Stop()
    {
        puts("BC");
        exit(7);
    }

    ~Stop()
    {
        puts("BD");
    }
};

Stop stop_object;

int main()
{
    puts("MAIN");
    return 0;
}
