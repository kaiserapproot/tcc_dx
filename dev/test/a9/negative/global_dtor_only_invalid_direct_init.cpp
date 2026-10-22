/* A class with a destructor but no user constructor has no constructor
   taking an int, so `A global_a(5);` is ill-formed (g++ -std=c++98
   rejects it).  The startup thunk called no constructor and the object
   was only destroyed at exit, as if it had been constructed. */
struct A {
    int value;
    ~A() {}
};

A global_a(5);

int main()
{
    return 0;
}
