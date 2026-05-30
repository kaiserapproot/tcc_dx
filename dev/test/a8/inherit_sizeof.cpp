class Base { public: int x; };
class Derived : public Base { public: int y; };
int main() {
    if (sizeof(Derived) != 2 * sizeof(int))
        return 1;
    return 0;
}
