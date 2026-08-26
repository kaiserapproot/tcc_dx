// G-CONV: implicit converting-constructor application at the four sites
// CPPUnit uses - return statement, member-initializer, assignment, and
// argument passing (including binding to const T&) - with run-time value
// checks.  Shapes match SimpleList::Iterator and SimpleString.
struct Node {
    int val;
    Node* next;
};
struct It {
    It(Node* n = 0) : node(n) {}        /* converting ctor, default arg */
    Node* node;
};
struct Str {
    Str() : len(0), c0(0) {}
    Str(const char* p, unsigned n = 100) {  /* converting ctor + default */
        len = (int)n;
        c0 = p ? p[0] : 0;
    }
    int len;
    char c0;
};
struct Holder {
    Str m_name;
    Holder(const char* name) : m_name(name) {}  /* mem-init conversion */
};
static Node g_node;
static It ret_convert(Node* n)
{
    return n;                            /* return-position conversion */
}
static int take_by_value(Str s)
{
    return s.len + s.c0;
}
static int take_by_cref(const Str& s)
{
    return s.len + s.c0;
}
int main()
{
    It it;
    Str s;
    Holder h("h");

    g_node.val = 7;

    /* return position: Node* -> It */
    it = ret_convert(&g_node);
    if (it.node != &g_node || it.node->val != 7)
        return 1;

    /* assignment: Node* -> It via the converting ctor */
    it.node = 0;
    it = &g_node;
    if (it.node != &g_node)
        return 2;

    /* mem-init: const char* -> Str (default n = 100) */
    if (h.m_name.len != 100 || h.m_name.c0 != 'h')
        return 3;

    /* assignment: const char* -> Str */
    s = "xy";
    if (s.len != 100 || s.c0 != 'x')
        return 4;

    /* argument by value and by const reference */
    if (take_by_value("a") != 100 + 'a')
        return 5;
    if (take_by_cref("b") != 100 + 'b')
        return 6;

    /* explicit second argument still wins over the default */
    {
        Str t("z", 5);
        if (t.len != 5 || t.c0 != 'z')
            return 7;
    }
    return 0;
}
