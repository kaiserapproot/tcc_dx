struct M {
    M(int required, int optional = 9);
};

M::M(int required, int optional)
{
    (void)required;
    (void)optional;
}

int main()
{
    M a[4];
    return 0;
}
