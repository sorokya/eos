#include <vcl.h>
#include <DBTables.hpp>
#include <ScktComp.hpp>
#pragma hdrstop
#pragma argsused
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    TDatabase *db = new TDatabase(NULL);
    TQuery *q = new TQuery(NULL);
    TServerSocket *s = new TServerSocket(NULL);
    TSession *ses = new TSession(NULL);
    Application->Initialize();
    Application->Run();
    return 0;
}
