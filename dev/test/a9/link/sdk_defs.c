/* dev\lib\msimg32.def and d3dcompiler_47.def must resolve their imports. */
#include <windows.h>
#include <d3dcompiler.h>
int main(void)
{
    void *a = (void *)AlphaBlend;
    void *b = (void *)D3DCompile;
    return (a != 0 && b != 0) ? 0 : 1;
}
