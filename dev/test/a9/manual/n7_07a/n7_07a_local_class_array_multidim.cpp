// N7-07A TCC-2: multidim top-level local class array (measurement only).
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
    M a[2][3];

    return M::ctor_count == 6 ? 0 : 1;
}
