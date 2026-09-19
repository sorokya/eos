#include <vcl.h>
#pragma hdrstop

#include "Mapcontrol.h"
#include "Mainform.h"
#include "Npc.h"
#include "Npcvalue.h"
#include "Npcvalues.h"
#include "Settings.h"
#include "Protocol.h"

#pragma package(smart_init)

// Cross-unit (Packets) helpers. Their definitions live in the Packets unit; the
// controllers declare the same prototypes.
int Mapcontrol_GetCount(Mapcontrol *map_control);
MapContainer *Mapcontrol_GetByIndex(Mapcontrol *map_control, int index);

// Cross-unit helpers owned by other units (Jukeboxcontrol, Mapcontrol).
void FUN_004aa4e4(JukeBoxController *jukebox_control, int map_id);
int FUN_00482834(Mapcontrol *map_control, MapContainer *map, int map_id);
int FUN_004813c8(void *list);
void FUN_0048441c(void *list, int count, int value);
void *Map_NpcIter_End(void *npc_list);
void FUN_004a9e74(JukeBoxController *jukebox_control, int map_id);
MapContainer Map_InitBlank(int map_id, int width, int height);
MapContainer *MapVector_End(Mapcontrol *map_control);
void FUN_0048835c(MapContainer *map, int flag);

typedef std::vector<ChestItem *> GroundItemPtrVector;

Mapcontrol::Mapcontrol(Settings *settings)
{
    encode_scratch = (char *)operator new(8);
    this->settings = settings;
    start_map = Settings::GetStartMap(this->settings);
    start_x = Settings::GetStartX(this->settings);
    start_y = Settings::GetStartY(this->settings);
    rescue_map = Settings::GetRescueMap(this->settings);
    rescue_x = Settings::GetRescueX(this->settings);
    rescue_y = Settings::GetRescueY(this->settings);
    max_maps = Settings::GetMaxMaps(this->settings);
    memory_map = Settings::GetMemoryMap(this->settings);
    Mapcontrol_LoadMaps(this);
}

int Mapcontrol::Map_GetWarpMap(Mapcontrol *map_control, int map_id, int x, int y)
{
    int result = 0;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount(map_control))
    {
        int tile_offset =
            Mapcontrol_GetByIndex(map_control, map_id - 1)->width * 2 * y + x * 2;
        if (Mapcontrol_GetByIndex(map_control, map_id - 1)->tile_bits[tile_offset])
        {
            for (std::vector<MapWarp>::iterator warp_iter =
                     Mapcontrol_GetByIndex(map_control, map_id - 1)->warp_list.begin();
                 warp_iter !=
                 Mapcontrol_GetByIndex(map_control, map_id - 1)->warp_list.end();
                 warp_iter++)
            {
                if (warp_iter->from_x == x && warp_iter->from_y == y)
                {
                    result = warp_iter->dest_map;
                    break;
                }
            }
        }
    }
    return result;
}

int Mapcontrol::Map_GetWarpLevelReq(Mapcontrol *map_control, int map_id, int x, int y)
{
    int result = 1000;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount(map_control))
    {
        for (std::vector<MapWarp>::iterator warp_iter =
                 Mapcontrol_GetByIndex(map_control, map_id - 1)->warp_list.begin();
             warp_iter != Mapcontrol_GetByIndex(map_control, map_id - 1)->warp_list.end();
             warp_iter++)
        {
            if (warp_iter->from_x == x && warp_iter->from_y == y)
            {
                result = warp_iter->level;
                break;
            }
        }
    }
    return result;
}

int Mapcontrol::Map_GetWarpX(Mapcontrol *map_control, int map_id, int x, int y)
{
    int result = 1000;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount(map_control))
    {
        for (std::vector<MapWarp>::iterator warp_iter =
                 Mapcontrol_GetByIndex(map_control, map_id - 1)->warp_list.begin();
             warp_iter != Mapcontrol_GetByIndex(map_control, map_id - 1)->warp_list.end();
             warp_iter++)
        {
            if (warp_iter->from_x == x && warp_iter->from_y == y)
            {
                result = warp_iter->to_x;
                break;
            }
        }
    }
    return result;
}

int Mapcontrol::Map_GetWarpY(Mapcontrol *map_control, int map_id, int x, int y)
{
    int result = 1000;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount(map_control))
    {
        for (std::vector<MapWarp>::iterator warp_iter =
                 Mapcontrol_GetByIndex(map_control, map_id - 1)->warp_list.begin();
             warp_iter != Mapcontrol_GetByIndex(map_control, map_id - 1)->warp_list.end();
             warp_iter++)
        {
            if (warp_iter->from_x == x && warp_iter->from_y == y)
            {
                result = warp_iter->to_y;
                break;
            }
        }
    }
    return result;
}

unsigned int
Mapcontrol::Map_GetTileSpec(Mapcontrol *map_control, int map_id, int x, int y)
{
    unsigned int result = 0xffffffff;
    for (std::vector<MapObject>::iterator spec_iter =
             Mapcontrol_GetByIndex(map_control, map_id - 1)->tile_specs.begin();
         spec_iter != Mapcontrol_GetByIndex(map_control, map_id - 1)->tile_specs.end();
         spec_iter++)
    {
        if ((unsigned short)spec_iter->x == x && (unsigned short)spec_iter->y == y)
        {
            result = (unsigned short)spec_iter->value;
            break;
        }
    }
    return result;
}

bool Mapcontrol::Map_IsOccupied(Mapcontrol *map_control, int map_id, int x, int y)
{
    bool result = false;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount(map_control))
    {
        for (std::vector<Npc *>::iterator npc_iter =
                 Mapcontrol_GetByIndex(map_control, map_id - 1)->npc_list.begin();
             npc_iter != Mapcontrol_GetByIndex(map_control, map_id - 1)->npc_list.end();
             npc_iter++)
        {
            if ((*npc_iter)->x == x && (*npc_iter)->y == y && (*npc_iter)->alive != false)
            {
                result = true;
                break;
            }
        }
    }
    return result;
}

bool Mapcontrol::Map_IsTileClear(Mapcontrol *map_control, int map_id, int x, int y)
{
    bool result = true;
    if (map_id > 0 && Mapcontrol_GetCount(map_control) >= map_id)
    {
        int tile_offset =
            Mapcontrol_GetByIndex(map_control, map_id - 1)->width * 2 * y + x * 2;
        if (x >= 0 && y >= 0 &&
            x < (int)Mapcontrol_GetByIndex(map_control, map_id - 1)->width &&
            y < (int)Mapcontrol_GetByIndex(map_control, map_id - 1)->height)
        {
            if (Mapcontrol_GetByIndex(map_control, map_id - 1)->tile_bits[tile_offset])
            {
                if (!Mapcontrol_GetByIndex(map_control, map_id - 1)
                         ->tile_bits[tile_offset + 1])
                    result = false;
            }
            else
            {
                if (Mapcontrol_GetByIndex(map_control, map_id - 1)
                        ->tile_bits[tile_offset + 1])
                    result = false;
            }
        }
        else
        {
            result = false;
        }
    }
    return result;
}

bool Mapcontrol::Map_IsTileWalkable(Mapcontrol *map_control, int map_id, int x, int y)
{
    bool result = false;
    if (map_id > 0 && Mapcontrol_GetCount(map_control) >= map_id)
    {
        int tile_offset =
            Mapcontrol_GetByIndex(map_control, map_id - 1)->width * 2 * y + x * 2;
        if (x >= 0 && y >= 0 &&
            x < (int)Mapcontrol_GetByIndex(map_control, map_id - 1)->width &&
            y < (int)Mapcontrol_GetByIndex(map_control, map_id - 1)->height)
        {
            if (Mapcontrol_GetByIndex(map_control, map_id - 1)->tile_bits[tile_offset])
            {
                if (Mapcontrol_GetByIndex(map_control, map_id - 1)
                        ->tile_bits[tile_offset + 1])
                {
                    result = true;
                }
                else
                {
                    for (std::vector<MapObject>::iterator spec_iter =
                             Mapcontrol_GetByIndex(map_control, map_id - 1)
                                 ->tile_specs.begin();
                         spec_iter !=
                         Mapcontrol_GetByIndex(map_control, map_id - 1)->tile_specs.end();
                         spec_iter++)
                    {
                        if ((unsigned short)spec_iter->x == x &&
                            (unsigned short)spec_iter->y == y)
                        {
                            if ((unsigned short)spec_iter->value ==
                                    MapTileSpec_Reserved10 ||
                                (unsigned short)spec_iter->value ==
                                    MapTileSpec_Reserved11)
                                result = true;
                            break;
                        }
                    }
                }
            }
        }
    }
    return result;
}

int Mapcontrol::Map_IsWalkableNPC(
    Mapcontrol *map_control, int map_id, int x, int y, char ignore_spec_block)
{
    int result = 1;
    if (map_id > 0 && Mapcontrol_GetCount(map_control) >= map_id)
    {
        int tile_offset =
            Mapcontrol_GetByIndex(map_control, map_id - 1)->width * 2 * y + x * 2;
        if (x >= 0 && y >= 0 &&
            x < (int)Mapcontrol_GetByIndex(map_control, map_id - 1)->width &&
            y < (int)Mapcontrol_GetByIndex(map_control, map_id - 1)->height)
        {
            if (Mapcontrol_GetByIndex(map_control, map_id - 1)->tile_bits[tile_offset])
            {
                if (Mapcontrol_GetByIndex(map_control, map_id - 1)
                        ->tile_bits[tile_offset + 1])
                {
                    result = 2;
                }
                else
                {
                    result = 1;
                    for (std::vector<MapObject>::iterator spec_iter =
                             Mapcontrol_GetByIndex(map_control, map_id - 1)
                                 ->tile_specs.begin();
                         spec_iter !=
                         Mapcontrol_GetByIndex(map_control, map_id - 1)->tile_specs.end();
                         spec_iter++)
                    {
                        if ((unsigned short)spec_iter->x == x &&
                            (unsigned short)spec_iter->y == y)
                        {
                            if ((unsigned short)spec_iter->value == MapTileSpec_Chest)
                                result = 0;
                            if ((unsigned short)spec_iter->value ==
                                MapTileSpec_Reserved11)
                                result = 2;
                            if ((unsigned short)spec_iter->value != MapTileSpec_BankVault)
                                break;
                            if (ignore_spec_block != 0)
                                result = 0;
                            break;
                        }
                    }
                }
            }
            else
            {
                if (Mapcontrol_GetByIndex(map_control, map_id - 1)
                        ->tile_bits[tile_offset + 1])
                    result = 1;
                else
                    result = 0;
            }
        }
    }
    return result;
}

void Mapcontrol::Mapcontrol_inc_player_count(Mapcontrol *map_control, int map_id)
{
    if (map_id > 0 && map_id <= Mapcontrol_GetCount(map_control))
    {
        Mapcontrol_GetByIndex(map_control, map_id - 1)->player_count++;
        Mapcontrol_GetByIndex(map_control, map_id - 1)->npc_act_ticks = 0xca;
    }
}

void Mapcontrol::Mapcontrol_dec_player_count(Mapcontrol *map_control, int map_id)
{
    if (map_id > 0 && map_id <= Mapcontrol_GetCount(map_control))
    {
        if (Mapcontrol_GetByIndex(map_control, map_id - 1)->player_count > 0)
            Mapcontrol_GetByIndex(map_control, map_id - 1)->player_count--;
    }
}

void Mapcontrol::Mapcontrol_SetArenaBlock(Mapcontrol *map_control, int map_id, int block)
{
    if (map_id > 0 && map_id <= Mapcontrol_GetCount(map_control))
        Mapcontrol_GetByIndex(map_control, map_id - 1)->arena_block = block;
}

void Mapcontrol_AddArenaSpawn(
    Mapcontrol *map_control, int map_id, int from_x, int from_y, int to_x, int to_y)
{
    if (map_id > 0 && map_id <= Mapcontrol_GetCount(map_control))
    {
        Mapcontrol_GetByIndex(map_control, map_id - 1)->arena_enabled = 1;
        Mapcontrol_GetByIndex(map_control, map_id - 1)->arena_block = 4;
        Mapcontrol_GetByIndex(map_control, map_id - 1)->arena_ticks = RandRange(0x3c);
        MapWarp value(from_x, from_y, map_id, 0, to_x, to_y);
        Mapcontrol_GetByIndex(map_control, map_id - 1)
            ->arena_spawn_list.insert(
                Mapcontrol_GetByIndex(map_control, map_id - 1)->arena_spawn_list.end(),
                value);
    }
}

void Mapcontrol::Mapcontrol_SetTileBits(
    Mapcontrol *map_control, MapContainer *map, int x, int y, int code)
{
    int tile_offset = x * 2 + map->width * 2 * y;
    if (code == 0)
    {
        map->tile_bits[tile_offset] = 0;
        map->tile_bits[tile_offset + 1] = 0;
    }
    if (code == 1)
    {
        map->tile_bits[tile_offset] = 0;
        map->tile_bits[tile_offset + 1] = 1;
    }
    if (code == 2)
    {
        map->tile_bits[tile_offset] = 1;
        map->tile_bits[tile_offset + 1] = 0;
    }
    if (code == 3)
    {
        map->tile_bits[tile_offset] = 1;
        map->tile_bits[tile_offset + 1] = 1;
    }
}

int Mapcontrol::Mapcontrol_CountNpcsChasingPlayer(Mapcontrol *map_control,
                                                  int map_id,
                                                  int player_id)
{
    int chase_count = 0;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount(map_control))
    {
        for (std::vector<Npc *>::iterator npc_iter =
                 Mapcontrol_GetByIndex(map_control, map_id - 1)->npc_list.begin();
             npc_iter != Mapcontrol_GetByIndex(map_control, map_id - 1)->npc_list.end();
             npc_iter++)
        {
            if ((*npc_iter)->chase_target_id == player_id)
                chase_count++;
        }
    }
    return chase_count;
}

bool Mapcontrol::Mapcontrol_AggroChildNpcs(Mapcontrol *map_control, int map_id)
{
    bool found_child = false;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount(map_control))
    {
        TDateTime now = Now();
        for (std::vector<Npc *>::iterator npc_iter =
                 Mapcontrol_GetByIndex(map_control, map_id - 1)->npc_list.begin();
             npc_iter != Mapcontrol_GetByIndex(map_control, map_id - 1)->npc_list.end();
             npc_iter++)
        {
            if ((unsigned short)(*npc_iter)->child > 0)
            {
                (*npc_iter)->aggressive = true;
                found_child = true;
            }
        }
    }
    return found_child;
}

bool Mapcontrol::Mapcontrol_KillChildNpcs(Mapcontrol *map_control, int map_id)
{
    bool found_child = false;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount(map_control))
    {
        Mapcontrol_GetByIndex(map_control, map_id - 1)->boss_alive = false;
        TDateTime now = Now();
        for (std::vector<Npc *>::iterator npc_iter =
                 Mapcontrol_GetByIndex(map_control, map_id - 1)->npc_list.begin();
             npc_iter != Mapcontrol_GetByIndex(map_control, map_id - 1)->npc_list.end();
             npc_iter++)
        {
            if ((unsigned short)(*npc_iter)->child > 0)
            {
                (*npc_iter)->nDeath_ms = DateTimeToTimeStamp(now);
                (*npc_iter)->alive = false;
                (*npc_iter)->aggressive = false;
                found_child = true;
            }
        }
    }
    return found_child;
}

void Mapcontrol::Mapcontrol_AddTileSpec(
    Mapcontrol *map_control, MapContainer *map, int x, int y, int spec)
{
    MapObject value(x, y, spec);
    map->tile_specs.insert(map->tile_specs.end(), value);
}

void Mapcontrol::Mapcontrol_AddWarp(Mapcontrol *map_control,
                                    MapContainer *map,
                                    int x,
                                    int y,
                                    int dest_map,
                                    int level,
                                    int dest_x,
                                    int dest_y)
{
    MapWarp value(x, y, dest_map, level, dest_x, dest_y);
    map->warp_list.insert(map->warp_list.end(), value);
}

void Mapcontrol::Mapcontrol_AddLockKey(Mapcontrol *map_control,
                                       MapContainer *map,
                                       unsigned int x,
                                       unsigned int y,
                                       int key_id)
{
    bool found = false;
    for (std::vector<MapObject>::iterator lock_iter = map->legacy_door_key_list.begin();
         lock_iter != map->legacy_door_key_list.end();
         lock_iter++)
    {
        if ((unsigned short)lock_iter->x == x && (unsigned short)lock_iter->y == y)
        {
            lock_iter->value = key_id;
            found = true;
            break;
        }
    }
    if (!found)
    {
        MapObject value(x, y, key_id);
        map->legacy_door_key_list.insert(map->legacy_door_key_list.end(), value);
    }
}

void Mapcontrol::Mapcontrol_GetOrCreateChest(Mapcontrol *map_control,
                                             MapContainer *map,
                                             unsigned int x,
                                             unsigned int y)
{
    bool found = false;
    for (std::vector<MapChest>::iterator chest_iter = map->chest_list.begin();
         chest_iter != map->chest_list.end();
         chest_iter++)
    {
        if ((unsigned short)chest_iter->x == x && (unsigned short)chest_iter->y == y)
        {
            found = true;
            break;
        }
    }
    if (!found)
    {
        MapChest value(x, y, 0);
        map->chest_list.insert(map->chest_list.end(), value);
    }
}

unsigned char Mapcontrol::Mapcontrol_ToggleDoor(Mapcontrol *map_control,
                                                int map_id,
                                                unsigned int x,
                                                unsigned int y)
{
    unsigned char result = 0;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount(map_control))
    {
        for (std::vector<MapObject>::iterator spec_iter =
                 Mapcontrol_GetByIndex(map_control, map_id - 1)->tile_specs.begin();
             spec_iter !=
             Mapcontrol_GetByIndex(map_control, map_id - 1)->tile_specs.end();
             spec_iter++)
        {
            if ((unsigned short)spec_iter->x == x && (unsigned short)spec_iter->y == y)
            {
                if (((unsigned short)spec_iter->value == MapTileSpec_Chest ||
                     (unsigned short)spec_iter->value == MapTileSpec_Reserved11) &&
                    (unsigned short)spec_iter->ticks != 2)
                {
                    spec_iter->ticks = 2;
                    result = 1;
                }
                if ((unsigned short)spec_iter->value == MapTileSpec_ChairAll)
                {
                    result = 1;
                    Mapcontrol_GetByIndex(map_control, map_id - 1)->has_open_doors = 1;
                    spec_iter->value = MapTileSpec_Chest;
                    spec_iter->ticks = 2;
                }
                if ((unsigned short)spec_iter->value == MapTileSpec_Reserved10)
                {
                    result = 1;
                    Mapcontrol_GetByIndex(map_control, map_id - 1)->has_open_doors = 1;
                    spec_iter->value = MapTileSpec_Reserved11;
                    spec_iter->ticks = 2;
                }
                break;
            }
        }
    }
    return result;
}

int Mapcontrol::Map_GetWarpDoorAt(Mapcontrol *map_control, int map_id, MapCoord coords)
{
    int result = 0;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount(map_control))
    {
        for (std::vector<MapObject>::iterator door_iter =
                 Mapcontrol_GetByIndex(map_control, map_id - 1)
                     ->legacy_door_key_list.begin();
             door_iter !=
             Mapcontrol_GetByIndex(map_control, map_id - 1)->legacy_door_key_list.end();
             door_iter++)
        {
            if ((unsigned short)door_iter->x == coords.x &&
                (unsigned short)door_iter->y == coords.y)
            {
                result = (unsigned short)door_iter->value;
                break;
            }
        }
    }
    return result;
}

int Mapcontrol::Mapcontrol_GetChestSlotCount(Mapcontrol *map_control,
                                             int map_id,
                                             unsigned int x,
                                             unsigned int y)
{
    int result = -1;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount(map_control))
    {
        for (std::vector<MapChest>::iterator chest_iter =
                 Mapcontrol_GetByIndex(map_control, map_id - 1)->chest_list.begin();
             chest_iter !=
             Mapcontrol_GetByIndex(map_control, map_id - 1)->chest_list.end();
             chest_iter++)
        {
            if ((unsigned short)chest_iter->x == x && (unsigned short)chest_iter->y == y)
            {
                result = chest_iter->slots.size();
                break;
            }
        }
    }
    return result;
}

void Mapcontrol::Mapcontrol_AddChestSpawn(Mapcontrol *map_control,
                                          MapContainer *map,
                                          unsigned int x,
                                          unsigned int y,
                                          int key_id,
                                          int slot,
                                          int item_id,
                                          int spawn_time,
                                          int amount)
{
    bool found = false;
    std::vector<MapChest>::iterator chest_iter = map->chest_list.begin();
    while (chest_iter != map->chest_list.end())
    {
        if ((unsigned short)chest_iter->x == x && (unsigned short)chest_iter->y == y)
        {
            found = true;
            chest_iter->key_id = key_id;
            if (slot >= (int)chest_iter->slots.size())
            {
                MapItem extra_slot(item_id);
                extra_slot.item_present = false;
                extra_slot.respawn_enabled = true;
                extra_slot.respawn_countdown = spawn_time;
                extra_slot.respawn_delay = spawn_time;
                extra_slot.amount = amount;
                extra_slot.alt_item_id[0] = item_id;
                extra_slot.alt_amount[0] = amount;
                extra_slot.alt_item_id[1] = 0;
                extra_slot.alt_item_id[2] = 0;
                extra_slot.alt_item_id[3] = 0;
                chest_iter->slots.insert(chest_iter->slots.end(), extra_slot);
            }
            else
            {
                if (Itemchest_GetSlot(&chest_iter->slots, slot)->alt_item_id[1] > 0)
                {
                    if (Itemchest_GetSlot(&chest_iter->slots, slot)->alt_item_id[2] > 0)
                    {
                        Itemchest_GetSlot(&chest_iter->slots, slot)->alt_item_id[3] =
                            item_id;
                        Itemchest_GetSlot(&chest_iter->slots, slot)->alt_amount[3] =
                            amount;
                    }
                    else
                    {
                        Itemchest_GetSlot(&chest_iter->slots, slot)->alt_item_id[2] =
                            item_id;
                        Itemchest_GetSlot(&chest_iter->slots, slot)->alt_amount[2] =
                            amount;
                    }
                }
                else
                {
                    Itemchest_GetSlot(&chest_iter->slots, slot)->alt_item_id[1] = item_id;
                    Itemchest_GetSlot(&chest_iter->slots, slot)->alt_amount[1] = amount;
                }
            }
            break;
        }
        chest_iter++;
    }
    if (!found)
    {
        MapChest new_chest(x, y, key_id);
        MapItem new_item(item_id);
        new_item.item_present = false;
        new_item.respawn_enabled = true;
        new_item.respawn_countdown = spawn_time;
        new_item.respawn_delay = spawn_time;
        new_item.amount = amount;
        new_item.alt_item_id[0] = item_id;
        new_item.alt_amount[0] = amount;
        new_item.alt_item_id[1] = 0;
        new_item.alt_item_id[2] = 0;
        new_item.alt_item_id[3] = 0;
        new_chest.slots.insert(new_chest.slots.end(), new_item);
        map->chest_list.insert(map->chest_list.end(), new_chest);
    }
}

void Mapcontrol::Mapcontrol_AddChestItem(Mapcontrol *map_control,
                                         int map_id,
                                         unsigned int x,
                                         unsigned int y,
                                         int item_id,
                                         int amount)
{
    if (map_id > 0 && map_id <= Mapcontrol_GetCount(map_control))
    {
        for (std::vector<MapChest>::iterator chest_iter =
                 Mapcontrol_GetByIndex(map_control, map_id - 1)->chest_list.begin();
             chest_iter !=
             Mapcontrol_GetByIndex(map_control, map_id - 1)->chest_list.end();
             chest_iter++)
        {
            if ((unsigned short)chest_iter->x == x && (unsigned short)chest_iter->y == y)
            {
                bool found = false;
                for (std::vector<MapItem>::iterator item_iter = chest_iter->slots.begin();
                     item_iter != chest_iter->slots.end() && !found;
                     item_iter++)
                {
                    if (item_iter->item_id == item_id && !found)
                    {
                        if (item_iter->respawn_enabled != false)
                        {
                            if (item_iter->item_present != false)
                            {
                                found = true;
                                item_iter->amount = item_iter->amount + amount;
                            }
                        }
                        else
                        {
                            found = true;
                            item_iter->amount = item_iter->amount + amount;
                        }
                    }
                }
                if (!found)
                {
                    MapItem new_item(item_id);
                    new_item.amount = amount;
                    new_item.item_present = true;
                    new_item.respawn_enabled = false;
                    chest_iter->slots.insert(chest_iter->slots.end(), new_item);
                }
                break;
            }
        }
    }
}

ItemStack Mapcontrol::Mapcontrol_TakeChestItem(
    Mapcontrol *map_control, int map_id, unsigned int x, unsigned int y, int item_id)
{
    ItemStack result;
    result.id = -1;
    result.amount = -1;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount(map_control))
    {
        for (std::vector<MapChest>::iterator chest_iter =
                 Mapcontrol_GetByIndex(map_control, map_id - 1)->chest_list.begin();
             chest_iter !=
             Mapcontrol_GetByIndex(map_control, map_id - 1)->chest_list.end();
             chest_iter++)
        {
            if ((unsigned short)chest_iter->x == x && (unsigned short)chest_iter->y == y)
            {
                bool found = false;
                for (std::vector<MapItem>::iterator item_iter = chest_iter->slots.begin();
                     item_iter != chest_iter->slots.end() && !found;
                     item_iter++)
                {
                    if (item_iter->item_id == item_id && !found)
                    {
                        if (item_iter->item_present != false)
                        {
                            found = true;
                            result.id = item_iter->item_id;
                            result.amount = item_iter->amount;
                            if (item_iter->respawn_enabled != false)
                            {
                                item_iter->item_present = false;
                                item_iter->respawn_countdown = item_iter->respawn_delay;
                                item_iter->amount = 0;
                            }
                            else
                            {
                                chest_iter->slots.erase(item_iter);
                            }
                        }
                    }
                }
                break;
            }
        }
    }
    return result;
}

int Mapcontrol::Mapcontrol_AddGroundItem(Mapcontrol *map_control,
                                         int map_id,
                                         unsigned int item_id,
                                         int x,
                                         int y,
                                         unsigned int amount,
                                         int owner_player_id,
                                         unsigned short protect_ticks)
{
    int item_index = -1;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount(map_control) && x >= 0 && y >= 0 &&
        x < (int)Mapcontrol_GetByIndex(map_control, map_id - 1)->width &&
        y < (int)Mapcontrol_GetByIndex(map_control, map_id - 1)->height)
    {
        ChestItem *item = new ChestItem();
        item->index = Mapcontrol_GetByIndex(map_control, map_id - 1)->next_ground_item_id;
        item->item_id = item_id;
        item->x = x;
        item->y = y;
        item->amount = amount;
        item->drop_time = DateTimeToTimeStamp(Now());
        item->owner_player_id = owner_player_id;
        item->protect_ticks = protect_ticks;
        if (Mapcontrol_GetByIndex(map_control, map_id - 1)->next_ground_item_id == 15000)
            Mapcontrol_PurgeGroundItemsInRange(map_control, map_id, 15000, 30000);
        if (Mapcontrol_GetByIndex(map_control, map_id - 1)->next_ground_item_id == 30000)
            Mapcontrol_PurgeGroundItemsInRange(map_control, map_id, 30000, 45000);
        if (Mapcontrol_GetByIndex(map_control, map_id - 1)->next_ground_item_id == 45000)
            Mapcontrol_PurgeGroundItemsInRange(map_control, map_id, 45000, 60000);
        if (Mapcontrol_GetByIndex(map_control, map_id - 1)->next_ground_item_id == 60000)
            Mapcontrol_PurgeGroundItemsInRange(map_control, map_id, 0, 15000);
        if (Mapcontrol_GetByIndex(map_control, map_id - 1)->next_ground_item_id >= 60000)
            Mapcontrol_GetByIndex(map_control, map_id - 1)->next_ground_item_id = 0;
        ((GroundItemPtrVector *)&Mapcontrol_GetByIndex(map_control, map_id - 1)
             ->ground_items)
            ->insert(
                ((GroundItemPtrVector *)&Mapcontrol_GetByIndex(map_control, map_id - 1)
                     ->ground_items)
                    ->end(),
                item);
        item_index = Mapcontrol_GetByIndex(map_control, map_id - 1)->next_ground_item_id;
        Mapcontrol_GetByIndex(map_control, map_id - 1)->next_ground_item_id =
            Mapcontrol_GetByIndex(map_control, map_id - 1)->next_ground_item_id + 1;
    }
    return item_index;
}

void Mapcontrol::Mapcontrol_PurgeGroundItemsInRange(Mapcontrol *map_control,
                                                    int map_id,
                                                    int range_low,
                                                    int range_high)
{
    GroundItemPtrVector::iterator cursor =
        ((GroundItemPtrVector *)&Mapcontrol_GetByIndex(map_control, map_id - 1)
             ->ground_items)
            ->begin();
    while (cursor !=
           ((GroundItemPtrVector *)&Mapcontrol_GetByIndex(map_control, map_id - 1)
                ->ground_items)
               ->end())
    {
        if ((*cursor)->index < range_low)
        {
            cursor++;
        }
        else if ((*cursor)->index > range_high)
        {
            cursor++;
        }
        else
        {
            ChestItem *item = *cursor;
            cursor =
                ((GroundItemPtrVector *)&Mapcontrol_GetByIndex(map_control, map_id - 1)
                     ->ground_items)
                    ->erase(cursor);
            delete item;
        }
    }
}

MapItem *Mapcontrol::Itemchest_GetSlot(std::vector<MapItem> *slot_list, int slot)
{
    return slot_list->begin() + slot;
}

int Mapcontrol::Pub_DecodeNumber_Map(Mapcontrol *map_control, String value)
{
    int result = 0;
    try
    {
        for (int digit_index = 1; digit_index <= value.Length(); digit_index++)
        {
            char c = value[digit_index];
            unsigned char ch = c;
            if (ch == 0xFE || ch == 0)
                break;
            int v = ch;
            v = v - 1;
            if (digit_index == 1)
                result = result + v;
            if (digit_index == 2)
                result = result + v * 0xfd;
            if (digit_index == 3)
                result = result + v * 0xfa09;
            if (digit_index == 4)
                result = result + v * 0xf71ae5;
        }
    }
    catch (...)
    {
        result = 0;
    }
    return result;
}

void Mapcontrol::Mapcontrol_LoadMaps(Mapcontrol *map_control)
{
    for (int map_id = 1; map_id <= 0xfa00 && map_id <= map_control->max_maps; map_id++)
    {
        if (!Mapcontrol_LoadMap(map_control, map_id))
        {
            MapContainer value(map_id, 0, 0);
            map_control->maps.insert(map_control->maps.end(), value);
        }
    }
}

String Mapcontrol::Mapcontrol_AppendEncoded(Mapcontrol *map_control,
                                            unsigned int value,
                                            int width)
{
    int rem;
    char c;
    try
    {
        unsigned int quotient = 1;
        bool flag = true;
        for (int i = 0; i < width; i++)
        {
            if (flag)
            {
                double d = value / 253.0;
                quotient = d;
                rem = value % 0xfd;
                c = rem + 1;
                ((char *)map_control->encode_scratch)[i] = c;
                value = quotient;
                if (quotient < 1)
                    flag = false;
                else if (i + 1 == width)
                    width++;
            }
            else
            {
                char pad = 0xfe;
                ((char *)map_control->encode_scratch)[i] = pad;
            }
        }
    }
    catch (...)
    {
        width = 0;
    }
    String encoded_str((char *)map_control->encode_scratch, width);
    return encoded_str;
}

char FUN_0047c3a4(int map_control, int map_id)
{
    if (map_id > 0 && map_id <= Mapcontrol_GetCount((Mapcontrol *)map_control))
    {
        if (Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)->quest_cooldown <
            1)
        {
            Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)->quest_cooldown =
                10;
            return 1;
        }
    }
    return 0;
}

char FUN_0047c3f0(int map_control, int map_id)
{
    char result = 0;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount((Mapcontrol *)map_control))
        result = Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)->can_scroll;
    return result;
}

MapCoord FUN_0047c428(int map_control, int map_id)
{
    MapCoord coords;
    coords.x = 0;
    coords.y = 0;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount((Mapcontrol *)map_control))
    {
        coords.x = Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)->relog_x;
        coords.y = Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)->relog_y;
        if (coords.x >= (int)Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)
                            ->width ||
            coords.y >=
                (int)Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)->height)
        {
            coords.x = 0;
            coords.y = 0;
        }
    }
    return coords;
}

unsigned int FUN_0047c634(int map_control, int map_id, unsigned int npc_index)
{
    unsigned int result = 0xffffffff;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount((Mapcontrol *)map_control))
    {
        for (std::vector<Npc *>::iterator npc_iter =
                 Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)
                     ->npc_list.begin();
             npc_iter !=
             Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)->npc_list.end();
             npc_iter++)
        {
            if ((*npc_iter)->index == npc_index)
            {
                result = (*npc_iter)->id;
                break;
            }
        }
    }
    return result;
}

MapCoord FUN_0047c6c0(int map_control, int map_id, unsigned int npc_index)
{
    MapCoord coords;
    coords.x = -1;
    coords.y = -1;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount((Mapcontrol *)map_control))
    {
        for (std::vector<Npc *>::iterator npc_iter =
                 Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)
                     ->npc_list.begin();
             npc_iter !=
             Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)->npc_list.end();
             npc_iter++)
        {
            if ((*npc_iter)->index == npc_index)
            {
                coords.x = (*npc_iter)->x;
                coords.y = (*npc_iter)->y;
                break;
            }
        }
    }
    return coords;
}

char FUN_0047c890(int map_control, int map_id, int x, int y)
{
    char result = 1;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount((Mapcontrol *)map_control))
    {
        int tile_offset =
            Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)->width * 2 * y +
            x * 2;
        if (x >= 0 && y >= 0 &&
            x < (int)Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)
                    ->width &&
            y < (int)Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)->height)
        {
            if (Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)
                    ->tile_bits[tile_offset])
            {
                if (!Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)
                         ->tile_bits[tile_offset + 1])
                {
                    result = 0;
                    for (std::vector<MapObject>::iterator spec_iter =
                             Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)
                                 ->tile_specs.begin();
                         spec_iter !=
                         Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)
                             ->tile_specs.end();
                         spec_iter++)
                    {
                        if ((unsigned short)spec_iter->x == x &&
                            (unsigned short)spec_iter->y == y)
                        {
                            if ((unsigned short)spec_iter->value != 0x10)
                                break;
                            result = 1;
                            break;
                        }
                    }
                }
            }
            else
            {
                if (Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)
                        ->tile_bits[tile_offset + 1])
                    result = 0;
            }
        }
        else
        {
            result = 0;
        }
    }
    return result;
}

unsigned int FUN_0047c27c(int map_control, int map_id, unsigned int x, unsigned int y)
{
    unsigned int result = 0xffffffff;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount((Mapcontrol *)map_control))
    {
        int tile_offset =
            Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)->width * 2 * y +
            x * 2;
        if (Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)
                ->tile_bits[tile_offset])
        {
            if (!Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)
                     ->tile_bits[tile_offset + 1])
            {
                for (std::vector<MapObject>::iterator spec_iter =
                         Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)
                             ->tile_specs.begin();
                     spec_iter !=
                     Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)
                         ->tile_specs.end();
                     spec_iter++)
                {
                    if ((unsigned short)spec_iter->x == x &&
                        (unsigned short)spec_iter->y == y)
                    {
                        result = (unsigned short)spec_iter->value;
                        break;
                    }
                }
            }
        }
    }
    return result;
}

int FUN_00486e64(int map_control, int map_id, unsigned int x, unsigned int y)
{
    int result = 0;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount((Mapcontrol *)map_control))
    {
        for (std::vector<MapChest>::iterator chest_iter =
                 Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)
                     ->chest_list.begin();
             chest_iter != Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)
                               ->chest_list.end();
             chest_iter++)
        {
            if ((unsigned short)chest_iter->x == x && (unsigned short)chest_iter->y == y)
            {
                result = (unsigned short)chest_iter->key_id;
                break;
            }
        }
    }
    return result;
}

int FUN_0047cd28(int map_control, int map_id, unsigned int x, unsigned int y)
{
    int result = 0;
    if (Mapcontrol::Map_IsWalkableNPC((Mapcontrol *)map_control, map_id, x - 1, y, 1) ==
        0)
        result = result + 1;
    if (Mapcontrol::Map_IsWalkableNPC((Mapcontrol *)map_control, map_id, x, y - 1, 1) ==
        0)
        result = result + 1;
    if (Mapcontrol::Map_IsWalkableNPC((Mapcontrol *)map_control, map_id, x + 1, y, 1) ==
        0)
        result = result + 1;
    if (Mapcontrol::Map_IsWalkableNPC((Mapcontrol *)map_control, map_id, x, y + 1, 1) ==
        0)
        result = result + 1;
    return result;
}

MapObject Map_GetTileSpecObject(Mapcontrol *map_control, int map_id, int x, int y)
{
    map_id <= 0 ? (map_id == 1) : 0;
    std::vector<MapObject>::iterator spec_iter =
        Mapcontrol_GetByIndex(map_control, map_id - 1)->tile_specs.begin();
    while (spec_iter != Mapcontrol_GetByIndex(map_control, map_id - 1)->tile_specs.end())
    {
        if ((unsigned short)spec_iter->x == x && (unsigned short)spec_iter->y == y)
            break;
        spec_iter++;
    }
    return *spec_iter;
}

bool FUN_004879b0(int map_control, int map_id, int x, int y, int player_id)
{
    if (map_id > 0 && map_id <= Mapcontrol_GetCount((Mapcontrol *)map_control))
    {
        int count = 0;
        for (GroundItemPtrVector::iterator cursor =
                 ((GroundItemPtrVector *)&Mapcontrol_GetByIndex((Mapcontrol *)map_control,
                                                                map_id - 1)
                      ->ground_items)
                     ->begin();
             cursor != ((GroundItemPtrVector *)&Mapcontrol_GetByIndex(
                            (Mapcontrol *)map_control, map_id - 1)
                            ->ground_items)
                           ->end();
             cursor++)
        {
            if ((*cursor)->x == x && (*cursor)->y == y)
            {
                if ((*cursor)->owner_player_id == player_id)
                {
                    count = count + 1;
                    if (count > 9)
                        return 0;
                }
                else
                {
                    TTimeStamp now = DateTimeToTimeStamp(Now());
                    int elapsed = now.Date - (*cursor)->drop_time.Date;
                    int ms = now.Time - (*cursor)->drop_time.Time;
                    elapsed = ms / 1000 + elapsed * 0x15180;
                    if ((*cursor)->protect_ticks > elapsed)
                        return 0;
                }
            }
        }
    }
    return 1;
}

GroundItemInfo FUN_00487ac0(int map_control, int map_id, int index, int player_id)
{
    GroundItemInfo result;
    result.x = -1;
    result.y = -1;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount((Mapcontrol *)map_control))
    {
        for (GroundItemPtrVector::iterator cursor =
                 ((GroundItemPtrVector *)&Mapcontrol_GetByIndex((Mapcontrol *)map_control,
                                                                map_id - 1)
                      ->ground_items)
                     ->begin();
             cursor != ((GroundItemPtrVector *)&Mapcontrol_GetByIndex(
                            (Mapcontrol *)map_control, map_id - 1)
                            ->ground_items)
                           ->end();
             cursor++)
        {
            if ((*cursor)->index == index)
            {
                TTimeStamp now = DateTimeToTimeStamp(Now());
                int elapsed = now.Date - (*cursor)->drop_time.Date;
                int ms = now.Time - (*cursor)->drop_time.Time;
                elapsed = ms / 1000 + elapsed * 0x15180;
                if ((*cursor)->protect_ticks > elapsed &&
                    (*cursor)->owner_player_id != player_id &&
                    (unsigned int)(*cursor)->owner_player_id > 0)
                {
                    result.x = -2;
                }
                else
                {
                    result.x = (*cursor)->x;
                    result.y = (*cursor)->y;
                    result.item_id = (*cursor)->item_id;
                    result.amount = (*cursor)->amount;
                }
                break;
            }
        }
    }
    return result;
}

String FUN_0047badc(Mapcontrol *map_control, int map_id, unsigned int x, unsigned int y)
{
    String result = "x";
    std::vector<MapChest>::iterator chest_iter;
    std::vector<MapItem>::iterator item_iter;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount(map_control))
    {
        for (chest_iter =
                 Mapcontrol_GetByIndex(map_control, map_id - 1)->chest_list.begin();
             chest_iter !=
             Mapcontrol_GetByIndex(map_control, map_id - 1)->chest_list.end();
             chest_iter++)
        {
            if ((unsigned short)chest_iter->x == x && (unsigned short)chest_iter->y == y)
            {
                result = "";
                for (item_iter = chest_iter->slots.begin();
                     item_iter != chest_iter->slots.end();
                     item_iter++)
                {
                    if (item_iter->item_present != false)
                    {
                        result.Insert(Mapcontrol::Mapcontrol_AppendEncoded(
                                          map_control, item_iter->item_id, 2),
                                      result.Length() + 1);
                        result.Insert(Mapcontrol::Mapcontrol_AppendEncoded(
                                          map_control, item_iter->amount, 3),
                                      result.Length() + 1);
                    }
                }
                break;
            }
        }
    }
    return result;
}

void FUN_00481e0c(int map_control, int map_id)
{
    for (int i = 0; i < (int)Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)
                            ->chest_list.size();
         i++)
    {
        Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)
            ->chest_list[i]
            .slots.clear();
    }
    for (int i = 0; i < (int)Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)
                            ->tile_bits.size();
         i++)
    {
        Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)->tile_bits[i] =
            false;
    }
    Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)->tile_specs.clear();
    Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)->chest_list.clear();
    Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)->warp_list.clear();
    ((GroundItemPtrVector *)&Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)
         ->ground_items)
        ->clear();
    Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)->npc_list.clear();
    Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)
        ->legacy_door_key_list.clear();
    Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)->boss_alive = false;
    Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)->width = 0;
    Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)->height = 0;
    Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1)
        ->buf.Delete(
            0, Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id)->buf.Length());
}

void FUN_004876c0(int map_control, int map_id, int index)
{
    if (map_id > 0 && map_id <= Mapcontrol_GetCount((Mapcontrol *)map_control))
    {
        for (GroundItemPtrVector::iterator cursor =
                 ((GroundItemPtrVector *)&Mapcontrol_GetByIndex((Mapcontrol *)map_control,
                                                                map_id - 1)
                      ->ground_items)
                     ->begin();
             cursor != ((GroundItemPtrVector *)&Mapcontrol_GetByIndex(
                            (Mapcontrol *)map_control, map_id - 1)
                            ->ground_items)
                           ->end();
             cursor++)
        {
            if ((*cursor)->index == index)
            {
                ChestItem *item = *cursor;
                ((GroundItemPtrVector *)&Mapcontrol_GetByIndex((Mapcontrol *)map_control,
                                                               map_id - 1)
                     ->ground_items)
                    ->erase(cursor);
                delete item;
                break;
            }
        }
    }
}

char FUN_004827c8(int map_control, int map_id)
{
    char result = 0;
    if (map_id > 0 && map_id <= Mapcontrol_GetCount((Mapcontrol *)map_control))
    {
        FUN_00481e0c(map_control, map_id);
        FUN_004aa4e4((*MAINFORM)->jukebox_control, map_id);
        result = (char)FUN_00482834(
            (Mapcontrol *)map_control,
            Mapcontrol_GetByIndex((Mapcontrol *)map_control, map_id - 1),
            map_id);
    }
    return result;
}

String Map_ReadRawFile(Mapcontrol *map_control, int map_id)
{
    String result;
    if (Mapcontrol_GetByIndex(map_control, map_id - 1)->buf == "")
    {
        String file_name;
        int file_handle;
        int size;
        char *buf;
        try
        {
            file_name = IntToStr(map_id);
            for (int i = file_name.Length(); i <= 4; i++)
                file_name.Insert("0", 0);
            file_name.Insert("./maps/", 0);
            file_name.Insert(".emf", file_name.Length() + 1);
            file_handle = FileOpen(file_name.c_str(), 0);
            if (file_handle < 0)
                return "";
            size = FileSeek(file_handle, 0, 2);
            FileSeek(file_handle, 0, 0);
            buf = new char[size + 1];
            FileRead(file_handle, buf, size);
            FileClose(file_handle);
            result = buf;
            result.SetLength(size);
            delete[] buf;
        }
        catch (...)
        {
            FileClose(file_handle);
        }
    }
    else
    {
        result = Mapcontrol_GetByIndex(map_control, map_id - 1)->buf;
    }
    return result;
}

// BEGIN GENERATED STUBS (scripts/genstubs.py)
#pragma warn - 8057
// STUB(0x0047af68, 80 bytes) FUN_0047af68 - ref: undefined FUN_0047af68(int param_1, byte
// param_2)
void FUN_0047af68_Stub(int a0, unsigned char a1)
{
}
// STUB(0x0047e30c, 248 bytes) FUN_0047e30c - ref: undefined FUN_0047e30c(undefined4
// param_1, undefined4 * param_2)
void FUN_0047e30c_Stub(int a0, void *a1)
{
}
// STUB(0x0047f270, 775 bytes) FUN_0047f270 - ref: undefined FUN_0047f270(undefined4
// param_1, undefined2 * param_2)
void FUN_0047f270_Stub(int a0, void *a1)
{
}
// STUB(0x0047f578, 729 bytes) FUN_0047f578 - ref: undefined2 * FUN_0047f578(undefined2 *
// param_1, undefined2 * param_2, undefined2 * param_3)
void *FUN_0047f578_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x00480240, 560 bytes) FUN_00480240 - ref: int FUN_00480240(int param_1, int
// param_2)
int FUN_00480240_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00481254, 42 bytes) FUN_00481254 - ref: int FUN_00481254(int param_1)
int FUN_00481254_Stub(int a0)
{
    return 0;
}
// STUB(0x00482444, 42 bytes) FUN_00482444 - ref: undefined4 * FUN_00482444(undefined4 *
// param_1, undefined4 * param_2, undefined4 * param_3)
void *FUN_00482444_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
int FUN_00482834(Mapcontrol *map_control, MapContainer *map, int map_id)
{
    String map_buf;
    String local_c;
    int file_handle;
    int size;
    char *buf;
    try
    {
        map_buf = IntToStr(map_id);
        for (int i = map_buf.Length(); i <= 4; i++)
            map_buf.Insert("0", 0);
        int count;
        int tile_x;
        int tile_y;
        int spec;
        int code;
        int lock_key;
        map_buf.Insert("./maps/", 0);
        map_buf.Insert(".emf", map_buf.Length() + 1);
        file_handle = FileOpen(map_buf.c_str(), 0);
        if (file_handle < 0)
            return 0;
        size = FileSeek(file_handle, 0, 2);
        FileSeek(file_handle, 0, 0);
        buf = new char[size + 1];
        FileRead(file_handle, buf, size);
        FileClose(file_handle);
        map_buf = buf;
        map_buf.SetLength(size);
        delete[] buf;
        if (map_buf[1] != 'E' || map_buf[2] != 'M' || map_buf[3] != 'F')
            return 0;
        map->rid = map_id;
        map->width =
            Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 0x26)) + 1;
        map->height =
            Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 0x27)) + 1;
        map->rid1 =
            Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(2, 4));
        map->rid2 =
            Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(2, 6));
        map->map_type =
            Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 0x20));
        map->timed_effect =
            Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 0x21));
        map->relog_x =
            Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 0x2c));
        map->relog_y =
            Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 0x2d));
        map->has_hp_drain = false;
        map->has_tp_drain = false;
        map->has_quakes = false;
        map->has_spikes = false;
        if (map->timed_effect == 1)
            map->has_hp_drain = true;
        if (map->timed_effect == 2)
            map->has_tp_drain = true;
        if (map->timed_effect > 2 && map->timed_effect < 7)
            map->has_quakes = true;
        if (FUN_004813c8(&map->tile_bits) != map->width * map->height * 2)
            FUN_0048441c(&map->tile_bits, map->width * map->height * 2, 0);
        if (Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 0x2b)) ==
            0)
            map->can_scroll = 1;
        map->filesize = size;
        if (map_control->start_map == map_id || map_control->memory_map == 0)
        {
            map->buf_copied = true;
            map->buf = map_buf;
        }
        else
            map->buf_copied = false;
        map_buf.Delete(1, 0x2e);
        count = Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 1));
        map_buf.Delete(1, 1);
        for (int i = 0; i < count; i++)
        {
            int npc_count =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 8));
            for (int j = 0; j < npc_count; j++)
            {
                int index = map->npc_list.size() + 1;
                Npc *npc =
                    new Npc(index,
                            Mapcontrol::Pub_DecodeNumber_Map(map_control,
                                                             map_buf.SubString(2, 3)),
                            Mapcontrol::Pub_DecodeNumber_Map(map_control,
                                                             map_buf.SubString(1, 1)),
                            Mapcontrol::Pub_DecodeNumber_Map(map_control,
                                                             map_buf.SubString(1, 2)),
                            0,
                            Mapcontrol::Pub_DecodeNumber_Map(map_control,
                                                             map_buf.SubString(1, 5)),
                            Mapcontrol::Pub_DecodeNumber_Map(map_control,
                                                             map_buf.SubString(2, 6)));
                NpcValue npc_value =
                    NpcValues::GetNpc((*MAINFORM)->npc_values,
                                      Mapcontrol::Pub_DecodeNumber_Map(
                                          map_control, map_buf.SubString(2, 3)));
                if (npc_value.npc_type == 2)
                {
                    npc->aggressive = true;
                    npc->in_combat = true;
                }
                if (npc_value.child > 0)
                    map->child_npc_id = npc->id;
                npc->boss = npc_value.boss;
                npc->child = npc_value.child;
                npc->min_damage = npc_value.min_damage;
                npc->max_damage = npc_value.max_damage;
                npc->accuracy = npc_value.accuracy;
                npc->evade = npc_value.evade;
                npc->armor = npc_value.armor;
                npc->element_weakness = npc_value.element_weakness;
                for (int k = 0; k < 7; k++)
                    ((short *)&npc->pad_34)[k] = 0;
                if (npc_value.element_weakness > 0 && npc_value.element_weakness < 7)
                    ((short *)&npc->pad_34)[npc_value.element_weakness] =
                        npc_value.element_weakness_damage;
                map->npc_list.insert((Npc **)Map_NpcIter_End(&map->npc_list), npc);
            }
            map_buf.Delete(1, 8);
        }
        count = Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 1));
        map_buf.Delete(1, 1);
        for (int i = 0; i < count; i++)
        {
            unsigned int key_x =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 1));
            unsigned int key_y =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 2));
            int key_id =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(2, 3));
            Mapcontrol::Mapcontrol_AddLockKey(map_control, map, key_x, key_y, key_id);
            map_buf.Delete(1, 4);
        }
        count = Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 1));
        map_buf.Delete(1, 1);
        for (int i = 0; i < count; i++)
        {
            unsigned int chest_x =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 1));
            unsigned int chest_y =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 2));
            int chest_key =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(2, 3));
            int chest_slot =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 5));
            int chest_item =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(2, 6));
            int chest_time =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(2, 8));
            int chest_amount =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(3, 0xa));
            Mapcontrol::Mapcontrol_AddChestSpawn(map_control,
                                                 map,
                                                 chest_x,
                                                 chest_y,
                                                 chest_key,
                                                 chest_slot,
                                                 chest_item,
                                                 chest_time,
                                                 chest_amount);
            map_buf.Delete(1, 12);
        }
        count = Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 1));
        map_buf.Delete(1, 1);
        for (int i = 0; i < count; i++)
        {
            tile_x =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 1));
            tile_y =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 2));
            map_buf.Delete(1, 2);
            for (int k = 0; k < tile_y; k++)
            {
                spec = Mapcontrol::Pub_DecodeNumber_Map(map_control,
                                                        map_buf.SubString(1, 1));
                code = Mapcontrol::Pub_DecodeNumber_Map(map_control,
                                                        map_buf.SubString(1, 2));
                if (code == 0 || code == 0x12)
                    Mapcontrol::Mapcontrol_SetTileBits(map_control, map, spec, tile_x, 1);
                if (code > 0 && code <= 0x11)
                {
                    Mapcontrol::Mapcontrol_SetTileBits(map_control, map, spec, tile_x, 2);
                    Mapcontrol::Mapcontrol_AddTileSpec(
                        map_control, map, spec, tile_x, code - 1);
                }
                if (code == 0x13 || code == 0x1d)
                {
                    Mapcontrol::Mapcontrol_SetTileBits(map_control, map, spec, tile_x, 2);
                    Mapcontrol::Mapcontrol_AddTileSpec(
                        map_control, map, spec, tile_x, 0x10);
                }
                if (code > 0x13 && code <= 0x1b)
                {
                    Mapcontrol::Mapcontrol_SetTileBits(map_control, map, spec, tile_x, 2);
                    Mapcontrol::Mapcontrol_AddTileSpec(
                        map_control, map, spec, tile_x, code - 1);
                }
                if (code == 0x1c)
                {
                    Mapcontrol::Mapcontrol_SetTileBits(map_control, map, spec, tile_x, 2);
                    Mapcontrol::Mapcontrol_AddTileSpec(
                        map_control, map, spec, tile_x, code - 1);
                    FUN_004a9e74((*MAINFORM)->jukebox_control, map_id);
                }
                if (code == 9)
                    Mapcontrol::Mapcontrol_GetOrCreateChest(
                        map_control, map, spec, tile_x);
                if (code == 0x20)
                    Mapcontrol::Mapcontrol_AddTileSpec(
                        map_control, map, spec, tile_x, code - 1);
                if (code > 0x21 && code < 0x25)
                {
                    Mapcontrol::Mapcontrol_AddTileSpec(
                        map_control, map, spec, tile_x, code - 1);
                    map->has_spikes = 1;
                }
                map_buf.Delete(1, 2);
            }
        }
        count = Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 1));
        map_buf.Delete(1, 1);
        for (int i = 0; i < count; i++)
        {
            tile_x =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 1));
            tile_y =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 2));
            map_buf.Delete(1, 2);
            for (int k = 0; k < tile_y; k++)
            {
                spec = Mapcontrol::Pub_DecodeNumber_Map(map_control,
                                                        map_buf.SubString(1, 1));
                lock_key = Mapcontrol::Pub_DecodeNumber_Map(map_control,
                                                            map_buf.SubString(2, 7));
                Mapcontrol::Mapcontrol_SetTileBits(map_control, map, spec, tile_x, 3);
                Mapcontrol::Mapcontrol_AddWarp(map_control,
                                               map,
                                               spec,
                                               tile_x,
                                               Mapcontrol::Pub_DecodeNumber_Map(
                                                   map_control, map_buf.SubString(2, 2)),
                                               Mapcontrol::Pub_DecodeNumber_Map(
                                                   map_control, map_buf.SubString(1, 6)),
                                               Mapcontrol::Pub_DecodeNumber_Map(
                                                   map_control, map_buf.SubString(1, 4)),
                                               Mapcontrol::Pub_DecodeNumber_Map(
                                                   map_control, map_buf.SubString(1, 5)));
                if (lock_key > 0)
                {
                    Mapcontrol::Mapcontrol_AddTileSpec(
                        map_control, map, spec, tile_x, 0xa);
                    Mapcontrol::Mapcontrol_SetTileBits(map_control, map, spec, tile_x, 2);
                    if (lock_key > 1)
                        Mapcontrol::Mapcontrol_AddLockKey(
                            map_control, map, spec, tile_x, lock_key);
                }
                map_buf.Delete(1, 8);
            }
        }
        return 1;
    }
    catch (...)
    {
        return 1;
    }
}
// STUB(0x0048441c, 161 bytes) FUN_0048441c - ref: undefined FUN_0048441c(undefined4 *
// param_1, uint param_2)
void FUN_0048441c_Stub(void *a0, unsigned int a1)
{
}
// STUB(0x00484558, 59 bytes) FUN_00484558 - ref: undefined4 * FUN_00484558(undefined4 *
// param_1, int param_2)
void *FUN_00484558_Stub(void *a0, int a1)
{
    return 0;
}
// STUB(0x00484594, 670 bytes) FUN_00484594 - ref: undefined FUN_00484594(undefined4 *
// param_1, undefined4 param_2, undefined4 param_3, undefined4 param_4, undefined4
// param_5, uint param_6, undefined4 param_7)
void FUN_00484594_Stub(void *a0, int a1, int a2, int a3, int a4, unsigned int a5, int a6)
{
}
// STUB(0x00484834, 159 bytes) FUN_00484834 - ref: undefined4 * FUN_00484834(undefined4 *
// param_1, int param_2, undefined4 param_3, undefined4 param_4, undefined4 param_5,
// undefined4 param_6)
void *FUN_00484834_Stub(void *a0, int a1, int a2, int a3, int a4, int a5)
{
    return 0;
}
// STUB(0x00484b18, 129 bytes) FUN_00484b18 - ref: undefined4 * FUN_00484b18(undefined4 *
// param_1)
void *FUN_00484b18_Stub(void *a0)
{
    return 0;
}
// STUB(0x00484b9c, 84 bytes) FUN_00484b9c - ref: undefined FUN_00484b9c(void)
void FUN_00484b9c_Stub()
{
}
// STUB(0x00484bf0, 151 bytes) FUN_00484bf0 - ref: undefined4 * FUN_00484bf0(undefined4 *
// param_1)
void *FUN_00484bf0_Stub(void *a0)
{
    return 0;
}
// STUB(0x00484c88, 75 bytes) FUN_00484c88 - ref: undefined FUN_00484c88(void)
void FUN_00484c88_Stub()
{
}
// STUB(0x00484d00, 99 bytes) FUN_00484d00 - ref: int FUN_00484d00(undefined4 * param_1,
// undefined4 * param_2, int param_3)
int FUN_00484d00_Stub(void *a0, void *a1, int a2)
{
    return 0;
}
// STUB(0x00484d88, 31 bytes) FUN_00484d88 - ref: bool FUN_00484d88(int param_1, int
// param_2)
bool FUN_00484d88_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00484da8, 17 bytes) FUN_00484da8 - ref: int FUN_00484da8(int param_1)
int FUN_00484da8_Stub(int a0)
{
    return 0;
}
// STUB(0x00484dbc, 26 bytes) FUN_00484dbc - ref: undefined FUN_00484dbc(undefined4 *
// param_1, undefined4 * param_2)
void FUN_00484dbc_Stub(void *a0, void *a1)
{
}
// STUB(0x00484dd8, 42 bytes) FUN_00484dd8 - ref: undefined4 FUN_00484dd8(int param_1, int
// param_2)
int FUN_00484dd8_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00484e04, 36 bytes) FUN_00484e04 - ref: undefined FUN_00484e04(int param_1)
void FUN_00484e04_Stub(int a0)
{
}
bool Mapcontrol::Mapcontrol_LoadMap(Mapcontrol *map_control, int map_id)
{
    String map_buf;
    String local_c;
    int file_handle;
    int size;
    char *buf;
    try
    {
        map_buf = IntToStr(map_id);
        for (int i = map_buf.Length(); i <= 4; i++)
            map_buf.Insert("0", 0);
        int count;
        int tile_x;
        int tile_y;
        int spec;
        int code;
        int lock_key;
        map_buf.Insert("./maps/", 0);
        map_buf.Insert(".emf", map_buf.Length() + 1);
        file_handle = FileOpen(map_buf.c_str(), 0);
        if (file_handle < 0)
            return false;
        size = FileSeek(file_handle, 0, 2);
        FileSeek(file_handle, 0, 0);
        buf = new char[size + 1];
        FileRead(file_handle, buf, size);
        FileClose(file_handle);
        map_buf = buf;
        map_buf.SetLength(size);
        delete[] buf;
        if (map_buf[1] != 'E' || map_buf[2] != 'M' || map_buf[3] != 'F')
            return false;
        MapContainer map = Map_InitBlank(
            map_id,
            Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 0x26)) + 1,
            Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 0x27)) +
                1);
        map.rid1 = Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(2, 4));
        map.rid2 = Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(2, 6));
        map.map_type =
            Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 0x20));
        map.timed_effect =
            Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 0x21));
        map.relog_x =
            Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 0x2c));
        map.relog_y =
            Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 0x2d));
        map.has_hp_drain = false;
        map.has_tp_drain = false;
        map.has_quakes = false;
        map.has_spikes = false;
        if (map.timed_effect == 1)
            map.has_hp_drain = true;
        if (map.timed_effect == 2)
            map.has_tp_drain = true;
        if (map.timed_effect > 2 && map.timed_effect < 7)
            map.has_quakes = true;
        if (Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 0x2b)) ==
            0)
            map.can_scroll = 1;
        map.filesize = size;
        if (map_control->start_map == map_id || map_control->memory_map == 0)
        {
            map.buf_copied = true;
            map.buf = map_buf;
        }
        else
            map.buf_copied = false;
        map_buf.Delete(1, 0x2e);
        count = Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 1));
        map_buf.Delete(1, 1);
        for (int i = 0; i < count; i++)
        {
            int npc_count =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 8));
            for (int j = 0; j < npc_count; j++)
            {
                int index = map.npc_list.size() + 1;
                Npc *npc =
                    new Npc(index,
                            Mapcontrol::Pub_DecodeNumber_Map(map_control,
                                                             map_buf.SubString(2, 3)),
                            Mapcontrol::Pub_DecodeNumber_Map(map_control,
                                                             map_buf.SubString(1, 1)),
                            Mapcontrol::Pub_DecodeNumber_Map(map_control,
                                                             map_buf.SubString(1, 2)),
                            0,
                            Mapcontrol::Pub_DecodeNumber_Map(map_control,
                                                             map_buf.SubString(1, 5)),
                            Mapcontrol::Pub_DecodeNumber_Map(map_control,
                                                             map_buf.SubString(2, 6)));
                NpcValue npc_value =
                    NpcValues::GetNpc((*MAINFORM)->npc_values,
                                      Mapcontrol::Pub_DecodeNumber_Map(
                                          map_control, map_buf.SubString(2, 3)));
                if (npc_value.npc_type == 2)
                {
                    npc->aggressive = true;
                    npc->in_combat = true;
                }
                if (npc_value.child > 0)
                    map.child_npc_id = npc->id;
                npc->boss = npc_value.boss;
                npc->child = npc_value.child;
                npc->min_damage = npc_value.min_damage;
                npc->max_damage = npc_value.max_damage;
                npc->accuracy = npc_value.accuracy;
                npc->evade = npc_value.evade;
                npc->armor = npc_value.armor;
                npc->element_weakness = npc_value.element_weakness;
                for (int k = 0; k < 7; k++)
                    ((short *)&npc->pad_34)[k] = 0;
                if (npc_value.element_weakness > 0 && npc_value.element_weakness < 7)
                    ((short *)&npc->pad_34)[npc_value.element_weakness] =
                        npc_value.element_weakness_damage;
                map.npc_list.insert((Npc **)Map_NpcIter_End(&map.npc_list), npc);
            }
            map_buf.Delete(1, 8);
        }
        count = Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 1));
        map_buf.Delete(1, 1);
        for (int i = 0; i < count; i++)
        {
            unsigned int key_x =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 1));
            unsigned int key_y =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 2));
            int key_id =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(2, 3));
            Mapcontrol::Mapcontrol_AddLockKey(map_control, &map, key_x, key_y, key_id);
            map_buf.Delete(1, 4);
        }
        count = Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 1));
        map_buf.Delete(1, 1);
        for (int i = 0; i < count; i++)
        {
            unsigned int chest_x =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 1));
            unsigned int chest_y =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 2));
            int chest_key =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(2, 3));
            int chest_slot =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 5));
            int chest_item =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(2, 6));
            int chest_time =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(2, 8));
            int chest_amount =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(3, 0xa));
            Mapcontrol::Mapcontrol_AddChestSpawn(map_control,
                                                 &map,
                                                 chest_x,
                                                 chest_y,
                                                 chest_key,
                                                 chest_slot,
                                                 chest_item,
                                                 chest_time,
                                                 chest_amount);
            map_buf.Delete(1, 12);
        }
        count = Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 1));
        map_buf.Delete(1, 1);
        for (int i = 0; i < count; i++)
        {
            tile_x =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 1));
            tile_y =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 2));
            map_buf.Delete(1, 2);
            for (int k = 0; k < tile_y; k++)
            {
                spec = Mapcontrol::Pub_DecodeNumber_Map(map_control,
                                                        map_buf.SubString(1, 1));
                code = Mapcontrol::Pub_DecodeNumber_Map(map_control,
                                                        map_buf.SubString(1, 2));
                if (code == 0 || code == 0x12)
                    Mapcontrol::Mapcontrol_SetTileBits(
                        map_control, &map, spec, tile_x, 1);
                if (code > 0 && code <= 0x11)
                {
                    Mapcontrol::Mapcontrol_SetTileBits(
                        map_control, &map, spec, tile_x, 2);
                    Mapcontrol::Mapcontrol_AddTileSpec(
                        map_control, &map, spec, tile_x, code - 1);
                }
                if (code == 0x13 || code == 0x1d)
                {
                    Mapcontrol::Mapcontrol_SetTileBits(
                        map_control, &map, spec, tile_x, 2);
                    Mapcontrol::Mapcontrol_AddTileSpec(
                        map_control, &map, spec, tile_x, 0x10);
                }
                if (code > 0x13 && code <= 0x1b)
                {
                    Mapcontrol::Mapcontrol_SetTileBits(
                        map_control, &map, spec, tile_x, 2);
                    Mapcontrol::Mapcontrol_AddTileSpec(
                        map_control, &map, spec, tile_x, code - 1);
                }
                if (code == 0x1c)
                {
                    Mapcontrol::Mapcontrol_SetTileBits(
                        map_control, &map, spec, tile_x, 2);
                    Mapcontrol::Mapcontrol_AddTileSpec(
                        map_control, &map, spec, tile_x, code - 1);
                    FUN_004a9e74((*MAINFORM)->jukebox_control, map_id);
                }
                if (code == 9)
                    Mapcontrol::Mapcontrol_GetOrCreateChest(
                        map_control, &map, spec, tile_x);
                if (code == 0x20)
                    Mapcontrol::Mapcontrol_AddTileSpec(
                        map_control, &map, spec, tile_x, code - 1);
                if (code > 0x21 && code < 0x25)
                {
                    Mapcontrol::Mapcontrol_AddTileSpec(
                        map_control, &map, spec, tile_x, code - 1);
                    map.has_spikes = 1;
                }
                map_buf.Delete(1, 2);
            }
        }
        count = Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 1));
        map_buf.Delete(1, 1);
        for (int i = 0; i < count; i++)
        {
            tile_x =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 1));
            tile_y =
                Mapcontrol::Pub_DecodeNumber_Map(map_control, map_buf.SubString(1, 2));
            map_buf.Delete(1, 2);
            for (int k = 0; k < tile_y; k++)
            {
                spec = Mapcontrol::Pub_DecodeNumber_Map(map_control,
                                                        map_buf.SubString(1, 1));
                lock_key = Mapcontrol::Pub_DecodeNumber_Map(map_control,
                                                            map_buf.SubString(2, 7));
                Mapcontrol::Mapcontrol_SetTileBits(map_control, &map, spec, tile_x, 3);
                Mapcontrol::Mapcontrol_AddWarp(map_control,
                                               &map,
                                               spec,
                                               tile_x,
                                               Mapcontrol::Pub_DecodeNumber_Map(
                                                   map_control, map_buf.SubString(2, 2)),
                                               Mapcontrol::Pub_DecodeNumber_Map(
                                                   map_control, map_buf.SubString(1, 6)),
                                               Mapcontrol::Pub_DecodeNumber_Map(
                                                   map_control, map_buf.SubString(1, 4)),
                                               Mapcontrol::Pub_DecodeNumber_Map(
                                                   map_control, map_buf.SubString(1, 5)));
                if (lock_key > 0)
                {
                    Mapcontrol::Mapcontrol_AddTileSpec(
                        map_control, &map, spec, tile_x, 0xa);
                    Mapcontrol::Mapcontrol_SetTileBits(
                        map_control, &map, spec, tile_x, 2);
                    if (lock_key > 1)
                        Mapcontrol::Mapcontrol_AddLockKey(
                            map_control, &map, spec, tile_x, lock_key);
                }
                map_buf.Delete(1, 8);
            }
        }
        map_control->maps.insert(MapVector_End(map_control), map);
        FUN_0048835c(&map, 2);
        return true;
    }
    catch (...)
    {
        return true;
    }
}
// STUB(0x004873c8, 19 bytes) FUN_004873c8 - ref: undefined FUN_004873c8(undefined4
// param_1, undefined4 param_2, undefined4 * param_3)
void FUN_004873c8_Stub(int a0, int a1, void *a2)
{
}
// STUB(0x0048760c, 42 bytes) FUN_0048760c - ref: undefined4 * FUN_0048760c(undefined4 *
// param_1, undefined4 * param_2, undefined4 * param_3)
void *FUN_0048760c_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x00487638, 99 bytes) FUN_00487638 - ref: int FUN_00487638(undefined4 * param_1,
// undefined4 * param_2, int param_3)
int FUN_00487638_Stub(void *a0, void *a1, int a2)
{
    return 0;
}
#pragma warn.8057
// END GENERATED STUBS
