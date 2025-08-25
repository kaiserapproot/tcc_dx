#include <windows.h>
#include <d3d9.h>
#include <d3dcompiler.h> // 必要なら関数宣言を自作
#include <math.h>

LPDIRECT3D9             g_pD3D = NULL;
LPDIRECT3DDEVICE9       g_pd3dDevice = NULL;
LPDIRECT3DVERTEXBUFFER9 g_pVB = NULL;
LPDIRECT3DVERTEXSHADER9 g_pVS = NULL;
HWND                    g_hWnd = NULL;

typedef struct {
    float x, y, z;
    DWORD color;
} CUSTOMVERTEX;

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)

const char* vs_code =
    "struct VS_IN { float4 pos : POSITION; float4 col : COLOR; };\n"
    "struct VS_OUT { float4 pos : POSITION; float4 col : COLOR; };\n"
    "VS_OUT main(VS_IN input) {\n"
    "  VS_OUT output;\n"
    "  output.pos = input.pos;\n"
    "  output.col = input.col;\n"
    "  return output;\n"
    "}";

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

HRESULT InitD3D(HWND hWnd)
{
    g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!g_pD3D) return E_FAIL;

    D3DPRESENT_PARAMETERS d3dpp = {0};
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

    if (FAILED(g_pD3D->lpVtbl->CreateDevice(
        g_pD3D, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice)))
        return E_FAIL;

    return S_OK;
}

HRESULT InitShader()
{
    ID3DBlob* pCode = NULL;
    ID3DBlob* pError = NULL;
    HRESULT hr = D3DCompile(
        vs_code, strlen(vs_code),
        NULL, NULL, NULL,
        "main", "vs_2_0", 0, 0,
        &pCode, &pError
    );
    if (FAILED(hr)) return hr;

    hr = g_pd3dDevice->lpVtbl->CreateVertexShader(g_pd3dDevice, (DWORD*)pCode->lpVtbl->GetBufferPointer(pCode), &g_pVS);
    pCode->lpVtbl->Release(pCode);
    if (pError) pError->lpVtbl->Release(pError);
    return hr;
}

HRESULT InitGeometry()
{
    CUSTOMVERTEX vertices[] = {
        { 0.0f,  1.0f, 0.0f, 0xFFFF0000 },
        { 1.0f, -1.0f, 0.0f, 0xFF00FF00 },
        {-1.0f, -1.0f, 0.0f, 0xFF0000FF },
    };

    if (FAILED(g_pd3dDevice->lpVtbl->CreateVertexBuffer(
        g_pd3dDevice, sizeof(vertices), 0, D3DFVF_CUSTOMVERTEX,
        D3DPOOL_DEFAULT, &g_pVB, NULL)))
        return E_FAIL;

    void* pVertices;
    if (FAILED(g_pVB->lpVtbl->Lock(g_pVB, 0, sizeof(vertices), &pVertices, 0)))
        return E_FAIL;
    memcpy(pVertices, vertices, sizeof(vertices));
    g_pVB->lpVtbl->Unlock(g_pVB);

    return S_OK;
}

void Cleanup()
{
    if (g_pVS) g_pVS->lpVtbl->Release(g_pVS);
    if (g_pVB) g_pVB->lpVtbl->Release(g_pVB);
    if (g_pd3dDevice) g_pd3dDevice->lpVtbl->Release(g_pd3dDevice);
    if (g_pD3D) g_pD3D->lpVtbl->Release(g_pD3D);
}

void Render()
{
    if (!g_pd3dDevice) return;
    g_pd3dDevice->lpVtbl->Clear(g_pd3dDevice, 0, NULL, D3DCLEAR_TARGET, 0xFF202060, 1.0f, 0);
    g_pd3dDevice->lpVtbl->BeginScene(g_pd3dDevice);

    g_pd3dDevice->lpVtbl->SetStreamSource(g_pd3dDevice, 0, g_pVB, 0, sizeof(CUSTOMVERTEX));
    g_pd3dDevice->lpVtbl->SetFVF(g_pd3dDevice, D3DFVF_CUSTOMVERTEX);

    // シェーダをセット
    g_pd3dDevice->lpVtbl->SetVertexShader(g_pd3dDevice, g_pVS);

    g_pd3dDevice->lpVtbl->DrawPrimitive(g_pd3dDevice, D3DPT_TRIANGLELIST, 0, 1);

    // シェーダ解除
    g_pd3dDevice->lpVtbl->SetVertexShader(g_pd3dDevice, NULL);

    g_pd3dDevice->lpVtbl->EndScene(g_pd3dDevice);
    g_pd3dDevice->lpVtbl->Present(g_pd3dDevice, NULL, NULL, NULL, NULL);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "DX9Shader";
    RegisterClass(&wc);

    g_hWnd = CreateWindow("DX9Shader", "DirectX9 Shader Sample",
        WS_OVERLAPPEDWINDOW, 100, 100, 640, 480, NULL, NULL, hInstance, NULL);
    ShowWindow(g_hWnd, nCmdShow);

    if (FAILED(InitD3D(g_hWnd)) || FAILED(InitGeometry()) || FAILED(InitShader()))
    {
        Cleanup();
        return 0;
    }

    MSG msg = {0};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            Render();
        }
    }

    Cleanup();
    return 0;
}