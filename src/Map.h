#ifndef MapH
#define MapH

#include <Classes.hpp>
#include <vector.h>

#include "Itemground.h"
#include "Mapchest.h"
#include "Mapobject.h"
#include "Mapwarp.h"
#include "Npc.h"

// The map record. The name is ChestItem, not anything map-ish: it is the only
// struct left to pair with the reference's unmatched by-value RTTI descriptor
// `vector<ChestItem,allocator<ChestItem> > *`, which is Mapcontrol's map list.
// Layout recovered from the reference (Map unit, 0x487e84..0x488474). The
// constructor stores rid/width/height and the scalar flags, constructs the
// vector and AnsiString members in declaration order, then resizes the
// walkability bit array (0x130) to width*height*2 bits and clears the two
// tile-spec lists. Every offset is pinned by the constructor/destructor stores.
struct ChestItem
{
    unsigned short rid;                          // +0x00
    short rid1;                                  // +0x02
    short rid2;                                  // +0x04
    short filesize;                              // +0x06
    unsigned char width;                         // +0x08
    unsigned char height;                        // +0x09
    char map_type;                               // +0x0a
    unsigned char timed_effect;                  // +0x0b
    bool has_quakes;                             // +0x0c
    bool has_hp_drain;                           // +0x0d
    bool has_tp_drain;                           // +0x0e
    bool has_spikes;                             // +0x0f
    bool arena_enabled;                          // +0x10
    int arena_block;                             // +0x14
    int arena_ticks;                             // +0x18
    int quest_cooldown;                          // +0x1c
    int evac_countdown;                          // +0x20
    vector<MapWarp> arena_spawn_list;       // +0x24
    int relog_x;                                 // +0x44
    int relog_y;                                 // +0x48
    short field_0x4c;                            // +0x4c
    bool buf_copied;                             // +0x4e
    bool can_scroll;                             // +0x4f
    String buf;                                  // +0x50
    String hp_drain_others;                      // +0x54
    char hp_drain_others_sent;                   // +0x58
    vector<MapObject> tile_specs;           // +0x5c
    vector<MapObject> legacy_door_key_list; // +0x7c
    vector<MapChest> chest_list;            // +0x9c
    vector<MapWarp> warp_list;              // +0xbc
    vector<Npc *> npc_list;                 // +0xdc
    vector<ItemObj *> ground_items;       // +0xfc
    int next_ground_item_id;                     // +0x11c
    short child_npc_id;                          // +0x120
    bool boss_alive;                             // +0x122
    char npc_dirty;                              // +0x123
    char chests_dirty;                           // +0x124
    char has_open_doors;                         // +0x125
    int npc_act_ticks;                           // +0x128
    int player_count;                            // +0x12c
    vector<bool> tile_bits;                 // +0x130

    ChestItem(int map_id, int width, int height);
    ~ChestItem();
};

#endif
