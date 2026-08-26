// G6 regressions (plan-mandated):
// - `delete (B*)0` with a VIRTUAL dtor: the NULL check must come BEFORE
//   the vptr read (G4's test only covered the non-virtual path).
// - non-virtual dtor and POD deletes keep the direct G4 path, which never
//   touches a vptr (mixing them into the G6 path would read a vptr that
//   does not exist).
int g_dtor;
struct V {
    virtual ~V() { g_dtor = g_dtor + 1; }
};
struct P {
    int v;
    ~P() { g_dtor = g_dtor + 10; }          /* non-virtual, no vptr */
};
int main()
{
    V* nv;
    P* p;
    int* raw;

    g_dtor = 0;
    nv = 0;
    delete nv;                  /* virtual dtor + NULL: complete no-op */
    if (g_dtor != 0)
        return 1;

    p = new P();
    if (sizeof(P) != 4)         /* really no vptr */
        return 2;
    delete p;                   /* direct dtor + free(p) */
    if (g_dtor != 10)
        return 3;

    raw = new int[3];
    raw[0] = 1;
    delete[] raw;               /* POD path untouched */

    nv = new V();
    delete nv;                  /* virtual path, exact static type */
    if (g_dtor != 11)
        return 4;
    return 0;
}
