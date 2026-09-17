#ifndef MapH
#define MapH

#include <Classes.hpp>
#include <vector>

#include "Mapchest.h"
#include "Mapobject.h"
#include "Mapwarp.h"
#include "Npc.h"

struct GroundItem;

// Layout recovered from the reference (Map unit, 0x487e84..0x488474). The
// constructor stores rid/width/height and the scalar flags, constructs the
// vector and AnsiString members in declaration order, then resizes the
// walkability bit array (0x130) to width*height*2 bits and clears the two
// tile-spec lists. Every offset is pinned by the constructor/destructor stores.
struct Map
{
    short rid;            // +0x00
    short rid1;           // +0x02
    short rid2;           // +0x04
    short filesize;       // +0x06
    unsigned char width;  // +0x08
    unsigned char height; // +0x09
    char map_type;        // +0x0a
    char timed_effect;    // +0x0b
    bool has_quakes;      // +0x0c
    bool has_hp_drain;    // +0x0d
    bool has_tp_drain;    // +0x0e
    bool has_spikes;      // +0x0f
    bool arena_enabled;   // +0x10
    char pad_11[3];
    int arena_block;                       // +0x14
    int arena_ticks;                       // +0x18
    int field_0x1c;                        // +0x1c
    int evac_countdown;                    // +0x20
    std::vector<Mapwarp> arena_spawn_list; // +0x24
    int relog_x;                           // +0x44
    int relog_y;                           // +0x48
    short field_0x4c;                      // +0x4c
    bool buf_copied;                       // +0x4e
    bool can_scroll;                       // +0x4f
    String buf;                            // +0x50
    String field_0x54;                     // +0x54
    char field_0x58;                       // +0x58
    char pad_59[3];
    std::vector<Mapobject> tile_specs;           // +0x5c
    std::vector<Mapobject> legacy_door_key_list; // +0x7c
    std::vector<Mapchest> chest_list;            // +0x9c
    std::vector<Mapwarp> warp_list;              // +0xbc
    std::vector<Npc *> npc_list;                 // +0xdc
    std::vector<GroundItem *> ground_items;      // +0xfc
    int next_ground_item_id;                     // +0x11c
    short child_npc_id;                          // +0x120
    bool boss_alive;                             // +0x122
    char field_0x123;                            // +0x123
    char chests_dirty;                           // +0x124
    char has_open_doors;                         // +0x125
    char pad_126[2];
    int npc_act_ticks;           // +0x128
    int player_count;            // +0x12c
    std::vector<bool> tile_bits; // +0x130

    Map(short map_id, int width, int height);
    ~Map();
};

#endif
