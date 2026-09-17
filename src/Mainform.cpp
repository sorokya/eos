#include <vcl.h>
#include <stdio.h>
#pragma hdrstop

#include "Mainform.h"

#pragma package(smart_init)

// Cross-unit operations the form invokes. Their mangled names are unobservable
// in the stripped image, so they are declared here; the argument shapes are
// pinned by the reference call sites. Replaced by the owning units' headers
// once those are reconstructed.
void Server_Shutdown(Server *server);
void Server_RemovePlayer(Server *server, TCustomWinSocket *socket);
void Server_ClientRead(Server *server, TCustomWinSocket *socket, String data);
bool Players_Add(Players *players, TCustomWinSocket *socket);
void Players_Remove(Players *players, TCustomWinSocket *socket);
void Players_MarkRemoving(Players *players, TCustomWinSocket *socket);
bool Logins_HandleAddress(Logins *logins, String address);
int Settings_GetMaxConnections(Settings *settings);

__fastcall TGUI::TGUI(TComponent *Owner) : TForm(Owner)
{
}

void __fastcall TGUI::FormClose(TObject *Sender, TCloseAction &Action)
{
    Server_Shutdown(server_ctrl);
    Action = caNone;
}

void __fastcall TGUI::serverClientError(TObject *Sender,
                                        TCustomWinSocket *Socket,
                                        TErrorEvent ErrorEvent,
                                        int &ErrorCode)
{
    ErrorCode = 0;
    if (Socket->SocketHandle < 1 || Socket->SocketHandle >= 100000)
        Socket->Close();
    else
        Players_MarkRemoving(players, Socket);
}

void __fastcall TGUI::serverClientConnect(TObject *Sender, TCustomWinSocket *Socket)
{
    if (Socket->SocketHandle < 1)
    {
        Socket->Close();
        return;
    }
    if (server->Socket->ActiveConnections > Settings_GetMaxConnections(settings) + 5)
    {
        Socket->Close();
        return;
    }
    if (!Logins_HandleAddress(logins, Socket->RemoteAddress))
    {
        Socket->Close();
        return;
    }
    if (!Players_Add(players, Socket))
    {
        Socket->Close();
        return;
    }
}

void __fastcall TGUI::serverClientDisconnect(TObject *Sender, TCustomWinSocket *Socket)
{
    if (Socket->SocketHandle >= 1 && Socket->SocketHandle < 100000)
    {
        Server_RemovePlayer(server_ctrl, Socket);
        Players_Remove(players, Socket);
    }
}

void __fastcall TGUI::serverClientRead(TObject *Sender, TCustomWinSocket *Socket)
{
    if (Socket->SocketHandle < 1 || Socket->SocketHandle >= 100000)
    {
        Socket->Close();
        return;
    }
    Server_ClientRead(server_ctrl, Socket, Socket->ReceiveText());
}

void __fastcall TGUI::ApplicationEvents1Exception(TObject *Sender, Exception *E)
{
    String s = DateTimeToStr(Now());
    s.Insert(" ", s.Length() + 1);
    s.Insert(TimeToStr(Now()), s.Length() + 1);
    s.Insert(" EndlServ ", s.Length() + 1);
    s.Insert(E->Message, s.Length() + 1);
    s.Insert(" ", s.Length() + 1);
    s.Insert(IntToStr(field_36c), s.Length() + 1);
    s.Insert(",", s.Length() + 1);
    s.Insert(IntToStr(field_370), s.Length() + 1);
    s.Insert("\n", s.Length() + 1);
    FILE *fp;
    fp = fopen(".\\logs\\error.log", "a");
    fprintf(fp, "%s", s.c_str());
    fclose(fp);
}
