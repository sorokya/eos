#include <vcl.h>
#include <stdio.h>
#pragma hdrstop

#include "MainForm.h"
#include "Serial.h"
#include "Protocol.h"

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

TGUI *GUI;

String Mainform_ComputeRegName(TGUI *self,
                               String key_base,
                               String display_code,
                               String unlock_code);

__fastcall TGUI::TGUI(TComponent *Owner) : TForm(Owner)
{
    version_patch = 0;
    version_minor = 0;
    version_major = 0x1c;
}

void __fastcall TGUI::FormCreate(TObject *Sender)
{
    String s = DateToStr(Now());
    s.Insert(" ", s.Length() + 1);
    s.Insert(TimeToStr(Now()), s.Length() + 1);
    s.Insert(" EndlServ started\n", s.Length() + 1);
    FILE *boot;
    boot = fopen(".\\logs\\boot.log", "a");
    fprintf(boot, "%s", s.c_str());
    fclose(boot);

    mysql_controls = new mySQLdb();
    if (!mySQLdb::TestConnection(mysql_controls))
    {
        MessageDlg(
            "No mysql database was found (115)", mtError, TMsgDlgButtons() << mbOK, 0);
        delete mysql_controls;
        Application->Terminate();
    }

    settings = new Settings();
    serial = new SerialKey();
    mySQLdb::Connect(mysql_controls, version_patch, version_minor, version_major);
    SerialKey::SetCounter(serial, 100);

    item_values = new ItemValues();
    npc_values = new NpcValues();
    skill_values = new SkillValues();
    learn_values = new LearnValues();
    shop_values = new ShopValues();
    inn_values = new InnValues();
    class_values = new ClassValues();
    news_control = new NewsTopics();
    jukebox_control = new JukeBoxController();
    players = new Players(settings, mysql_controls);
    map_control = new MapContainer(settings);
    quest_engine = new QuestContainer(settings);
    logins = new Logins(mysql_controls);
    server_ctrl = new Packets(map_control,
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
    game_control = new Game();

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
    MapContainer::Mapcontrol_SetArenaBlock(map_control, 0x89, 2);
    Mapcontrol_AddArenaSpawn(map_control, 0x8a, 0x11, 9, 0xe, 0xa);
    Mapcontrol_AddArenaSpawn(map_control, 0x8a, 0x11, 0xb, 6, 0xa);
    MapContainer::Mapcontrol_SetArenaBlock(map_control, 0x8a, 2);
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

    if (SerialKey::IsValid(serial))
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
    if (SerialKey::GetCounter(serial) > 20)
    {
        tick_counter = 0;
        timer->Interval = 10;
        timer->Enabled = true;
    }
    Caption = Settings::GetServerName(settings);
}

Packets *Mainform_GetServer(TGUI *form)
{
    return form->server_ctrl;
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

void __fastcall TGUI::serverClientError(TObject *Sender,
                                        TCustomWinSocket *Socket,
                                        TErrorEvent ErrorEvent,
                                        int &ErrorCode)
{
    ErrorCode = 0;
    if (Socket->SocketHandle < 1 || Socket->SocketHandle >= SOCKET_HANDLE_MAX)
        Socket->Close();
    else
        Players::Players_MarkRemoving(players, Socket);
}

void __fastcall TGUI::serverClientDisconnect(TObject *Sender, TCustomWinSocket *Socket)
{
    if (Socket->SocketHandle >= 1 && Socket->SocketHandle < SOCKET_HANDLE_MAX)
    {
        Server_RemovePlayer(server_ctrl, Socket);
        Players::Players_Remove(players, Socket);
    }
}

void __fastcall TGUI::serverClientRead(TObject *Sender, TCustomWinSocket *Socket)
{
    if (Socket->SocketHandle < 1 || Socket->SocketHandle >= SOCKET_HANDLE_MAX)
    {
        Socket->Close();
        return;
    }
    Server_ClientRead(server_ctrl, Socket, Socket->ReceiveText());
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
        mySQLdb::UpdateServerStatus(mysql_controls,
                                    Settings::GetRefreshSeconds(settings),
                                    server->Socket->ActiveConnections,
                                    Players::Players_GetOnlineCount(players),
                                    Players::Players_GetPeakOnline(players),
                                    Server_FormatSentTraffic(server_ctrl),
                                    Server_FormatReceivedTraffic(server_ctrl));
        if (Visible)
        {
            String s = IntToStr(server->Socket->ActiveConnections) + " con / ";
            s.Insert(IntToStr(Players::Players_GetOnlineCount(players)) + " players",
                     s.Length() + 1);
            panel_buffer->Caption =
                IntToStr(mySQLdb::Db_GetActiveConnectionCount(mysql_controls)) + " sql";
            panel_send->Caption = Server_FormatSentTraffic(server_ctrl);
            panel_received->Caption = Server_FormatReceivedTraffic(server_ctrl);
            panel_connections->Caption = s;
        }
    }
    if (tick_counter % 10000 == 0)
    {
        if (SerialKey::GetCounter(serial) < 20)
            server->Active = false;
    }
    if (tick_counter > 100000)
    {
        if (Players::Players_GetActiveCount(players) > 1)
        {
            String expected = Mainform_ComputeRegName(this,
                                                      SerialKey::GetKeyBaseCopy(serial),
                                                      SerialKey::GetDisplayCode(serial),
                                                      SerialKey::GetUnlockCode(serial));
            if (SerialKey::GetRegName(serial) != expected)
                server->Active = false;
        }
        tick_counter = 0;
    }
}

String Mainform_ComputeRegName(TGUI *self,
                               String key_base,
                               String display_code,
                               String unlock_code)
{
    if (key_base.Length() < 1 || display_code.Length() < 1 || unlock_code.Length() < 1)
        return "";
    String result = "";
    String part = "";
    int sum = 0xd;
    bool again = true;
    do
    {
        for (int i = 1; i <= key_base.Length(); i++)
            sum += (unsigned char)key_base[i];
        for (int i = 1; i <= display_code.Length(); i++)
            sum += (unsigned char)display_code[i];
        for (int i = 1; i <= unlock_code.Length(); i++)
            sum += (unsigned char)unlock_code[i];
        sum += 2;
        if (key_base.Length() == 0 && display_code.Length() == 0)
            again = false;
        if (key_base.Length() > 0)
            key_base.Delete(1, 1);
        if (display_code.Length() > 0)
            display_code.Delete(1, 1);
        if (unlock_code.Length() > 0)
            unlock_code.Delete(1, 1);
        sum += 3;
    } while (again);
    part = IntToHex(sum * 0x1040, 2);
    if (part.Length() > 2)
        part = part.SubString(part.Length() - 2, 2);
    result = result + part;
    sum += 2;
    part = IntToHex(sum * 0xd91, 3);
    if (part.Length() > 2)
        part = part.SubString(part.Length() - 3, 3);
    result = result + part;
    sum += 3;
    part = IntToHex(sum * 0x872, 4);
    if (part.Length() > 2)
        part = part.SubString(part.Length() - 4, 4);
    result = result + part;
    sum += 4;
    part = IntToHex(sum * 0x157e, 3);
    if (part.Length() > 2)
        part = part.SubString(part.Length() - 3, 3);
    return result + part;
}

void __fastcall TGUI::FormClose(TObject *Sender, TCloseAction &Action)
{
    Server_Shutdown(server_ctrl);
    Action = caNone;
}

void __fastcall TGUI::ApplicationEvents1Exception(TObject *Sender, Exception *E)
{
    String s = DateToStr(Now());
    s.Insert(" ", s.Length() + 1);
    s.Insert(TimeToStr(Now()), s.Length() + 1);
    s.Insert(" EndlServ ", s.Length() + 1);
    s.Insert(E->Message, s.Length() + 1);
    s.Insert(" ", s.Length() + 1);
    s.Insert(IntToStr(last_packet_action), s.Length() + 1);
    s.Insert(",", s.Length() + 1);
    s.Insert(IntToStr(last_packet_family), s.Length() + 1);
    s.Insert("\n", s.Length() + 1);
    FILE *fp;
    fp = fopen(".\\logs\\error.log", "a");
    fprintf(fp, "%s", s.c_str());
    fclose(fp);
}
