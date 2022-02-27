
#include "GraphicsImpl.h"
#include "Common.h"


GraphicsImpl::GraphicsImpl() = default;

GraphicsImpl::~GraphicsImpl()
{
    for (unsigned i = 0; i < SwapChainBufferCount; ++i)
        D3D_SAFE_RELEASE(defaultRenderTargets_[i]);

    D3D_SAFE_RELEASE(defaultDepthStencil_);

    D3D_SAFE_RELEASE(fence_);

    D3D_SAFE_RELEASE(renderTargetViewHeap_);
    D3D_SAFE_RELEASE(depthStencilViewHeap_);

    D3D_SAFE_RELEASE(commandList_);
    D3D_SAFE_RELEASE(commandAllocator_);
    D3D_SAFE_RELEASE(commandQueue_);

    D3D_SAFE_RELEASE(swapChain_);
    D3D_SAFE_RELEASE(device_);
}

bool GraphicsImpl::CreateCommandObjects()
{
    // Get descriptor size 
    renderTargetViewSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    depthStencilViewSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    bufferViewSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    
    // Create fence
    HRESULT hr = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
    if (FAILED(hr))
    {
        D3D_SAFE_RELEASE(fence_);
        LOGERROR("Failed to create D3D12 fence. (HRESULT %x)", hr);
        return false;
    }

    // Create command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc;
    ZeroMemory(&queueDesc, sizeof(queueDesc));
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    hr = device_->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_));
    if (FAILED(hr))
    {
        D3D_SAFE_RELEASE(commandQueue_);
        LOGERROR("Failed to create D3D12 command queue. (HRESULT %x)", hr);
        return false;
    }

    // Create command allocator
    hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
    if (FAILED(hr))
    {
        D3D_SAFE_RELEASE(commandAllocator_);
        LOGERROR("Failed to create D3D12 command allocator. (HRESULT %x)", hr);
        return false;
    }

    // Create command list
    hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, 
        commandAllocator_, nullptr, IID_PPV_ARGS(&commandList_));
    if (FAILED(hr))
    {
        D3D_SAFE_RELEASE(commandList_);
        LOGERROR("Failed to create D3D12 command list. (HRESULT %x)", hr);
        return false;
    }

    commandList_->Close();

    return true;
}

bool GraphicsImpl::CreateDescriptorHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;

    HRESULT hr = device_->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&renderTargetViewHeap_));
    if (FAILED(hr))
    {
        D3D_SAFE_RELEASE(renderTargetViewHeap_);
        LOGERROR("Create RTV descriptor heap failed. (HRESULT %x)", hr);
        return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;

    hr = device_->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&depthStencilViewHeap_));
    if (FAILED(hr))
    {
        D3D_SAFE_RELEASE(depthStencilViewHeap_);
        LOGERROR("Create DSV descriptor heap failed. (HRESULT %x)", hr);
        return false;
    }

    return true;
}

bool GraphicsImpl::ResetRenderTargetViews()
{
    /// Release previous resource
    for (int i = 0; i < GraphicsImpl::SwapChainBufferCount; ++i)
        D3D_SAFE_RELEASE(defaultRenderTargets_[i]);
    
    D3D12_CPU_DESCRIPTOR_HANDLE handle = renderTargetViewHeap_->GetCPUDescriptorHandleForHeapStart();

    for (unsigned i = 0; i < SwapChainBufferCount; ++i)
    {    
        swapChain_->GetBuffer(i, IID_PPV_ARGS(&defaultRenderTargets_[i]));
        device_->CreateRenderTargetView(defaultRenderTargets_[i], nullptr, handle);

        handle.ptr += renderTargetViewSize_;
    }

    return true;
}

bool GraphicsImpl::ResetDepthStencilView(int width, int height, unsigned sampleCount, unsigned sampleQuality)
{
    D3D_SAFE_RELEASE(defaultDepthStencil_);

    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = width;
    depthStencilDesc.Height = height;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    depthStencilDesc.SampleDesc.Count = sampleCount;
    depthStencilDesc.SampleDesc.Quality = sampleQuality;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = DepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;
    
    D3D12_HEAP_PROPERTIES heapProperty;
    heapProperty.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProperty.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperty.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperty.CreationNodeMask = 1;
    heapProperty.VisibleNodeMask = 1;

    HRESULT hr = device_->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &depthStencilDesc, 
        D3D12_RESOURCE_STATE_COMMON, &optClear, IID_PPV_ARGS(&defaultDepthStencil_));
    if (FAILED(hr))
    {
        D3D_SAFE_RELEASE(defaultDepthStencil_);
        LOGERROR("Create default depth setncil buffer failed. (HRESULT %x)", hr);
        return false;
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DepthStencilFormat;
	dsvDesc.Texture2D.MipSlice = 0;

    device_->CreateDepthStencilView(defaultDepthStencil_, &dsvDesc, DepthStencilView());

    // Transition resource state from common to depth buffer
    commandList_->ResourceBarrier(1, &Transition(defaultDepthStencil_, 
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

    ExecuteCommandList();
    FlushCommandQueue();

    return true;
}

unsigned GraphicsImpl::GetMultiSampleQuality(DXGI_FORMAT format, unsigned sampleCount) const
{
    if (sampleCount < 2)
        return 0; // Not multisampled, should use quality 0

    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
    msQualityLevels.Format = format;
    msQualityLevels.SampleCount = sampleCount;
    msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msQualityLevels.NumQualityLevels = 0;

    device_->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels));
    return msQualityLevels.NumQualityLevels;
}

D3D12_CPU_DESCRIPTOR_HANDLE GraphicsImpl::CurrentBackBufferView() const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = renderTargetViewHeap_->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += currentBackBufferIndex_ * renderTargetViewSize_;

    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE GraphicsImpl::DepthStencilView() const
{
    return depthStencilViewHeap_->GetCPUDescriptorHandleForHeapStart();
}

ID3D12Resource* GraphicsImpl::CurrentBackBuffer() const
{
    return defaultRenderTargets_[currentBackBufferIndex_];
}

D3D12_RESOURCE_BARRIER GraphicsImpl::Transition(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, 
        unsigned subResource, D3D12_RESOURCE_BARRIER_FLAGS flags)
{
    D3D12_RESOURCE_BARRIER barrier;
    ZeroMemory(&barrier, sizeof(barrier));

    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = flags;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    barrier.Transition.Subresource = subResource;

    return barrier;
}

void GraphicsImpl::ResetViewport(int width, int height)
{
    viewport_.TopLeftX = 0;
    viewport_.TopLeftY = 0;
    viewport_.Width = (float) width;
    viewport_.Height = (float) height;
    viewport_.MaxDepth = 1.0f;
    viewport_.MinDepth = 0.0f;

    scissor_ = { 0, 0, width, height };
}

void GraphicsImpl::FlushCommandQueue()
{
    ++fenceValue_;
    commandQueue_->Signal(fence_, fenceValue_);

    // Wait GPu completed
    if (fence_->GetCompletedValue() < fenceValue_)
    {
        HANDLE event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        fence_->SetEventOnCompletion(fenceValue_, event);

        WaitForSingleObject(event, INFINITE);
        CloseHandle(event);
    }
}

void GraphicsImpl::ExecuteCommandList()
{
    HRESULT hr = commandList_->Close();
    if (FAILED(hr))
    {
        LOGERROR("Execute command list failed. (HRESULT %x)", hr);
        return;
    }

    ID3D12CommandList* commandLists[] = { commandList_ };
    commandQueue_->ExecuteCommandLists(_countof(commandLists), commandLists);
}

void GraphicsImpl::Begin()
{
    commandAllocator_->Reset();
    commandList_->Reset(commandAllocator_, nullptr);

    commandList_->ResourceBarrier(1, &Transition(
        CurrentBackBuffer(), 
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET)
    );

    commandList_->RSSetViewports(1, &viewport_);
    commandList_->RSSetScissorRects(1, &scissor_);

    float clearColor[4] = {0.2f, 0.3f, 0.7f, 1.0f};
    commandList_->ClearRenderTargetView(CurrentBackBufferView(), clearColor, 0, nullptr);
    commandList_->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    commandList_->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
}

void GraphicsImpl::End()
{
    commandList_->ResourceBarrier(1, &Transition(
        CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT)
    );

    ExecuteCommandList();

    swapChain_->Present(0, 0);
    currentBackBufferIndex_ = (currentBackBufferIndex_ + 1) % SwapChainBufferCount;

    FlushCommandQueue();
}
