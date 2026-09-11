struct M {
    int x;

    M& operator=(const M& rhs)
    {
        x = rhs.x;
        return *this;
    }
};

struct V {
    M m[4];
};

int main(void)
{
    char storage_a[128];
    char storage_b[128];
    V *a;
    V *b;
    int i;

    // FEAT-4G: class-type member arrays cannot be default-constructed yet.
    // Use raw storage + pointer so only copy assignment is exercised (N7-03).
    a = (V *)storage_a;
    b = (V *)storage_b;
    for (i = 0; i < 4; i++)
        b->m[i].x = i + 10;
    *a = *b;
    return a->m[0].x == 10 && a->m[3].x == 13 ? 0 : 1;
}
