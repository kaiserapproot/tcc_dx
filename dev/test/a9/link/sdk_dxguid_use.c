/* IID_IDXGIFactory is not defined by sdk_dxguid_own.c, so it has to come
   from libdxguid.a while that TU keeps its own INITGUID definitions. */
#include <windows.h>
#include <dxgi.h>
int sdk_dxguid_own_ok(void);
int main(void)
{
    return (sdk_dxguid_own_ok() && IID_IDXGIFactory.Data1 == 0x7b7166ecu) ? 0 : 1;
}
