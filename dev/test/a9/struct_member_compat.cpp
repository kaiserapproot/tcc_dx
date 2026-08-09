struct Value
{
    int number;

    Value() : number(7) {}
    Value(int input) : number(input) {}
    Value(const Value &other) : number(other.number) {}
    ~Value() {}

    Value &operator=(const Value &other)
    {
        number = other.number;
        return *this;
    }
};

void touch_value(Value *value)
{
    value->number = 9;
}

void use_local_value()
{
    Value value;
    touch_value(&value);
}

Value make_value(int input)
{
    Value value(input);
    return value;
}

int main()
{
    Value value = make_value(7);
    return value.number - 7;
}
