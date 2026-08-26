// G3 negative: a class-scope typedef must NOT leak into the enclosing
// (file) scope as an unqualified name.
class A {
public:
    typedef int T;
};
T x;
