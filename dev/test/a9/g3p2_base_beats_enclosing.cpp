// G3 P2 (rev.3 Blocker 2-A): a base-class typedef must beat the
// ENCLOSING class's typedef of the same name - the class scope
// (self + bases) is searched to completion before going outward.
// Instantiation goes through the hoisted tag (O::I itself is P3).
struct B {
    typedef char T;
};
struct O {
    typedef int T;
    struct I : B {
        T x;
    };
};
int main()
{
    I i;
    i.x = 'q';
    return sizeof(i.x) == sizeof(char) ? 0 : 1;
}
