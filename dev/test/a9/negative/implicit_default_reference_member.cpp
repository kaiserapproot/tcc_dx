struct M {
    int value;
};

struct R {
    M& ref;
};

int main()
{
    R source;
    return source.ref.value;
}
