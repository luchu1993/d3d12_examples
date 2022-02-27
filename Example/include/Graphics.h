#pragma once

#include <windows.h>
#include <memory>
#include <string>

#include "Common.h"


struct WindowModeParams
{
    /// Width of the window. 0 to pick automatically.
    int width_{};
    /// Height of the window. 0 to pick automatically.
    int height_{};
    /// Whether the window is resizable.
    bool resizable_{};
    /// Level of multisampling.
    unsigned multiSample_{1};
    /// Monitor for fullscreen mode. Has no effect in windowed mode.
    int monitor_{};
    /// Refresh rate. 0 to pick automatically.
    int refreshRate_{};
};

class GraphicsImpl;

class Graphics
{
public:
    /// Construct.
    explicit Graphics();
    /// Destruct
    virtual ~Graphics();
    /// Initialize
    bool Initialize();
    /// Return whether exit has been requested.
    bool IsExiting();
    /// Run one frame.
    void RunFrame();
    /// Send frame update events.
    void Update();
    /// Render after frame update.
    void Render();
    /// Close the graphics window and set the exit flag
    void Exit();
    
    // Set window title
    void SetWindowTitle(std::string const& title);
    /// Set window mode
    bool SetWindowMode(int width, int height);
    /// Set window mode
    bool SetWindowMode(WindowModeParams const& mode);

private:
    /// Create the Direct3D12 device and swap chain.
    bool CreateDevice(int width, int height);
    /// Create window 
    HWND OpenWindow();
    /// Update swap chain size
    bool UpdateSwapChain();

    /// Implementation.
    std::shared_ptr<GraphicsImpl> impl_;
    /// Window titile name
    std::string title_ { "D3D12 Example" };
    /// Window instance
    HWND window_;
    /// Initialized flag.
    bool initialized_;
    /// Exiting flag.
    bool exiting_;
    
    /// sRGB conversion on write flag for the main window.
    bool sRGB_{};
    /// Window mode
    WindowModeParams modeParams_;
};
