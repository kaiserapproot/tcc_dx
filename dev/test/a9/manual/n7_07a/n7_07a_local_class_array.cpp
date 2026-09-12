// N7-07A TCC-1: top-level local class array default construction audit.
struct M {
    static int ctor_count;

    M()
    {
        ++ctor_count;
    }
};

int M::ctor_count;

int main()
{
    M a[4];

    return M::ctor_count == 4 ? 0 : 1;
}
