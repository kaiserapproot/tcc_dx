struct M {
    ~M() {}
};

void make_array()
{
    static M values[2];
}

int main()
{
    return 0;
}