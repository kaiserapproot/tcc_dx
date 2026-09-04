extern "C" int puts(const char *);
extern "C" int get_a();
extern "C" int get_b();

int main()
{
    if (get_a() != 1 || get_b() != 2)
        return 1;
    puts("END");
    return 0;
}
