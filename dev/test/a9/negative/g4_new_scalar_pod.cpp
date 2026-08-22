// G4 negative: scalar POD `new` is out of scope (zero occurrences in
// CPPUnit).  Accepting it would mean silently choosing between
// default-initialization (`new int`, indeterminate) and
// value-initialization (`new int()`, zero) - reject instead.
int main()
{
    int* p;
    p = new int;
    return *p;
}
