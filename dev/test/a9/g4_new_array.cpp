// G4: new POD[n] / delete[] - the SimpleString.cpp shape
// (`value_type* data = new value_type[size];` with value_type = char).
class S {
public:
    typedef char value_type;
    typedef unsigned int size_type;
};
int main()
{
    S::size_type size;
    S::value_type* data;
    int* nums;
    int i;
    int sum;

    size = 8;
    data = new S::value_type[size];
    for (i = 0; i < (int)size; i++)
        data[i] = (S::value_type)('a' + i);
    if (data[0] != 'a' || data[7] != 'h')
        return 1;
    delete[] data;

    nums = new int[4];
    sum = 0;
    for (i = 0; i < 4; i++)
        nums[i] = i * 10;
    for (i = 0; i < 4; i++)
        sum = sum + nums[i];
    if (sum != 60)
        return 2;
    delete[] nums;
    return 0;
}
