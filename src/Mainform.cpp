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

#include "Mapcontrol.h"
#include "Players.h"
#include "Packets.h"
#include "Questengine.h"

#pragma package(smart_init)

String FUN_00403080(TGUI *self, String key_base, String display_code, String unlock_code);

// Non-PACKAGE redeclaration keeps `&GUI` a link-time constant, so bcc emits a
// static `.data` relocation (matching reference slot 0x58b60c) rather than the
// package-aware runtime initializer the PACKAGE declaration would produce.
extern TGUI *GUI;
TGUI **MAINFORM = &GUI;

Server *Mainform_GetServer(TGUI *form)
{
    return form->server_ctrl;
}

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
        Players::Players_MarkRemoving(players, Socket);
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
    if (!Players::Players_Add(players, Socket))
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
        Players::Players_Remove(players, Socket);
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
    Mapcontrol::Mapcontrol_SetArenaBlock(map_control, 0x89, 2);
    Mapcontrol_AddArenaSpawn(map_control, 0x8a, 0x11, 9, 0xe, 0xa);
    Mapcontrol_AddArenaSpawn(map_control, 0x8a, 0x11, 0xb, 6, 0xa);
    Mapcontrol::Mapcontrol_SetArenaBlock(map_control, 0x8a, 2);
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
        Players::Players_Tick(players);
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
                                          Players::Players_GetIdleTimeout(players),
                                          Players::Players_GetStatTotal(players),
                                          Server_FormatSentTraffic(server_ctrl),
                                          Server_FormatReceivedTraffic(server_ctrl));
        if (Visible)
        {
            String s = IntToStr(server->Socket->ActiveConnections) + " con / ";
            s.Insert(IntToStr(Players::Players_GetIdleTimeout(players)) + " players",
                     s.Length() + 1);
            panel_buffer->Caption =
                IntToStr(Mysqlcontrols::Db_GetActiveConnectionCount(mysql_controls)) +
                " sql";
            panel_send->Caption = Server_FormatSentTraffic(server_ctrl);
            panel_received->Caption = Server_FormatReceivedTraffic(server_ctrl);
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
        if (Players::Players_GetActiveCount(players) > 1)
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
