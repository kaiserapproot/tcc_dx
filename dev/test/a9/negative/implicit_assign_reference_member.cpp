struct RefMember {
    int& value;
    RefMember(int& source) : value(source) {}
};

int main()
{
    int a = 1;
    int b = 2;
    RefMember left(a);
    RefMember right(b);

    right = left;
    return right.value;
}