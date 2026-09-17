#include <vcl.h>
#pragma hdrstop

#include "Map.h"

#pragma package(smart_init)

MapContainer::MapContainer(short map_id, int width, int height)
{
    rid = map_id;
    this->width = width;
    this->height = height;
    field_0x4c = 0;
    buf_copied = false;
    npc_dirty = 0;
    boss_alive = false;
    chests_dirty = 0;
    has_open_doors = 0;
    can_scroll = false;
    arena_enabled = 0;
    child_npc_id = 0;
    player_count = 0;
    npc_act_ticks = 0;
    next_ground_item_id = 0;
    quest_cooldown = 0;
    evac_countdown = 0;
    has_quakes = false;
    has_hp_drain = false;
    has_tp_drain = false;
    has_spikes = false;
    relog_x = 0;
    relog_y = 0;
    hp_drain_others_sent = 0;
    hp_drain_others = "";
    tile_bits.resize(this->width * this->height * 2);
    legacy_door_key_list.clear();
    chest_list.clear();
}

MapContainer::~MapContainer()
{
}
