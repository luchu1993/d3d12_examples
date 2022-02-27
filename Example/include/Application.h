#pragma once

#include "Common.h"


class Graphics;

class Application
{
public:
    explicit Application();
    virtual ~Application();

    /// Initialize
    virtual void Setup() { }

    /// Start after initialize and before main loop
    virtual void Start() { }

    /// Cleanup 
    virtual void Stop() { }

    /// Initialize and run main loop, then return exit code.
    int Run();

    /// Show an error message
    void ErrorExit(std::string const& message = "");

private:
    /// d3d graphics
    std::shared_ptr<Graphics> graphics_;
    /// Application exit code
    int exitCode_;
};
