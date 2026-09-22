#ifndef ServerH
#define ServerH

#include <Classes.hpp>

#include "Players.h"
#include "Mysqlcontrols.h"
#include "Logins.h"
#include "Banned.h"
#include "Questengine.h"
#include "Mapcontrol.h"
#include "Settings.h"
#include "Killcounters.h"
#include "Questcounters.h"
#include "Weaponmap.h"

// The application core. Layout recovered from the reference server constructor
// (Packets unit, 0x41670c) and the per-tick loop. sizeof is 0xc8, pinned by the
// `operator new(0xc8)` in Mainform's FormCreate.
class Packets
{
  public:
    int state_0x00;                // +0x00
    int state_0x04;                // +0x04
    TStringList *wordfilter;       // +0x08
    WeaponMapper *weapon_map;      // +0x0c
    Players *players;              // +0x10
    mySQLdb *mysql_controls;       // +0x14
    Logins *logins;                // +0x18
    Banned *banned;                // +0x1c
    QuestContainer *quest_engine;  // +0x20
    MapContainer *map_control;     // +0x24
    Settings *settings;            // +0x28
    KillCounters *kill_counters;   // +0x2c
    QuestCounters *quest_counters; // +0x30
    int pad_0x34;                  // +0x34
    TDateTime start_time;          // +0x38
    char *encode_buffer;           // +0x40
    char *packet_buffer;           // +0x44
    int version_patch;             // +0x48
    int version_minor;             // +0x4c
    int version_major;             // +0x50
    int sent_bytes;                // +0x54
    int sent_kilobytes;            // +0x58
    int sent_megabytes;            // +0x5c
    int received_bytes;            // +0x60
    int received_kilobytes;        // +0x64
    int received_megabytes;        // +0x68
    String reader_data;            // +0x6c
    int reader_pos;                // +0x70
    int reader_len;                // +0x74
    char reader_break_byte;        // +0x78
    int online_names_ttl;          // +0x7c
    String online_names_cache;     // +0x80
    int online_list_ttl;           // +0x84
    String online_list_cache;      // +0x88
    String field_0x8c[7];          // +0x8c
    int ping_history[3];           // +0xa8
    int ticks;                     // +0xb4
    char hangup_gate;              // +0xb8
    char kill_counters_cleared;    // +0xb9
    char shutting_down;            // +0xba
    char pad_0xbb[1];              // +0xbb
    int cheat_offset_x;            // +0xbc
    int cheat_offset_y;            // +0xc0
    int pad_0xc4;                  // +0xc4

    Packets(MapContainer *map_control,
            QuestContainer *quest_engine,
            Players *players,
            Settings *settings,
            mySQLdb *mysql_controls,
            Logins *logins,
            int version_patch,
            int version_minor,
            int version_major);
    ~Packets();
};

#endif
