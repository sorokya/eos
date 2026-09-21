#ifndef NpcvaluesH
#define NpcvaluesH

#include <Classes.hpp>
#include <vector.h>
#include "Npcvalue.h"

int RandRange(int max);

// The two-int result returned by NpcValues::GetType: {npc_type, behavior_id}. The
// reference calls an empty user constructor on the local at entry (the folded
// EH-frame-only constructor at 0x44f58c) before storing -1 in both fields, then
// returns it in the caller's return slot; the same shape the Itemvalues pair
// records use.
struct NpcTypeInfo
{
    struct
    {
        int type;
        int behavior_id;
    };
    NpcTypeInfo()
    {
    }
};

// The two-int result returned by Npc_GetDrop: {item_id, amount}. Same empty
// user constructor / by-value return shape as NpcTypeInfo.
struct NpcDropInfo
{
    struct
    {
        int item_id;
        int amount;
    };
    NpcDropInfo()
    {
    }
};

// The npc table (ENF/EDF/ETF). Layout recovered from the reference constructor
// (0x4a5420) and the parsers/accessors:
//   +0x00 int                       file_id = file - 1 (files loaded)
//   +0x04 int                       rid_1
//   +0x08 int                       rid_2
//   +0x0c int                       num_records (total npcs from the first ENF)
//   +0x10 char                      loaded
//   +0x11 char                      drops_loaded
//   +0x12 char                      talk_loaded
//   +0x14 TStringList *             string_list (each loaded ENF blob)
//   +0x18 void *                    field_0x18 = operator new(8)
//   +0x1c vector<NpcValue>     record_list
//   +0x3c int                       field_0x3c = -1
class NpcValues
{
  public:
    int file_id;
    int num_records;
    int rid_1;
    int rid_2;
    char loaded;
    char drops_loaded;
    char talk_loaded;
    char pad_0x13;
    TStringList *string_list;
    void *field_0x18;
    vector<NpcValue> record_list;
    int field_0x3c;

    NpcValues();
    ~NpcValues();

    int DecodeNumber(String value);

    static void LoadNpcs(NpcValues *self);
    static void LoadDrops(NpcValues *self);
    static void LoadTalk(NpcValues *self);
    static void AddDrop(NpcValues *self,
                        int npc_id,
                        int item_id,
                        int min_amount,
                        int max_amount,
                        int rate);
    static void SetTalk(NpcValues *self, int npc_id, int rate, String message);
    static String RollTalk(NpcValues *self, int enf_id);
    static void AddNpc(NpcValues *self,
                       int id,
                       String name,
                       short graphic_id,
                       short race,
                       short boss,
                       short child,
                       short npc_type,
                       short behavior_id,
                       int hp,
                       short tp,
                       short min_damage,
                       short max_damage,
                       short accuracy,
                       short evade,
                       short armor,
                       short return_damage,
                       short element,
                       short element_damage,
                       short element_weakness,
                       short element_weakness_damage,
                       short level,
                       int experience);
    static int GetCount(NpcValues *self);
    static NpcValue GetNpc(NpcValues *self, int id);
    static NpcTypeInfo GetType(NpcValues *self, int enf_id);
    static NpcDropInfo GetDrop(NpcValues *self, int npc_id);
    static int GetExp(NpcValues *self, int npc_id);
    static int GetMaxHp(NpcValues *self, int npc_id);
};

#endif
