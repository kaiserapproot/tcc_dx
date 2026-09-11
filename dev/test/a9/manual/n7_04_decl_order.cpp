struct M {
    static int seq;
    static int last;
    int id;

    M()
    {
        id = ++seq;
        last = id;
    }
};

int M::seq;
int M::last;

struct N {
    static int n_done;
    N() { n_done = 1; }
};

int N::n_done;

struct X {
    M a[2];
    N middle;
    M b[2];
};

int main(void)
{
    X x;

    if (M::seq != 4)
        return 1;
    if (N::n_done != 1)
        return 2;
    if (x.a[0].id != 1 || x.a[1].id != 2)
        return 3;
    if (x.b[0].id != 3 || x.b[1].id != 4)
        return 4;
    return 0;
}
