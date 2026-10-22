/* A class array declared in a for-init statement is a local automatic
   array, whose element destruction is unsupported: reject it like a block
   scope one (decl() receives the for-init declaration as VT_JMP, and the
   check tested VT_LOCAL only, so the destructors were silently skipped). */
struct A { ~A() { } };

int main()
{
    for (A a[2]; 0; ) {
    }
    return 0;
}
