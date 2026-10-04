// [dcl.link]/7 case A: the single-declaration form is a DECLARATION.
// The definition lives in dcl_link_a_def.c; on its own this TU must not link.
extern "C" int dcl_a;
int main() { return dcl_a != 7; }
