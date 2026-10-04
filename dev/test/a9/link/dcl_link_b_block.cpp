// [dcl.link]/7 case B: a declaration inside the braces is NOT "directly
// contained" in the linkage-specification, so this is a DEFINITION and the
// TU links on its own.
extern "C" {
    int dcl_b;
}
int main()
{
    dcl_b = 123;
    return dcl_b != 123;
}
