
#include "Graphics.h"
#include "GraphicsImpl.h"


static Graphics* gInstance = nullptr;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

Graphics::Graphics()
    : impl_(new GraphicsImpl)
    , window_(nullptr)
    , initialized_(false)
    , exiting_(false)
{
    gInstance = this;
}

Graphics::~Graphics() = default;

bool Graphics::Initialize()
{
    WindowModeParams mode;
    mode.width_ = 1334;
    mode.height_ = 750;
    mode.resizable_ = false;

    if (!SetWindowMode(mode))
        return false;

    initialized_ = true;
    return true;
}

bool Graphics::IsExiting()
{
    return exiting_;
}

void Graphics::RunFrame()
{
    assert(initialized_);

    if (exiting_) return;

    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));

    if (PeekMessage(&msg,0 , 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (msg.message == WM_QUIT)
    {
        exiting_ = true;
        return;
    }

    Update();

    Render();
}

void Graphics::Update()
{

}

void Graphics::Render()
{
    impl_->Begin();
    
    impl_->End();
}

void Graphics::Exit()
{
    exiting_ = true;
}

void Graphics::SetWindowTitle(std::string const& title)
{
    title_ = title;

    if (window_)
    {
        SetWindowText(window_, title_.c_str());
    }
}

bool Graphics::SetWindowMode(int width, int height)
{
    WindowModeParams params;
    params.width_ = width;
    params.height_ = height;
    params.resizable_ = false;

    return SetWindowMode(params);
}

bool Graphics::SetWindowMode(WindowModeParams const& mode)
{
    modeParams_ = mode;

    if (!window_)
    {
        window_ = OpenWindow();
    }

    return CreateDevice(modeParams_.width_, modeParams_.height_);
}

HWND Graphics::OpenWindow()
{
    const char* title = title_.c_str();

    WNDCLASS ws;
    ws.style = CS_HREDRAW | CS_VREDRAW;
    ws.lpfnWndProc = WndProc;
    ws.cbClsExtra = 0;
    ws.cbWndExtra = 0;
    ws.hInstance = GetModuleHandle(nullptr);
    ws.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    ws.hCursor = LoadCursor(nullptr, IDC_ARROW);
    ws.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
    ws.lpszMenuName = nullptr;
    ws.lpszClassName = title;

    if (!RegisterClass(&ws))
        return nullptr;

    UINT windowStyle = WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU;
    if (modeParams_.resizable_)
    {
        windowStyle |= WS_SIZEBOX;
    }

    window_ = CreateWindow(
        title, title, windowStyle, 
        CW_USEDEFAULT, CW_USEDEFAULT,
        modeParams_.width_, modeParams_.height_,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr
    );
    
    if (window_ == nullptr)
        return nullptr;

    ShowWindow(window_, SW_SHOWDEFAULT);
    UpdateWindow(window_);
    
    return window_;
}

bool Graphics::CreateDevice(int width, int height)
{
    if (!impl_->device_)
    {
#if defined(DEBUG) || defined(_DEBUG)
        ID3D12Debug* debugController;
        D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
        debugController->EnableDebugLayer();
        D3D_SAFE_RELEASE(debugController);
#endif

        HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&impl_->factory_));
        if (FAILED(hr))
        {
            D3D_SAFE_RELEASE(impl_->factory_);
            LOGERROR("Failed to create DXGI factory. (HRESULT %x)", hr);
            return false;
        }

        hr = D3D12CreateDevice(
            nullptr,  // default mointor
            D3D_FEATURE_LEVEL_12_0,
            IID_PPV_ARGS(&impl_->device_)
        );

        if (FAILED(hr))
        {
            D3D_SAFE_RELEASE(impl_->device_);
            LOGERROR("Failed to create D3D12 device. (HRESULT %x)", hr);
            return false;
        }
    }

    // Create command objects
    if (!impl_->CreateCommandObjects())
        return false;

    // Create descriptor heap
    if (!impl_->CreateDescriptorHeap())
        return false;

    return UpdateSwapChain();
}

bool Graphics::UpdateSwapChain()
{
    impl_->FlushCommandQueue();
    impl_->commandList_->Reset(impl_->commandAllocator_, nullptr);
    
    DXGI_FORMAT backBufferFormat = sRGB_ ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    unsigned sampleCount = modeParams_.multiSample_;
    unsigned sampleQuality = impl_->GetMultiSampleQuality(backBufferFormat, modeParams_.multiSample_);
        
    if (!impl_->swapChain_)
    {
        DXGI_SWAP_CHAIN_DESC swapChainDesc;
        ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
        swapChainDesc.BufferCount = GraphicsImpl::SwapChainBufferCount;
        swapChainDesc.BufferDesc.Width = (UINT) modeParams_.width_;
        swapChainDesc.BufferDesc.Height = (UINT) modeParams_.height_;
        swapChainDesc.BufferDesc.Format = backBufferFormat;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferDesc.RefreshRate = { 60, 1 };
        swapChainDesc.OutputWindow = window_;
        swapChainDesc.SampleDesc.Count = sampleCount;
        swapChainDesc.SampleDesc.Quality = sampleQuality;
        swapChainDesc.Windowed = true;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        HRESULT hr = impl_->factory_->CreateSwapChain(impl_->commandQueue_, &swapChainDesc, &impl_->swapChain_);
        if (FAILED(hr))
        {
            D3D_SAFE_RELEASE(impl_->swapChain_);
            LOGERROR("Failed to create D3D11 swap chain. (HRESULT %x)", hr);
            return false;
        }
    }    

    // Reset render target views
    if (!impl_->ResetRenderTargetViews())
        return false;

    // Reset depth stencil view
    if (!impl_->ResetDepthStencilView(modeParams_.width_, modeParams_.height_, sampleCount, sampleQuality))
        return false;

    // Reset viewport
    impl_->ResetViewport(modeParams_.width_, modeParams_.height_);

    return true;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch(msg)
    {
        case WM_KEYDOWN:
        {
            if (wparam == VK_ESCAPE)
                gInstance->Exit();
        }  break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wparam, lparam);
    }

    return 0;
}