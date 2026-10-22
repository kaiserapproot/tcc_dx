/* A class array declared in a for-init statement is a local automatic
   object: its elements are default constructed (decl() receives the
   for-init declaration as VT_JMP, and the element construction tested
   VT_LOCAL only, so the elements were left unconstructed). */
struct C { int v; C() : v(5) {} };

int main()
{
    int s = 0;
    int k;

    for (C c[3]; s == 0; ) {
        for (k = 0; k < 3; k++)
            s += c[k].v;
    }
    return s == 15 ? 0 : 1;
}
