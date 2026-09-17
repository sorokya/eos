#include <vcl.h>
#pragma hdrstop

#include "GUI.h"

TGUI *GUI;

__fastcall TGUI::TGUI(TComponent *Owner) : TForm(Owner)
{
}

#pragma argsused
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    Application->Initialize();
    Application->Run();
    return 0;
}
