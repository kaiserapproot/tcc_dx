/* dev\lib\libdxguid.a must link from C: one GUID each from d3d9, d3d11 and
   dxgi, compared with the published values. */
#include <string.h>
#include <windows.h>
#include <d3d9.h>
#include <d3d11.h>
#include <dxgi.h>
int main(void)
{
    /* IID_IDirect3D9 = {81BDCBCA-64D4-426D-AE8D-AD0147F4275C} */
    static const unsigned char d3d9[16] = {
        0xca, 0xcb, 0xbd, 0x81, 0xd4, 0x64, 0x6d, 0x42,
        0xae, 0x8d, 0xad, 0x01, 0x47, 0xf4, 0x27, 0x5c };
    if (memcmp(&IID_IDirect3D9, d3d9, 16)) return 1;
    /* IID_ID3D11Device = {DB6F6DDB-AC77-4E88-8253-819DF9BBF140} */
    if (IID_ID3D11Device.Data1 != 0xdb6f6ddbu) return 2;
    /* IID_IDXGIFactory = {7B7166EC-21C7-44AE-B21A-C9AE321AE369} */
    if (IID_IDXGIFactory.Data1 != 0x7b7166ecu) return 3;
    return 0;
}
