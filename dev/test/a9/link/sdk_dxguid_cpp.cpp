// dev\lib\libdxguid.a must link from C++ as well: the headers declare the
// GUIDs with `extern "C" const GUID name;` (a declaration, [dcl.link]/7).
#include <windows.h>
#include <d3d9.h>
#include <dxgi.h>
int main()
{
    if (IID_IDirect3D9.Data1 != 0x81bdcbcau) return 1;
    if (IID_IDXGIFactory.Data1 != 0x7b7166ecu) return 2;
    return 0;
}
