class Base {
public:
    int x;
};

class Derived : public Base {
public:
    int y;
};

int main() {
    Derived d;
    d.x = 1;
    d.y = 2;
    return d.x + d.y - 3;
}
