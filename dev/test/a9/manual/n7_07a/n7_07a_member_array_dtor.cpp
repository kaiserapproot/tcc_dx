// N7-07A TCC-3: class member array with non-trivial destructor audit.
struct M {
    static int live;

    M()
    {
        ++live;
    }

    ~M()
    {
        --live;
    }
};

int M::live;

struct X {
    M m[3];
};

int main()
{
    {
        X x;
    }

    return M::live == 0 ? 0 : 1;
}
