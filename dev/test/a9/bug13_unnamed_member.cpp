// BUG-13: a member function declared with an UNNAMED parameter crashed tcc
// when emitted (gen_function -> gfunc_prolog -> sym_push(v & ~SYM_FIELD)
// indexed table_ident[0 - TOK_IDENT], a negative index).  Free functions
// reject unnamed params at the definition check, but member bodies bypass
// it, so the crash only reproduced for members.  Now unnamed params get a
// fresh anonymous token id at registration and compile/run cleanly.
class C {
public:
    int v;
    // unnamed int parameter (value is unused inside the body)
    int add10(int) { return v + 10; }
};

int main()
{
    C a;
    a.v = 5;
    return a.add10(999) - 15;   // 5 + 10 - 15 = 0 (arg ignored)
}
