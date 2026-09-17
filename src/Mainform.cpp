#include <vcl.h>
#include <stdio.h>
#pragma hdrstop

#include "Mainform.h"
#include "Serial.h"

#include "Itemvalues.h"
#include "Npcvalues.h"
#include "Skillvalues.h"
#include "Learnvalues.h"
#include "Shopvalues.h"
#include "Innvalues.h"
#include "Classvalues.h"
#include "Jukeboxcontrol.h"
#include "Settings.h"
#include "Gamecontrol.h"
#include "Logins.h"
#include "Newscontrol.h"
#include "Mysqlcontrols.h"
#include "Npccontrol.h"
#include "Effectcontrol.h"
#include "Weddings.h"
#include "Doorcontrol.h"
#include "Chestcontrol.h"
#include "Eventcontrol.h"
#include "Msgboardcontrol.h"

#pragma package(smart_init)

// Stub definitions for the controller/subsystem classes the form constructs.
// Their real layouts live in their own (not yet reconstructed) units; only the
// size and constructor shape are needed to reproduce the reference codegen.
// The sizes are pinned by the `operator new` arguments in FormCreate.
class Mapcontrol
{
    char _pad[0x44];

  public:
    Mapcontrol(Settings *settings);
};
class Players
{
    char _pad[0x61ab4];

  public:
    Players(Settings *settings, Mysqlcontrols *mysql);
};
class Server
{
    char _pad[0xc8];

  public:
    Server(Mapcontrol *map,
           Questengine *quest,
           Players *players,
           Settings *settings,
           Mysqlcontrols *mysql,
           Logins *logins,
           int version_patch,
           int version_minor,
           int version_major);
};
class Questengine
{
    char _pad[0x94];

  public:
    Questengine(Settings *settings);
};

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
void Mapcontrol_AddArenaSpawn(Mapcontrol *map, int map_id, int a, int b, int c, int d);
void Mapcontrol_SetArenaBlock(Mapcontrol *map, int map_id, int block);
void Game_Tick(Server *server);
void Players_Tick(Players *players);
String FUN_00473540(Server *server);
String FUN_004731d0(Server *server);
int Players_GetStatTotal(Players *players);
int Players_GetIdleTimeout(Players *players);
int Players_GetActiveCount(Players *players);
String FUN_00403080(TGUI *self, String a, String b, String c);

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
    if (server->Socket->ActiveConnections > Settings::GetMaxConnections(settings) + 5)
    {
        Socket->Close();
        return;
    }
    if (!Logins::HandleAddress(logins, Socket->RemoteAddress))
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

void __fastcall TGUI::FormCreate(TObject *Sender)
{
    String s = DateTimeToStr(Now());
    s.Insert(" ", s.Length() + 1);
    s.Insert(TimeToStr(Now()), s.Length() + 1);
    s.Insert(" EndlServ started\n", s.Length() + 1);
    FILE *boot;
    boot = fopen(".\\logs\\boot.log", "a");
    fprintf(boot, "%s", s.c_str());
    fclose(boot);

    mysql_controls = new Mysqlcontrols();
    if (!Mysqlcontrols::TestConnection(mysql_controls))
    {
        MessageDlg(
            "No mysql database was found (115)", mtError, TMsgDlgButtons() << mbOK, 0);
        Mysqlcontrols::Free(mysql_controls, 3);
        Application->Terminate();
    }

    settings = new Settings();
    serial = new Serial();
    Mysqlcontrols::Connect(mysql_controls, version_patch, version_minor, version_major);
    Serial::SetCounter(serial, 100);

    item_values = new ItemValues();
    npc_values = new NpcValues();
    skill_values = new SkillValues();
    learn_values = new LearnValues();
    shop_values = new ShopValues();
    inn_values = new InnValues();
    class_values = new ClassValues();
    news_control = new Newscontrol();
    jukebox_control = new JukeBoxController();
    players = new Players(settings, mysql_controls);
    map_control = new Mapcontrol(settings);
    quest_engine = new Questengine(settings);
    logins = new Logins(mysql_controls);
    server_ctrl = new Server(map_control,
                             quest_engine,
                             players,
                             settings,
                             mysql_controls,
                             logins,
                             version_patch,
                             version_minor,
                             version_major);
    npc_control = new NpcController(map_control, players, server_ctrl, settings);
    chest_control = new ChestController(map_control, players, server_ctrl, settings);
    effect_control = new EffectController(map_control, players, server_ctrl, settings);
    event_control = new EventController(map_control, players, server_ctrl, settings);
    weddings = new WeddingController(players, server_ctrl);
    door_control = new DoorController(map_control);
    msgboard_control = new MsgBoardController();
    game_control = new Gamecontrol();

    Mapcontrol_AddArenaSpawn(map_control, 0x2e, 0xb, 0x2c, 0xc, 0x18);
    Mapcontrol_AddArenaSpawn(map_control, 0x2e, 0xd, 0x2c, 0xc, 0x11);
    Mapcontrol_AddArenaSpawn(map_control, 0x2e, 0xf, 0x2c, 0xc, 0xa);
    Mapcontrol_AddArenaSpawn(map_control, 0x2e, 0x11, 0x2c, 0x12, 0x18);
    Mapcontrol_AddArenaSpawn(map_control, 0x2e, 0x13, 0x2c, 0x12, 0xa);
    Mapcontrol_AddArenaSpawn(map_control, 0x2e, 0x15, 0x2c, 0x18, 0x18);
    Mapcontrol_AddArenaSpawn(map_control, 0x2e, 0x17, 0x2c, 0x18, 0x11);
    Mapcontrol_AddArenaSpawn(map_control, 0x2e, 0x19, 0x2c, 0x18, 0xa);
    Mapcontrol_AddArenaSpawn(map_control, 0x89, 0x11, 9, 0xe, 0xa);
    Mapcontrol_AddArenaSpawn(map_control, 0x89, 0x11, 0xb, 6, 0xa);
    Mapcontrol_SetArenaBlock(map_control, 0x89, 2);
    Mapcontrol_AddArenaSpawn(map_control, 0x8a, 0x11, 9, 0xe, 0xa);
    Mapcontrol_AddArenaSpawn(map_control, 0x8a, 0x11, 0xb, 6, 0xa);
    Mapcontrol_SetArenaBlock(map_control, 0x8a, 2);
    Mapcontrol_AddArenaSpawn(map_control, 0xb7, 0x12, 0x27, 0x10, 0xc);
    Mapcontrol_AddArenaSpawn(map_control, 0xb7, 0x14, 0x27, 0x10, 0x1c);
    Mapcontrol_AddArenaSpawn(map_control, 0xb7, 0x16, 0x27, 0x16, 0x14);
    Mapcontrol_AddArenaSpawn(map_control, 0xb7, 0x18, 0x27, 0x1c, 0x1c);
    Mapcontrol_AddArenaSpawn(map_control, 0xb7, 0x1a, 0x27, 0x1c, 0xc);
    Mapcontrol_AddArenaSpawn(map_control, 0xb8, 0x12, 0x27, 0x10, 0xc);
    Mapcontrol_AddArenaSpawn(map_control, 0xb8, 0x14, 0x27, 0x10, 0x1c);
    Mapcontrol_AddArenaSpawn(map_control, 0xb8, 0x16, 0x27, 0x16, 0x14);
    Mapcontrol_AddArenaSpawn(map_control, 0xb8, 0x18, 0x27, 0x1c, 0x1c);
    Mapcontrol_AddArenaSpawn(map_control, 0xb8, 0x1a, 0x27, 0x1c, 0xc);

    if (Serial::IsValid(serial))
    {
        server->Port = Settings::GetPort(settings);
        try
        {
            server->Active = true;
        }
        catch (...)
        {
            Application->Terminate();
        }
    }
    if (Serial::GetCounter(serial) > 20)
    {
        tick_counter = 0;
        timer->Interval = 10;
        timer->Enabled = true;
    }
    Caption = Settings::GetServerName(settings);
}

void __fastcall TGUI::timerTimer(TObject *Sender)
{
    tick_counter++;
    if (tick_counter % 10 == 0)
        Game_Tick(server_ctrl);
    if (tick_counter % 10 == 0)
        Players_Tick(players);
    if (tick_counter % 20 == 0)
        NpcController::NpcControl_Tick(npc_control);
    if (tick_counter % 100 == 0)
    {
        Logins::Tick(logins);
        DoorController::Tick(door_control);
        EffectController::Tick(effect_control);
        EventController::Tick(event_control);
        WeddingController::Tick(weddings);
    }
    if (tick_counter % 5000 == 0)
        ChestController::Tick(chest_control);
    if (tick_counter % 1000 == 0)
    {
        Mysqlcontrols::UpdateServerStatus(mysql_controls,
                                          Settings::GetRefreshSeconds(settings),
                                          server->Socket->ActiveConnections,
                                          Players_GetIdleTimeout(players),
                                          Players_GetStatTotal(players),
                                          FUN_004731d0(server_ctrl),
                                          FUN_00473540(server_ctrl));
        if (Visible)
        {
            String s = IntToStr(server->Socket->ActiveConnections) + " con / ";
            s.Insert(IntToStr(Players_GetIdleTimeout(players)) + " players",
                     s.Length() + 1);
            panel_buffer->Caption =
                IntToStr(Mysqlcontrols::Db_GetActiveConnectionCount(mysql_controls)) +
                " sql";
            panel_send->Caption = FUN_004731d0(server_ctrl);
            panel_received->Caption = FUN_00473540(server_ctrl);
            panel_connections->Caption = s;
        }
    }
    if (tick_counter % 10000 == 0)
    {
        if (Serial::GetCounter(serial) < 20)
            server->Active = false;
    }
    if (tick_counter > 100000)
    {
        if (Players_GetActiveCount(players) > 1)
        {
            String expected = FUN_00403080(this,
                                           Serial::GetKeyBaseCopy(serial),
                                           Serial::GetDisplayCode(serial),
                                           Serial::GetUnlockCode(serial));
            if (Serial::GetRegName(serial) != expected)
                server->Active = false;
        }
        tick_counter = 0;
    }
}
