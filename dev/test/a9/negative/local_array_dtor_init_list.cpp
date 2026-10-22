/* A local class array with an initializer list is not constructed by
   generated code, but its elements still need destruction at scope exit,
   which is not supported: reject it instead of skipping the destructors. */
struct A { int v; ~A() { } };

int main()
{
    A la[2] = { { 1 }, { 2 } };
    return la[0].v - 1;
}
