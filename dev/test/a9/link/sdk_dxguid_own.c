/* This TU defines the windows.h / d3d9 GUIDs itself through INITGUID.  The
   library's weak definitions must not collide with them. */
#include <initguid.h>
#include <windows.h>
#include <d3d9.h>
int sdk_dxguid_own_ok(void) { return IID_IDirect3D9.Data1 == 0x81bdcbcau; }
