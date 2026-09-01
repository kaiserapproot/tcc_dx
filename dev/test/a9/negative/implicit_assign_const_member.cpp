struct ConstMember {
    const int value;
    ConstMember(int source) : value(source) {}
};

int main()
{
    ConstMember left(1);
    ConstMember right(2);

    right = left;
    return right.value;
}