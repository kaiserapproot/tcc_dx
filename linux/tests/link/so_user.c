/* linked against the .so at link time (-L -l), run with LD_LIBRARY_PATH */
int so_get(int x);
extern int so_value;
int main(void)
{
    if (so_get(2) != 12) return 1;
    so_value = 0;
    return so_get(2) == 2 ? 0 : 2;
}
