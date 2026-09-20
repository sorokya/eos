#ifndef MainformH
#define MainformH

#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <DBTables.hpp>
#include <ScktComp.hpp>
#include <AppEvnts.hpp>

// Cross-unit application classes (units reconstructed separately). Only the
// pointer types are needed here; the members the TGUI handlers call are
// declared in Mainform.cpp.
class Settings;
class Serial;
class Mapcontrol;
class Mysqlcontrols;
class Players;
class Logins;
class Server;
class NpcController;
class ChestController;
class DoorController;
class EffectController;
class EventController;
class ItemValues;
class NpcValues;
class SkillValues;
class LearnValues;
class ShopValues;
class InnValues;
class ClassValues;
class MsgBoardController;
class JukeBoxController;
class WeddingController;
class Questengine;
class Newscontrol;
class Gamecontrol;

// TGUI is the main form (class name from the embedded DFM/RTTI, unit name
// "MainForm" from the class RTTI, export @@Mainform@Initialize). The published
// fields are in DFM field-table order; the offsets below are pinned by the
// reference VCL field table (VA 0x55c460) and the handler accesses.
class TGUI : public TForm
{
    __published : TServerSocket *server;    // +0x2d0
    TDatabase *mysql;                       // +0x2d4
    TQuery *myquery;                        // +0x2d8
    TSession *mysession;                    // +0x2dc
    TTimer *timer;                          // +0x2e0
    TPanel *panel_connections;              // +0x2e4
    TPanel *panel_send;                     // +0x2e8
    TPanel *panel_received;                 // +0x2ec
    TPanel *panel_buffer;                   // +0x2f0
    TApplicationEvents *ApplicationEvents1; // +0x2f4

  public:
    Settings *settings;                   // +0x2f8
    Serial *serial;                       // +0x2fc
    Mapcontrol *map_control;              // +0x300
    Mysqlcontrols *mysql_controls;        // +0x304
    Players *players;                     // +0x308
    Logins *logins;                       // +0x30c
    Server *server_ctrl;                  // +0x310
    NpcController *npc_control;           // +0x314
    ChestController *chest_control;       // +0x318
    DoorController *door_control;         // +0x31c
    EffectController *effect_control;     // +0x320
    EventController *event_control;       // +0x324
    int version_patch;                    // +0x328
    int version_minor;                    // +0x32c
    int version_major;                    // +0x330
    int tick_counter;                     // +0x334
    ItemValues *item_values;              // +0x338
    NpcValues *npc_values;                // +0x33c
    SkillValues *skill_values;            // +0x340
    LearnValues *learn_values;            // +0x344
    ShopValues *shop_values;              // +0x348
    InnValues *inn_values;                // +0x34c
    ClassValues *class_values;            // +0x350
    MsgBoardController *msgboard_control; // +0x354
    JukeBoxController *jukebox_control;   // +0x358
    WeddingController *weddings;          // +0x35c
    Questengine *quest_engine;            // +0x360
    Newscontrol *news_control;            // +0x364
    Gamecontrol *game_control;            // +0x368
    int field_0x36c;                      // +0x36c
    int field_0x370;                      // +0x370
    int field_0x374;                      // +0x374

    __fastcall TGUI(TComponent *Owner);
    __fastcall void FormCreate(TObject *Sender);
    __fastcall void serverClientConnect(TObject *Sender, TCustomWinSocket *Socket);
    __fastcall void serverClientDisconnect(TObject *Sender, TCustomWinSocket *Socket);
    __fastcall void serverClientRead(TObject *Sender, TCustomWinSocket *Socket);
    __fastcall void serverClientError(TObject *Sender,
                                      TCustomWinSocket *Socket,
                                      TErrorEvent ErrorEvent,
                                      int &ErrorCode);
    __fastcall void timerTimer(TObject *Sender);
    __fastcall void FormClose(TObject *Sender, TCloseAction &Action);
    __fastcall void ApplicationEvents1Exception(TObject *Sender, Exception *E);
};

extern PACKAGE TGUI *GUI;
extern TGUI **MAINFORM;

Server *Mainform_GetServer(TGUI *form);

#endif
