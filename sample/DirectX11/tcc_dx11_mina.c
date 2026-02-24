#include <windows.h>
#include <stdio.h>
#include <string.h>

#define D3D11_CREATE_DEVICE_DEBUG 0x2
#define DXGI_SWAP_EFFECT_FLIP_DISCARD 4

// D3D11 USAGE’è‹`
#define D3D11_USAGE_DEFAULT 0
#define D3D11_USAGE_IMMUTABLE 1
#define D3D11_USAGE_DYNAMIC 2
#define D3D11_USAGE_STAGING 3

// D3D11 BIND_FLAG’è‹`
#define D3D11_BIND_VERTEX_BUFFER 0x0001
#define D3D11_BIND_INDEX_BUFFER 0x0002
#define D3D11_BIND_CONSTANT_BUFFER 0x0004
#define D3D11_BIND_SHADER_RESOURCE 0x0008
#define D3D11_BIND_STREAM_OUTPUT 0x0010
#define D3D11_BIND_RENDER_TARGET 0x0020
#define D3D11_BIND_DEPTH_STENCIL 0x0040
#define D3D11_BIND_UNORDERED_ACCESS 0x0080
#define D3D11_BIND_DECODER 0x0200
#define D3D11_BIND_VIDEO_ENCODER 0x0400

// DirectX11 Constants
#define D3D_DRIVER_TYPE_HARDWARE 1
#define DXGI_FORMAT_R8G8B8A8_UNORM 28
#define DXGI_FORMAT_R32G32B32_FLOAT 6
#define DXGI_FORMAT_R32G32B32A32_FLOAT 2
#define DXGI_SWAP_EFFECT_DISCARD 0
#define D3D11_SDK_VERSION 7
#define D3D11_INPUT_PER_VERTEX_DATA 0
#define D3D11_APPEND_ALIGNED_ELEMENT 0xffffffff
#define D3DCOMPILE_DEBUG 1
#define D3DCOMPILE_SKIP_OPTIMIZATION 4
#define D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST 4

typedef struct ID3D11Device ID3D11Device;
typedef struct ID3D11DeviceContext ID3D11DeviceContext;
typedef struct IDXGISwapChain IDXGISwapChain;
typedef struct ID3D11RenderTargetView ID3D11RenderTargetView;
typedef struct ID3D11Buffer ID3D11Buffer;
typedef struct ID3D11VertexShader ID3D11VertexShader;
typedef struct ID3D11PixelShader ID3D11PixelShader;
typedef struct ID3D11InputLayout ID3D11InputLayout;
typedef struct ID3D11Texture2D ID3D11Texture2D;

// Minimal ID3DBlob interface for shader compilation
typedef struct ID3DBlobVtbl
{
    HRESULT(__stdcall* QueryInterface)(void* This, const void* riid, void** ppvObject);
    ULONG(__stdcall* AddRef)(void* This);
    ULONG(__stdcall* Release)(void* This);
    void* (__stdcall* GetBufferPointer)(void* This);
    SIZE_T (__stdcall* GetBufferSize)(void* This);
} ID3DBlobVtbl;

typedef struct ID3DBlob { ID3DBlobVtbl* lpVtbl; } ID3DBlob;

// DXGI_MODE_DESC structure
typedef struct DXGI_MODE_DESC
{
    UINT Width;
    UINT Height;
    struct {
        UINT Numerator;
        UINT Denominator;
    } RefreshRate;
    UINT Format;
    UINT ScanlineOrdering;
    UINT Scaling;
} DXGI_MODE_DESC;

// DXGI_SAMPLE_DESC structure
typedef struct DXGI_SAMPLE_DESC
{
    UINT Count;
    UINT Quality;
} DXGI_SAMPLE_DESC;

// DXGI_SWAP_CHAIN_DESC - Correct nested structure
typedef struct DXGI_SWAP_CHAIN_DESC
{
    DXGI_MODE_DESC BufferDesc;
    DXGI_SAMPLE_DESC SampleDesc;
    UINT BufferUsage;
    UINT BufferCount;
    HWND OutputWindow;
    BOOL Windowed;
    UINT SwapEffect;
    UINT Flags;
} DXGI_SWAP_CHAIN_DESC;

typedef struct D3D11_BUFFER_DESC
{
    UINT ByteWidth;
    UINT Usage;
    UINT BindFlags;
    UINT CPUAccessFlags;
    UINT MiscFlags;
    UINT StructureByteStride;
} D3D11_BUFFER_DESC;

typedef struct D3D11_SUBRESOURCE_DATA
{
    const void* pSysMem;
    UINT SysMemPitch;
    UINT SysMemSlicePitch;
} D3D11_SUBRESOURCE_DATA;

typedef struct D3D11_INPUT_ELEMENT_DESC
{
    const char* SemanticName;
    UINT SemanticIndex;
    UINT Format;
    UINT InputSlot;
    UINT AlignedByteOffset;
    UINT InputSlotClass;
    UINT InstanceDataStepRate;
} D3D11_INPUT_ELEMENT_DESC;

typedef struct D3D11_VIEWPORT
{
    float TopLeftX;
    float TopLeftY;
    float Width;
    float Height;
    float MinDepth;
    float MaxDepth;
} D3D11_VIEWPORT;

typedef enum D3D_FEATURE_LEVEL
{
    D3D_FEATURE_LEVEL_9_1 = 0x9100,
    D3D_FEATURE_LEVEL_9_2 = 0x9200,
    D3D_FEATURE_LEVEL_9_3 = 0x9300,
    D3D_FEATURE_LEVEL_10_0 = 0xa000,
    D3D_FEATURE_LEVEL_10_1 = 0xa100,
    D3D_FEATURE_LEVEL_11_0 = 0xb000,
    D3D_FEATURE_LEVEL_11_1 = 0xb100
} D3D_FEATURE_LEVEL;

typedef enum D3D_COUNTER_TYPE
{
    D3D_COUNTER_GPU_IDLE = 0,
    D3D_COUNTER_VERTEX_PROCESSING = 1,
    D3D_COUNTER_GEOMETRY_PROCESSING = 2,
    D3D_COUNTER_PIXEL_PROCESSING = 3,
    D3D_COUNTER_OTHER_GPU_PROCESSING = 4,
    D3D_COUNTER_HOST_ADAPTER_BANDWIDTH_UTILIZATION = 5
} D3D_COUNTER_TYPE;

typedef enum D3D11_MAP
{
    D3D11_MAP_READ = 1,
    D3D11_MAP_WRITE = 2,
    D3D11_MAP_READ_WRITE = 3,
    D3D11_MAP_WRITE_DISCARD = 4,
    D3D11_MAP_WRITE_NO_OVERWRITE = 5
} D3D11_MAP;

typedef enum D3D11_DEVICE_CONTEXT_TYPE
{
    D3D11_DEVICE_CONTEXT_IMMEDIATE = 0,
    D3D11_DEVICE_CONTEXT_DEFERRED = 1
} D3D11_DEVICE_CONTEXT_TYPE;

// Vertex structure
typedef struct
{
    float x, y, z;
    float r, g, b, a;
} Vertex;

HRESULT __stdcall D3D11CreateDeviceAndSwapChain(
    void* pAdapter, int DriverType, HMODULE Software, UINT Flags,
    const void* pFeatureLevels, UINT FeatureLevels, UINT SDKVersion,
    const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
    IDXGISwapChain** ppSwapChain, ID3D11Device** ppDevice,
    void* pFeatureLevel, ID3D11DeviceContext** ppImmediateContext
);

// D3DCompile proc pointer (loaded dynamically)
typedef HRESULT(__stdcall *PFN_D3DCompile)(
    const void* pSrcData, SIZE_T SrcDataSize, const char* pSourceName,
    const void* pDefines, void* pInclude, const char* pEntrypoint,
    const char* pTarget, UINT Flags1, UINT Flags2,
    void** ppCode, void** ppErrorMsgs
);

static PFN_D3DCompile g_pD3DCompile = NULL;

// ID3D11DeviceVtbl - correct method order
typedef struct ID3D11DeviceVtbl
{
    HRESULT(__stdcall* QueryInterface)(void* This, const void* riid, void** ppvObject);
    ULONG(__stdcall* AddRef)(void* This);
    ULONG(__stdcall* Release)(void* This);
    HRESULT(__stdcall* CreateBuffer)(void* This, const D3D11_BUFFER_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, void** ppBuffer);
    HRESULT(__stdcall* CreateTexture1D)(void* This, const void* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, void** ppTexture1D);
    HRESULT(__stdcall* CreateTexture2D)(void* This, const void* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, void** ppTexture2D);
    HRESULT(__stdcall* CreateTexture3D)(void* This, const void* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, void** ppTexture3D);
    HRESULT(__stdcall* CreateShaderResourceView)(void* This, void* pResource, const void* pDesc, void** ppSRView);
    HRESULT(__stdcall* CreateUnorderedAccessView)(void* This, void* pResource, const void* pDesc, void** ppUAView);
    HRESULT(__stdcall* CreateRenderTargetView)(void* This, void* pResource, const void* pDesc, void** ppRTView);
    HRESULT(__stdcall* CreateDepthStencilView)(void* This, void* pResource, const void* pDesc, void** ppDepthStencilView);
    HRESULT(__stdcall* CreateInputLayout)(void* This, const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs, UINT NumElements, const void* pShaderBytecodeWithInputSignature, SIZE_T BytecodeLength, void** ppInputLayout);
    HRESULT(__stdcall* CreateVertexShader)(void* This, const void* pShaderBytecode, SIZE_T BytecodeLength, void* pClassLinkage, void** ppVertexShader);
    HRESULT(__stdcall* CreateGeometryShader)(void* This, const void* pShaderBytecode, SIZE_T BytecodeLength, void* pClassLinkage, void** ppGeometryShader);
    HRESULT(__stdcall* CreateGeometryShaderWithStreamOutput)(void* This, const void* pShaderBytecode, SIZE_T BytecodeLength, const void* pSODeclaration, UINT NumEntries, const UINT* pBufferStrides, UINT NumStrides, UINT RasterizedStream, void* pClassLinkage, void** ppGeometryShader);
    HRESULT(__stdcall* CreatePixelShader)(void* This, const void* pShaderBytecode, SIZE_T BytecodeLength, void* pClassLinkage, void** ppPixelShader);
    HRESULT(__stdcall* CreateHullShader)(void* This, const void* pShaderBytecode, SIZE_T BytecodeLength, void* pClassLinkage, void** ppHullShader);
    HRESULT(__stdcall* CreateDomainShader)(void* This, const void* pShaderBytecode, SIZE_T BytecodeLength, void* pClassLinkage, void** ppDomainShader);
    HRESULT(__stdcall* CreateComputeShader)(void* This, const void* pShaderBytecode, SIZE_T BytecodeLength, void* pClassLinkage, void** ppComputeShader);
    HRESULT(__stdcall* CreateClassLinkage)(void* This, void** ppLinkage);
    HRESULT(__stdcall* CreateBlendState)(void* This, const void* pBlendDesc, void** ppBlendState);
    HRESULT(__stdcall* CreateDepthStencilState)(void* This, const void* pDepthStencilDesc, void** ppDepthStencilState);
    HRESULT(__stdcall* CreateRasterizerState)(void* This, const void* pRasterizerDesc, void** ppRasterizerState);
    HRESULT(__stdcall* CreateSamplerState)(void* This, const void* pSamplerDesc, void** ppSamplerState);
    HRESULT(__stdcall* CreateQuery)(void* This, const void* pQueryDesc, void** ppQuery);
    HRESULT(__stdcall* CreatePredicate)(void* This, const void* pPredicateDesc, void** ppPredicate);
    HRESULT(__stdcall* CreateCounter)(void* This, const void* pCounterDesc, void** ppCounter);
    HRESULT(__stdcall* CreateDeferredContext)(void* This, UINT ContextFlags, void** ppDeferredContext);
    HRESULT(__stdcall* OpenSharedResource)(void* This, HANDLE hResource, const void* riid, void** ppResource);
    HRESULT(__stdcall* CheckFormatSupport)(void* This, UINT Format, UINT* pFormatSupport);
    HRESULT(__stdcall* CheckMultisampleQualityLevels)(void* This, UINT Format, UINT SampleCount, UINT* pNumQualityLevels);
    void(__stdcall* CheckCounterInfo)(void* This, void* pCounterInfo);
    HRESULT(__stdcall* CheckCounter)(void* This, const void* pDesc, D3D_COUNTER_TYPE* pType, UINT* pActiveCounters, char* szName, UINT* pNameLength, char* szUnits, UINT* pUnitsLength, wchar_t* szDescription, UINT* pDescriptionLength);
    HRESULT(__stdcall* CheckFeatureSupport)(void* This, UINT Feature, void* pFeatureSupportData, UINT FeatureSupportDataSize);
    HRESULT(__stdcall* GetPrivateData)(void* This, const void* guid, UINT* pDataSize, void* pData);
    HRESULT(__stdcall* SetPrivateData)(void* This, const void* guid, UINT DataSize, const void* pData);
    HRESULT(__stdcall* SetPrivateDataInterface)(void* This, const void* guid, const void* pData);
    D3D_FEATURE_LEVEL(__stdcall* GetFeatureLevel)(void* This);
    UINT(__stdcall* GetCreationFlags)(void* This);
    HRESULT(__stdcall* GetDeviceRemovedReason)(void* This);
    void(__stdcall* GetImmediateContext)(void* This, void** ppImmediateContext);
    HRESULT(__stdcall* SetExceptionMode)(void* This, UINT RaiseFlags);
    UINT(__stdcall* GetExceptionMode)(void* This);
} ID3D11DeviceVtbl;

// ID3D11DeviceContextVtbl
typedef struct ID3D11DeviceContextVtbl
{
    HRESULT(__stdcall* QueryInterface)(void* This, const void* riid, void** ppvObject);
    ULONG(__stdcall* AddRef)(void* This);
    ULONG(__stdcall* Release)(void* This);
    void(__stdcall* GetDevice)(void* This, void** ppDevice);
    HRESULT(__stdcall* GetPrivateData)(void* This, const void* guid, UINT* pDataSize, void* pData);
    HRESULT(__stdcall* SetPrivateData)(void* This, const void* guid, UINT DataSize, const void* pData);
    HRESULT(__stdcall* SetPrivateDataInterface)(void* This, const void* guid, const void* pData);
    void(__stdcall* VSSetConstantBuffers)(void* This, UINT StartSlot, UINT NumBuffers, void* const* ppConstantBuffers);
    void(__stdcall* PSSetShaderResources)(void* This, UINT StartSlot, UINT NumViews, void* const* ppShaderResourceViews);
    void(__stdcall* PSSetShader)(void* This, void* pPixelShader, void* const* ppClassInstances, UINT NumClassInstances);
    void(__stdcall* PSSetSamplers)(void* This, UINT StartSlot, UINT NumSamplers, void* const* ppSamplers);
    void(__stdcall* VSSetShader)(void* This, void* pVertexShader, void* const* ppClassInstances, UINT NumClassInstances);
    void(__stdcall* DrawIndexed)(void* This, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);
    void(__stdcall* Draw)(void* This, UINT VertexCount, UINT StartVertexLocation);
    HRESULT(__stdcall* Map)(void* This, void* pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags, void* pMappedResource);
    void(__stdcall* Unmap)(void* This, void* pResource, UINT Subresource);
    void(__stdcall* PSSetConstantBuffers)(void* This, UINT StartSlot, UINT NumBuffers, void* const* ppConstantBuffers);
    void(__stdcall* IASetInputLayout)(void* This, void* pInputLayout);
    void(__stdcall* IASetVertexBuffers)(void* This, UINT StartSlot, UINT NumBuffers, void* const* ppVertexBuffers, const UINT* pStrides, const UINT* pOffsets);
    void(__stdcall* IASetIndexBuffer)(void* This, void* pIndexBuffer, UINT Format, UINT Offset);
    void(__stdcall* DrawIndexedInstanced)(void* This, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation);
    void(__stdcall* DrawInstanced)(void* This, UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation);
    void(__stdcall* GSSetConstantBuffers)(void* This, UINT StartSlot, UINT NumBuffers, void* const* ppConstantBuffers);
    void(__stdcall* GSSetShader)(void* This, void* pShader, void* const* ppClassInstances, UINT NumClassInstances);
    void(__stdcall* IASetPrimitiveTopology)(void* This, int Topology);
    void(__stdcall* VSSetShaderResources)(void* This, UINT StartSlot, UINT NumViews, void* const* ppShaderResourceViews);
    void(__stdcall* VSSetSamplers)(void* This, UINT StartSlot, UINT NumSamplers, void* const* ppSamplers);
    void(__stdcall* Begin)(void* This, void* pAsync);
    void(__stdcall* End)(void* This, void* pAsync);
    HRESULT(__stdcall* GetData)(void* This, void* pAsync, void* pData, UINT DataSize, UINT GetDataFlags);
    void(__stdcall* SetPredication)(void* This, void* pPredicate, BOOL PredicateValue);
    void(__stdcall* GSSetShaderResources)(void* This, UINT StartSlot, UINT NumViews, void* const* ppShaderResourceViews);
    void(__stdcall* GSSetSamplers)(void* This, UINT StartSlot, UINT NumSamplers, void* const* ppSamplers);
    void(__stdcall* OMSetRenderTargets)(void* This, UINT NumViews, void* const* ppRenderTargetViews, void* pDepthStencilView);
    void(__stdcall* OMSetRenderTargetsAndUnorderedAccessViews)(void* This, UINT NumRTVs, void* const* ppRenderTargetViews, void* pDepthStencilView, UINT UAVStartSlot, UINT NumUAVs, void* const* ppUnorderedAccessViews, const UINT* pUAVInitialCounts);
    void(__stdcall* OMSetBlendState)(void* This, void* pBlendState, const float BlendFactor[4], UINT SampleMask);
    void(__stdcall* OMSetDepthStencilState)(void* This, void* pDepthStencilState, UINT StencilRef);
    void(__stdcall* SOSetTargets)(void* This, UINT NumBuffers, void* const* ppSOTargets, const UINT* pOffsets);
    void(__stdcall* DrawAuto)(void* This);
    void(__stdcall* DrawIndexedInstancedIndirect)(void* This, void* pBufferForArgs, UINT AlignedByteOffsetForArgs);
    void(__stdcall* DrawInstancedIndirect)(void* This, void* pBufferForArgs, UINT AlignedByteOffsetForArgs);
    void(__stdcall* Dispatch)(void* This, UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ);
    void(__stdcall* DispatchIndirect)(void* This, void* pBufferForArgs, UINT AlignedByteOffsetForArgs);
    void(__stdcall* RSSetState)(void* This, void* pRasterizerState);
    void(__stdcall* RSSetViewports)(void* This, UINT NumViewports, const D3D11_VIEWPORT* pViewports);
    void(__stdcall* RSSetScissorRects)(void* This, UINT NumRects, const void* pRects);
    void(__stdcall* CopySubresourceRegion)(void* This, void* pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, void* pSrcResource, UINT SrcSubresource, const void* pSrcBox);
    void(__stdcall* CopyResource)(void* This, void* pDstResource, void* pSrcResource);
    void(__stdcall* UpdateSubresource)(void* This, void* pDstResource, UINT DstSubresource, const void* pDstBox, const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch);
    void(__stdcall* CopyStructureCount)(void* This, void* pDstBuffer, UINT DstAlignedByteOffset, void* pSrcView);
    void(__stdcall* ClearRenderTargetView)(void* This, void* pRenderTargetView, const float ColorRGBA[4]);
} ID3D11DeviceContextVtbl;

typedef struct ID3D11Device
{
    ID3D11DeviceVtbl* lpVtbl;
} ID3D11Device;

typedef struct ID3D11DeviceContext
{
    ID3D11DeviceContextVtbl* lpVtbl;
} ID3D11DeviceContext;

typedef struct IDXGISwapChainVtbl
{
    HRESULT(__stdcall* QueryInterface)(void* This, const void* riid, void** ppvObject);
    ULONG(__stdcall* AddRef)(void* This);
    ULONG(__stdcall* Release)(void* This);
    HRESULT(__stdcall* SetPrivateData)(void* This, const void* guid, UINT DataSize, const void* pData);
    HRESULT(__stdcall* SetPrivateDataInterface)(void* This, const void* guid, const void* pData);
    HRESULT(__stdcall* GetPrivateData)(void* This, const void* guid, UINT* pDataSize, void* pData);
    HRESULT(__stdcall* GetParent)(void* This, const void* riid, void** ppParent);
    HRESULT(__stdcall* GetDevice)(void* This, const void* riid, void** ppDevice);
    HRESULT(__stdcall* Present)(void* This, UINT SyncInterval, UINT Flags);
    HRESULT(__stdcall* GetBuffer)(void* This, UINT Buffer, const void* riid, void** ppSurface);
} IDXGISwapChainVtbl;

typedef struct IDXGISwapChain
{
    IDXGISwapChainVtbl* lpVtbl;
} IDXGISwapChain;

typedef struct ID3D11RenderTargetViewVtbl
{
    HRESULT(__stdcall* QueryInterface)(void* This, const void* riid, void** ppvObject);
    ULONG(__stdcall* AddRef)(void* This);
    ULONG(__stdcall* Release)(void* This);
} ID3D11RenderTargetViewVtbl;

typedef struct ID3D11RenderTargetView
{
    ID3D11RenderTargetViewVtbl* lpVtbl;
} ID3D11RenderTargetView;

typedef struct ID3D11BufferVtbl
{
    HRESULT(__stdcall* QueryInterface)(void* This, const void* riid, void** ppvObject);
    ULONG(__stdcall* AddRef)(void* This);
    ULONG(__stdcall* Release)(void* This);
} ID3D11BufferVtbl;

typedef struct ID3D11Buffer
{
    ID3D11BufferVtbl* lpVtbl;
} ID3D11Buffer;

// Global variables
static IDXGISwapChain* g_swapchain = NULL;
static ID3D11Device* g_dev = NULL;
static ID3D11DeviceContext* g_ctx = NULL;
static ID3D11RenderTargetView* g_backbuffer = NULL;
static ID3D11Buffer* g_vertexbuffer = NULL;
static ID3D11VertexShader* g_vertexshader = NULL;
static ID3D11PixelShader* g_pixelshader = NULL;
static ID3D11InputLayout* g_layout = NULL;

static BOOL LoadD3DCompiler()
{
    if (g_pD3DCompile) return TRUE;
    const char* dlls[] = {
        "d3dcompiler_47.dll",
        "D3DCompiler_47.dll",
        "D3DCompiler_46.dll",
        "D3DCompiler_43.dll"
    };
    for (int i = 0; i < (int)(sizeof(dlls)/sizeof(dlls[0])); ++i)
    {
        HMODULE h = LoadLibraryA(dlls[i]);
        if (h)
        {
            g_pD3DCompile = (PFN_D3DCompile)GetProcAddress(h, "D3DCompile");
            if (g_pD3DCompile) return TRUE;
        }
    }
    return FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

BOOL InitD3D(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC scd;
    memset(&scd, 0, sizeof(scd));
    scd.BufferDesc.Width = 800;
    scd.BufferDesc.Height = 600;
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.ScanlineOrdering = 0;  // DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED
    scd.BufferDesc.Scaling = 0;           // DXGI_MODE_SCALING_UNSPECIFIED
    scd.SampleDesc.Count = 1;
    scd.SampleDesc.Quality = 0;
    scd.BufferUsage = 0x20;               // DXGI_USAGE_RENDER_TARGET_OUTPUT
    scd.BufferCount = 2;
    scd.OutputWindow = hwnd;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scd.Flags = 0;

    UINT createDeviceFlags = 0;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags,
        NULL, 0, D3D11_SDK_VERSION,
        &scd, &g_swapchain, &g_dev, NULL, &g_ctx
    );

    if (hr != 0)
    {
        printf("D3D11CreateDeviceAndSwapChain failed: 0x%08X\n", hr);
        return FALSE;
    }

    // Get back buffer and create render target view
    ID3D11Texture2D* backbuffer = NULL;
    static const GUID IID_ID3D11Texture2D = { 0x6f15aaf2, 0xd208, 0x4e89, {0x9a, 0xb4, 0x48, 0x95, 0x35, 0xd3, 0x4f, 0x9c} };

    hr = g_swapchain->lpVtbl->GetBuffer(g_swapchain, 0, &IID_ID3D11Texture2D, (void**)&backbuffer);
    if (hr != 0)
    {
        printf("GetBuffer failed: 0x%08X\n", hr);
        return FALSE;
    }

    // Create render target view
    hr = g_dev->lpVtbl->CreateRenderTargetView(g_dev, (void*)backbuffer, NULL, (void**)&g_backbuffer);
    if (hr != 0)
    {
        printf("CreateRenderTargetView failed: 0x%08X\n", hr);
        return FALSE;
    }

    printf("Back buffer and render target view created successfully\n");

    // Create vertex buffer
    Vertex vertices[] = {
        { 0.0f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f},
        { 0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f},
        {-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f}
    };

    D3D11_BUFFER_DESC bd;
    memset(&bd, 0, sizeof(bd));
    bd.ByteWidth = sizeof(vertices);
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
    bd.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA initData;
    memset(&initData, 0, sizeof(initData));
    initData.pSysMem = vertices;

    printf("Creating vertex buffer...\n");

    hr = g_dev->lpVtbl->CreateBuffer(g_dev, &bd, &initData, (void**)&g_vertexbuffer);
    if (hr != 0)
    {
        printf("CreateBuffer failed: 0x%08X\n", hr);
        return FALSE;
    }

    printf("Vertex buffer created successfully\n");

    // Load D3DCompiler and compile shaders
    if (!LoadD3DCompiler())
    {
        printf("Failed to load D3DCompiler DLL. Install D3DCompiler_47.dll or link with -ld3dcompiler.\n");
        return FALSE;
    }

    const char* vsSrc =
        "struct VSIn { float3 pos: POSITION; float4 col: COLOR; };\n"
        "struct VSOut { float4 pos: SV_Position; float4 col: COLOR; };\n"
        "VSOut main(VSIn i) { VSOut o; o.pos = float4(i.pos,1.0); o.col = i.col; return o; }\n";

    const char* psSrc =
        "struct PSIn { float4 pos: SV_Position; float4 col: COLOR; };\n"
        "float4 main(PSIn i) : SV_Target { return i.col; }\n";

    ID3DBlob* vsBlob = NULL;
    ID3DBlob* psBlob = NULL;
    ID3DBlob* errBlob = NULL;

    hr = g_pD3DCompile(vsSrc, strlen(vsSrc), "vs.hlsl", NULL, NULL, "main", "vs_4_0",
                       D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
                       (void**)&vsBlob, (void**)&errBlob);
    if (hr != 0)
    {
        const char* msg = errBlob ? (const char*)errBlob->lpVtbl->GetBufferPointer(errBlob) : "(no message)";
        printf("VS compile failed: 0x%08X %s\n", hr, msg);
        if (errBlob) errBlob->lpVtbl->Release(errBlob);
        return FALSE;
    }
    if (errBlob) { errBlob->lpVtbl->Release(errBlob); errBlob = NULL; }

    hr = g_pD3DCompile(psSrc, strlen(psSrc), "ps.hlsl", NULL, NULL, "main", "ps_4_0",
                       D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0,
                       (void**)&psBlob, (void**)&errBlob);
    if (hr != 0)
    {
        const char* msg = errBlob ? (const char*)errBlob->lpVtbl->GetBufferPointer(errBlob) : "(no message)";
        printf("PS compile failed: 0x%08X %s\n", hr, msg);
        if (errBlob) errBlob->lpVtbl->Release(errBlob);
        if (vsBlob) vsBlob->lpVtbl->Release(vsBlob);
        return FALSE;
    }
    if (errBlob) { errBlob->lpVtbl->Release(errBlob); errBlob = NULL; }

    // Create shaders
    hr = g_dev->lpVtbl->CreateVertexShader(g_dev,
        vsBlob->lpVtbl->GetBufferPointer(vsBlob), vsBlob->lpVtbl->GetBufferSize(vsBlob), NULL, (void**)&g_vertexshader);
    if (hr != 0)
    {
        printf("CreateVertexShader failed: 0x%08X\n", hr);
        vsBlob->lpVtbl->Release(vsBlob);
        psBlob->lpVtbl->Release(psBlob);
        return FALSE;
    }

    hr = g_dev->lpVtbl->CreatePixelShader(g_dev,
        psBlob->lpVtbl->GetBufferPointer(psBlob), psBlob->lpVtbl->GetBufferSize(psBlob), NULL, (void**)&g_pixelshader);
    if (hr != 0)
    {
        printf("CreatePixelShader failed: 0x%08X\n", hr);
        vsBlob->lpVtbl->Release(vsBlob);
        psBlob->lpVtbl->Release(psBlob);
        return FALSE;
    }

    // Create input layout
    D3D11_INPUT_ELEMENT_DESC layout[2];
    memset(layout, 0, sizeof(layout));
    layout[0].SemanticName = "POSITION";
    layout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    layout[0].InputSlot = 0;
    layout[0].AlignedByteOffset = 0;
    layout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    layout[1].SemanticName = "COLOR";
    layout[1].SemanticIndex = 0;
    layout[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    layout[1].InputSlot = 0;
    layout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    layout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

    hr = g_dev->lpVtbl->CreateInputLayout(g_dev, layout, 2,
        vsBlob->lpVtbl->GetBufferPointer(vsBlob), vsBlob->lpVtbl->GetBufferSize(vsBlob), (void**)&g_layout);

    // Release blobs
    vsBlob->lpVtbl->Release(vsBlob);
    psBlob->lpVtbl->Release(psBlob);

    if (hr != 0)
    {
        printf("CreateInputLayout failed: 0x%08X\n", hr);
        return FALSE;
    }

    return TRUE;
}

void Render()
{
    if (!g_ctx || !g_swapchain) return;

    float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    if (g_backbuffer)
    {
        g_ctx->lpVtbl->ClearRenderTargetView(g_ctx, (void*)g_backbuffer, clearColor);
    }

    // Set render target
    g_ctx->lpVtbl->OMSetRenderTargets(g_ctx, 1, (void* const*)&g_backbuffer, NULL);

    // Set pipeline state
    if (g_layout)
    {
        g_ctx->lpVtbl->IASetInputLayout(g_ctx, (void*)g_layout);
    }

    if (g_vertexbuffer)
    {
        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        g_ctx->lpVtbl->IASetVertexBuffers(g_ctx, 0, 1, (void* const*)&g_vertexbuffer, &stride, &offset);
    }

    g_ctx->lpVtbl->IASetPrimitiveTopology(g_ctx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D11_VIEWPORT viewport;
    memset(&viewport, 0, sizeof(viewport));
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = 800;
    viewport.Height = 600;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    g_ctx->lpVtbl->RSSetViewports(g_ctx, 1, &viewport);

    if (g_vertexshader)
    {
        g_ctx->lpVtbl->VSSetShader(g_ctx, (void*)g_vertexshader, NULL, 0);
    }
    if (g_pixelshader)
    {
        g_ctx->lpVtbl->PSSetShader(g_ctx, (void*)g_pixelshader, NULL, 0);
    }

    // Draw triangle
    g_ctx->lpVtbl->Draw(g_ctx, 3, 0);

    g_swapchain->lpVtbl->Present(g_swapchain, 1, 0);
}

void CleanupD3D()
{
    if (g_vertexbuffer) g_vertexbuffer->lpVtbl->Release((void*)g_vertexbuffer);
    if (g_backbuffer) g_backbuffer->lpVtbl->Release((void*)g_backbuffer);
    if (g_ctx) g_ctx->lpVtbl->Release((void*)g_ctx);
    if (g_swapchain) g_swapchain->lpVtbl->Release((void*)g_swapchain);
    if (g_dev) g_dev->lpVtbl->Release((void*)g_dev);
}

int main()
{
    WNDCLASSW wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"DX11Window";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(L"DX11Window", L"DX11 Triangle with TCC",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, wc.hInstance, NULL);

    if (!hwnd)
    {
        printf("CreateWindow failed\n");
        return 1;
    }

    if (!InitD3D(hwnd))
    {
        printf("DirectX11 initialization failed\n");
        CleanupD3D();
        return 1;
    }

    printf("DirectX11 initialized successfully!\n");
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    MSG msg;
    memset(&msg, 0, sizeof(msg));
    while (TRUE)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        Render();
        Sleep(16);
    }

    CleanupD3D();
    return 0;
}