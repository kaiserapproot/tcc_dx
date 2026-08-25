// BUG-46: an implicit copy through new must recurse through two class
// members and still invoke the innermost copy ctor.  Mid has an unrelated
// user constructor, so its implicitly declared copy constructor must still
// be synthesized.  A flat copy leaves both objects pointing at storage_a;
// memberwise repair changes the nested Leaf to storage_b.
int storage_a;
int storage_b;

struct Leaf {
    int *p;
    Leaf() { p = &storage_a; }
    Leaf(const Leaf& other) {
        p = &storage_b;
        *p = *other.p;
    }
};

struct Mid {
    Leaf leaf;
    Mid(int value) { leaf.p = &storage_a; }
};

struct Top {
    Mid mid;
};

int main()
{
    Top source;
    Top *copy;

    storage_a = 41;
    storage_b = 0;
    source.mid.leaf.p = &storage_a;
    copy = new Top(source);
    if (copy->mid.leaf.p == source.mid.leaf.p)
        return 1;
    if (copy->mid.leaf.p != &storage_b)
        return 2;
    if (*copy->mid.leaf.p != 41)
        return 3;
    delete copy;
    return 0;
}
