#include <vcl.h>
#pragma hdrstop

#include "GUI.h"

TGUI *GUI;

#pragma argsused
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Application->Initialize();
    Application->Run();
    return 0;
}
