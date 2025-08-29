#include <d3d9.h>
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (pD3D == NULL) {
        MessageBox(NULL, "DirectX初期化失敗", "エラー", MB_OK);
        return 1;
    }

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;

    LPDIRECT3DDEVICE9 pDevice = NULL;
    HRESULT hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, 
                                    GetDesktopWindow(),
                                    D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                    &d3dpp, &pDevice);
    if (FAILED(hr)) {
        MessageBox(NULL, "デバイス作成失敗", "エラー", MB_OK);
        pD3D->Release();
        return 1;
    }

    pDevice->Release();
    pD3D->Release();
    return 0;
}