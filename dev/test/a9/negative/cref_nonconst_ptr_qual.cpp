// NEGATIVE test: must FAIL to compile.
// A char * binds directly to const PC & (const char * const &), but not to
// PC &: char ** to const char ** is not a qualification conversion
// ([conv.qual]), since storing a const char * through the reference would
// put it in the char *.  A temporary does not help either, as the reference
// is not const.  TCC bound PC & to the char * itself.
typedef const char *PC;

int main()
{
    char a[] = "abc";
    char *p = a;
    PC &r = p;          // expected error: a char * lvalue for PC &
    return *r;
}
