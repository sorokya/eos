#ifndef NpcvalueH
#define NpcvalueH

#include <Classes.hpp>
#include <vector>
#include "Npcdrop.h"

// Recovered from the reference (Npcvalue unit, 0x4a8f84..0x4a9ac8). The
// constructor default-constructs `name` (offset 4) and the drop/talk vectors
// (offsets 0x3c/0x5c), stores the npc id at offset 0 and clears both vectors;
// the destructor destroys the talk vector, the drop vector then `name`. The
// element size of std::vector<NpcValue> is 0x7c, matching this layout; every
// offset is pinned by the parsers (Pub_LoadNpcs/AddNpc 0x4a5654/0x4a6f68) and
// the accessors (Enf_GetExp/MaxHp/Type 0x4a8be4/0x4a8c34/0x4a8c88) and the
// implicit copy constructor (0x4a7528). Fields widen the on-disk EnfRecord
// (eo-protocol): each decoded char/short/three is stored as its machine width
// in memory. The class name is the RTTI type name ("NpcValue").
struct NpcValue
{
    int id;
    String name;
    short graphic_id;
    short race;
    short boss;
    short child;
    short npc_type;
    short behavior_id;
    int hp;
    short tp;
    short min_damage;
    short max_damage;
    short accuracy;
    short evade;
    short armor;
    short return_damage;
    short element;
    short element_damage;
    short element_weakness;
    short element_weakness_damage;
    short level;
    int experience;
    int talk_rate;
    char has_talk;
    std::vector<NpcDropItem> drops;
    std::vector<String> talk_lines;

    NpcValue();
    NpcValue(int id);
    ~NpcValue();

    void AddDrop(int item_id, int min_amount, int max_amount, int rate);
    void AddTalkMessage(int rate, String message);
};

#endif
