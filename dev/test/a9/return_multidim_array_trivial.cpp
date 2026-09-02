struct P {
    int x;
};

struct H {
    P a[2][3];
};

H make_trivial(H& source)
{
    return source;
}

int main()
{
    H source = {{{{1}, {2}, {3}}, {{4}, {5}, {6}}}};
    H result = make_trivial(source);
    return result.a[1][2].x == 6 ? 0 : 1;
}
