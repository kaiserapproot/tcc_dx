// G-OP negative: operator-> returning a non-pointer must be rejected
// (single application only - no C++-style recursive operator->).
struct Q {
    int x;
};
struct Bad {
    Q q;
    Q operator->() { return q; }
};
int main()
{
    Bad b;
    return b->x;
}
