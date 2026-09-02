// An unrelated operator=(int) on a member does not suppress that member's
// implicit copy-assignment operator.  The outer class remains byte-copy safe.
struct M {
    int value;
    M() { value = 0; }
    M& operator=(int n) { value = n; return *this; }
};

struct Holder {
    M member;
    int tag;
    Holder() { tag = 0; }
};

int main()
{
    Holder source;
    Holder dest;

    source.member.value = 17;
    source.tag = 23;
    dest = source;
    if (dest.member.value != 17 || dest.tag != 23)
        return 1;
    dest.member = 31;
    return dest.member.value == 31 ? 0 : 2;
}