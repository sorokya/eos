#include <vcl.h>
#pragma hdrstop
USERES("GameServer.res");
USEFORM("Mainform.cpp", GUI);

#pragma argsused
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    try
    {
        Application->Initialize();
        Application->CreateForm(__classid(TGUI), &GUI);
        Application->Run();
    }
    catch (Exception &exception)
    {
        Application->ShowException(&exception);
    }
    return 0;
}
