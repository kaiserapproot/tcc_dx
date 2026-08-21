// G2 guard: in C mode "friend" is demoted to a plain identifier by the
// A-2 keyword demotion, so this must keep compiling and running.
int main(void)
{
    int friend = 5;
    friend++;
    return friend - 6;
}
