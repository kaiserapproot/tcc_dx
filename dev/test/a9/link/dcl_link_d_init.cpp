// [dcl.link]/7 case D: an initializer makes the single-declaration form a
// definition.
extern "C" int dcl_d = 123;
int main() { return dcl_d != 123; }
