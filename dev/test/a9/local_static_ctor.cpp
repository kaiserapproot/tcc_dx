static int ctor_count;

struct Probe {
    int value;

    Probe()
    {
        value = 7;
        ctor_count++;
    }

    Probe(int a, int b)
    {
        value = a + b;
        ctor_count++;
    }
};

struct Defaulted {
    int value;

    Defaulted(int n = 13)
    {
        value = n;
        ctor_count++;
    }
};

Probe *maybe_default(int enter)
{
    if (enter) {
        static Probe item;
        return &item;
    }
    return 0;
}

Probe *first_arg_static(int a, int b)
{
    static Probe item(a, b);
    return &item;
}

Probe *second_arg_static(int a, int b)
{
    static Probe item(a, b);
    return &item;
}

int comma_static_values()
{
    static Probe left, right;
    return left.value + right.value;
}

Defaulted *default_arg_static()
{
    static Defaulted item;
    return &item;
}

int main()
{
    Probe *p;
    Probe *q;
    Probe *r;

    if (ctor_count != 0)
        return 1;
    if (maybe_default(0) != 0 || ctor_count != 0)
        return 2;

    p = maybe_default(1);
    if (!p || p->value != 7 || ctor_count != 1)
        return 3;
    p->value = 9;
    if (maybe_default(1) != p || p->value != 9 || ctor_count != 1)
        return 4;

    q = first_arg_static(10, 20);
    if (!q || q->value != 30 || ctor_count != 2)
        return 5;
    if (first_arg_static(40, 50) != q || q->value != 30 || ctor_count != 2)
        return 6;

    r = second_arg_static(1, 2);
    if (!r || r == q || r->value != 3 || ctor_count != 3)
        return 7;
    if (second_arg_static(8, 9) != r || r->value != 3 || ctor_count != 3)
        return 8;

    if (comma_static_values() != 14 || ctor_count != 5)
        return 9;
    if (comma_static_values() != 14 || ctor_count != 5)
        return 10;

    if (default_arg_static()->value != 13 || ctor_count != 6)
        return 11;
    if (default_arg_static()->value != 13 || ctor_count != 6)
        return 12;

    return 0;
}
