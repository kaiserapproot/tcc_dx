// NEGATIVE test: must FAIL to compile.
// char * to const char * needs a temporary, which a non-const reference
// cannot bind ([dcl.init.ref]).  TCC bound PC & to the char * itself.
typedef const char *PC;

int main()
{
    char a[] = "abc";
    char *p = a;
    PC &r = p;          // expected error: a char * lvalue for PC &
    return *r;
}
