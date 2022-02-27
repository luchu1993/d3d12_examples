#include <crtdbg.h>
#include <windows.h>
#include "Application.h"


int WINAPI WinMain(HINSTANCE hPrevInstance, HINSTANCE hInstance, PSTR pCmdLine, int nCmdLine)
{
#if defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    
    std::shared_ptr<Application> application(new Application);
    application->Run();

    return 0;
}