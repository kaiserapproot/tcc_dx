struct A { int value; };
struct Left : public A { int left; };
struct Right : public A { int right; };
struct Diamond : public Left, public Right { int diamond; };

int read_a(const A& value)
{
    return value.value;
}

int main()
{
    Diamond value;
    return read_a(value);
}