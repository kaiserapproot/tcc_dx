/* G4 guard: in C mode `new` and `delete` are ordinary identifiers (the
   A-2 keyword demotion), so this must keep compiling and running. */
int main(void)
{
    int new = 3;
    int delete = 4;
    new = new + delete;
    return new - 7;
}
