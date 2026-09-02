#include <stdlib.h>

static int allocs;
static int frees;
static int failures;

struct Owner {
    int *p;

    Owner() { p = (int *)malloc(sizeof(int)); *p = 7; ++allocs; }
    Owner(const Owner& other) {
        p = (int *)malloc(sizeof(int));
        *p = *other.p;
        ++allocs;
    }
    Owner &operator=(const Owner& other) {
        int *old = p;
        p = (int *)malloc(sizeof(int));
        *p = *other.p;
        ++allocs;
        ++frees;
        free(old);
        return *this;
    }
    int read() { return *p; }
    ~Owner() { ++frees; free(p); }
};

Owner make_owner()
{
    Owner value;
    *value.p = 7;
    return value;
}

void consume(Owner value)
{
    if (*value.p != 7)
        failures++;
}

int main()
{
    {
        Owner assigned;
        int method_value;

        {
            Owner result = make_owner();
            if (*result.p != 7)
                return 1;
        }
        consume(make_owner());
        assigned = make_owner();
        if (*assigned.p != 7)
            return 2;
        method_value = make_owner().read();
        if (method_value != 7)
            return 3;
        if (allocs != frees + 1)
            return 4;
    }
    if (failures != 0)
        return 5;
    return allocs == frees ? 0 : 6;
}
