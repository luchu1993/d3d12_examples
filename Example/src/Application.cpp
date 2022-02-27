#include "Application.h"
#include "Graphics.h"


void ErrorDialog(const std::string& title, const std::string& message)
{
    MessageBox(NULL, message.c_str(), title.c_str(), MB_ICONERROR);
}

Application::Application()
    : exitCode_(EXIT_SUCCESS)
{
    graphics_ = std::make_shared<Graphics>();
}

Application::~Application() = default;

int Application::Run()
{
    Setup();
    if (exitCode_)
        return exitCode_;

    if (!graphics_->Initialize())
    {
        ErrorExit();
        return exitCode_;
    }

    Start();
    if (exitCode_)
        return exitCode_;

    while (!graphics_->IsExiting())
    {
        graphics_->RunFrame();
    }

    Stop();

    return exitCode_;
}

void Application::ErrorExit(std::string const& message)
{
    graphics_->Exit();

    if (!message.length())
    {
        ErrorDialog("Application",  message);
    }
}
