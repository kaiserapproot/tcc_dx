// FEAT-6B-P2 (manual, must FAIL to compile):
// explicit this-> member write inside a const method.
class Box {
public:
    int v;
    void bad() const { this->v = 2; }   // error: assignment of read-only location
};

int main() { Box b; b.bad(); return 0; }
