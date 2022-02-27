#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>

#define D3D_SAFE_RELEASE(p) if (p) { ((IUnknown*) p)->Release(); p = nullptr; }

class GraphicsImpl
{
    friend class Graphics;

public:
    /// Construct.
    explicit GraphicsImpl();
    /// Destructor
    ~GraphicsImpl();

    /// Return D3D12 device.
    ID3D12Device* GetDevice() const { return device_;}
    /// Return D3D12 command list.
    ID3D12GraphicsCommandList* GetCommandList() const { return commandList_; }
    /// Flush command queue
    void FlushCommandQueue();
    /// Submit command list
    void ExecuteCommandList();
    /// Begin render
    void Begin();
    /// End render
    void End();
    
private:
    /// Back buffer count
    static constexpr unsigned SwapChainBufferCount{2};
    /// Depth stencil format
    static constexpr DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    /// Create D3D12 command objects
    bool CreateCommandObjects();
    /// Create D3D12 descriptor heap
    bool CreateDescriptorHeap();

    /// Create render target views
    bool ResetRenderTargetViews();
    /// Create depth stencil view
    bool ResetDepthStencilView(int width, int height, unsigned sampleCount, unsigned sampleQuality);
    /// Reset viewport
    void ResetViewport(int width, int height);

    /// Return multisample quality level for a given texture format and sample count.
    unsigned GetMultiSampleQuality(DXGI_FORMAT format, unsigned sampleCount) const;
    
    /// Return current backbuffer descriptor
    D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
    /// Return depth stencil view
    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;
    /// Return current back buffer
    ID3D12Resource* CurrentBackBuffer() const;

    /// DXGi Factory
    IDXGIFactory1* factory_;
    /// Graphics device.
    ID3D12Device* device_{};
    /// Swap chain.
    IDXGISwapChain* swapChain_{};
    
    /// Back buffers
    ID3D12Resource* defaultRenderTargets_[SwapChainBufferCount] {};
    /// Depth stencil buffer
    ID3D12Resource* defaultDepthStencil_{};

    /// Command queue
    ID3D12CommandQueue* commandQueue_{};
    /// Graphics Command list
    ID3D12GraphicsCommandList* commandList_{};
    /// Command allocator
    ID3D12CommandAllocator* commandAllocator_{};
    
    /// Fence
    ID3D12Fence* fence_{};
    /// Current Fence value
    unsigned fenceValue_{};

    /// RTV descriptor heap
    ID3D12DescriptorHeap* renderTargetViewHeap_{};
    /// DSV descriptor heap
    ID3D12DescriptorHeap* depthStencilViewHeap_;

    /// RTV descriptor size
    unsigned renderTargetViewSize_{};
    /// DSV descriptor size
    unsigned depthStencilViewSize_{};
    /// CBV SRV UAV descriptor size
    unsigned bufferViewSize_{};

    /// Current backbuffer index
    unsigned currentBackBufferIndex_{};
    
    /// Viewport
    D3D12_VIEWPORT viewport_{}; 
    D3D12_RECT scissor_{};

    /// Return a resource transition barrier
    static D3D12_RESOURCE_BARRIER Transition(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, 
        unsigned subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE);
};
