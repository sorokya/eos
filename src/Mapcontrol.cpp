#include <vcl.h>
#pragma hdrstop

#include "Mapcontrol.h"
#include "Npc.h"
#include "Settings.h"
#include "Protocol.h"

#pragma package(smart_init)

// Cross-unit (Packets) helpers. Their definitions live in the Packets unit; the
// controllers declare the same prototypes.
int Mapcontrol_GetCount(Mapcontrol *map_control);
MapContainer *Mapcontrol_GetByIndex(Mapcontrol *map_control, int index);

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

// BEGIN GENERATED STUBS (scripts/genstubs.py)
#pragma warn - 8057
// STUB(0x00482470, 836 bytes) Map_ReadRawFile - ref: AnsiString *
// Map_ReadRawFile(AnsiString * out_data, Mapcontrol * map_control, int map_id)
void *Map_ReadRawFile_Stub(void *a0, void *a1, int a2)
{
    return 0;
}
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
// STUB(0x0047e8f0, 19 bytes) FUN_0047e8f0 - ref: undefined FUN_0047e8f0(undefined4
// param_1, undefined4 param_2, undefined4 * param_3)
void FUN_0047e8f0_Stub(int a0, int a1, void *a2)
{
}
// STUB(0x0047eb38, 53 bytes) FUN_0047eb38 - ref: undefined4 * FUN_0047eb38(undefined4 *
// param_1, undefined4 * param_2, undefined4 * param_3)
void *FUN_0047eb38_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x0047eb70, 99 bytes) FUN_0047eb70 - ref: int FUN_0047eb70(undefined4 * param_1,
// undefined4 * param_2, int param_3)
int FUN_0047eb70_Stub(void *a0, void *a1, int a2)
{
    return 0;
}
// STUB(0x0047ed7c, 19 bytes) FUN_0047ed7c - ref: undefined FUN_0047ed7c(undefined4
// param_1, undefined4 param_2, undefined2 * param_3)
void FUN_0047ed7c_Stub(int a0, int a1, void *a2)
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
// STUB(0x0047f854, 560 bytes) FUN_0047f854 - ref: int FUN_0047f854(int param_1, int
// param_2)
int FUN_0047f854_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x0047facc, 560 bytes) FUN_0047facc - ref: int FUN_0047facc(int param_1, int
// param_2)
int FUN_0047facc_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x0047fd44, 572 bytes) FUN_0047fd44 - ref: int FUN_0047fd44(int param_1, int
// param_2)
int FUN_0047fd44_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x0047ffc8, 560 bytes) FUN_0047ffc8 - ref: int FUN_0047ffc8(int param_1, int
// param_2)
int FUN_0047ffc8_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00480240, 560 bytes) FUN_00480240 - ref: int FUN_00480240(int param_1, int
// param_2)
int FUN_00480240_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x004804b8, 273 bytes) FUN_004804b8 - ref: undefined4 * FUN_004804b8(undefined4 *
// param_1, undefined4 * param_2)
void *FUN_004804b8_Stub(void *a0, void *a1)
{
    return 0;
}
// STUB(0x004805cc, 112 bytes) FUN_004805cc - ref: int FUN_004805cc(undefined4 param_1,
// int param_2)
int FUN_004805cc_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x0048063c, 105 bytes) FUN_0048063c - ref: int FUN_0048063c(undefined2 * param_1,
// undefined2 * param_2, int param_3)
int FUN_0048063c_Stub(void *a0, void *a1, int a2)
{
    return 0;
}
// STUB(0x004806cc, 33 bytes) FUN_004806cc - ref: int FUN_004806cc(int param_1, int
// param_2, undefined4 * param_3)
int FUN_004806cc_Stub(int a0, int a1, void *a2)
{
    return 0;
}
// STUB(0x00480ca4, 214 bytes) FUN_00480ca4 - ref: undefined4 * FUN_00480ca4(undefined4 *
// param_1, int param_2)
void *FUN_00480ca4_Stub(void *a0, int a1)
{
    return 0;
}
// STUB(0x00480da4, 42 bytes) FUN_00480da4 - ref: int FUN_00480da4(int param_1)
int FUN_00480da4_Stub(int a0)
{
    return 0;
}
// STUB(0x00480dd0, 99 bytes) FUN_00480dd0 - ref: int FUN_00480dd0(undefined2 * param_1,
// undefined2 * param_2, int param_3)
int FUN_00480dd0_Stub(void *a0, void *a1, int a2)
{
    return 0;
}
// STUB(0x00480e58, 53 bytes) FUN_00480e58 - ref: undefined4 * FUN_00480e58(undefined4 *
// param_1, undefined4 * param_2, undefined4 * param_3)
void *FUN_00480e58_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x00480e90, 42 bytes) FUN_00480e90 - ref: int FUN_00480e90(int param_1)
int FUN_00480e90_Stub(int a0)
{
    return 0;
}
// STUB(0x00480ebc, 99 bytes) FUN_00480ebc - ref: int FUN_00480ebc(undefined2 * param_1,
// undefined2 * param_2, int param_3)
int FUN_00480ebc_Stub(void *a0, void *a1, int a2)
{
    return 0;
}
// STUB(0x00480f44, 48 bytes) FUN_00480f44 - ref: undefined4 * FUN_00480f44(undefined4 *
// param_1, undefined4 * param_2, undefined4 * param_3)
void *FUN_00480f44_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x00480f74, 40 bytes) FUN_00480f74 - ref: int FUN_00480f74(int param_1)
int FUN_00480f74_Stub(int a0)
{
    return 0;
}
// STUB(0x00480f9c, 99 bytes) FUN_00480f9c - ref: int FUN_00480f9c(undefined2 * param_1,
// undefined2 * param_2, int param_3)
int FUN_00480f9c_Stub(void *a0, void *a1, int a2)
{
    return 0;
}
// STUB(0x00481024, 123 bytes) FUN_00481024 - ref: undefined2 * FUN_00481024(undefined2 *
// param_1, undefined2 * param_2, undefined2 * param_3)
void *FUN_00481024_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x004810c8, 42 bytes) FUN_004810c8 - ref: int FUN_004810c8(int param_1)
int FUN_004810c8_Stub(int a0)
{
    return 0;
}
// STUB(0x0048110c, 111 bytes) FUN_0048110c - ref: int FUN_0048110c(undefined4 param_1,
// int param_2)
int FUN_0048110c_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x0048117c, 99 bytes) FUN_0048117c - ref: int FUN_0048117c(undefined4 * param_1,
// undefined4 * param_2, int param_3)
int FUN_0048117c_Stub(void *a0, void *a1, int a2)
{
    return 0;
}
// STUB(0x00481204, 33 bytes) FUN_00481204 - ref: int FUN_00481204(int param_1, int
// param_2, undefined4 * param_3)
int FUN_00481204_Stub(int a0, int a1, void *a2)
{
    return 0;
}
// STUB(0x00481228, 42 bytes) FUN_00481228 - ref: undefined4 * FUN_00481228(undefined4 *
// param_1, undefined4 * param_2, undefined4 * param_3)
void *FUN_00481228_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x00481254, 42 bytes) FUN_00481254 - ref: int FUN_00481254(int param_1)
int FUN_00481254_Stub(int a0)
{
    return 0;
}
// STUB(0x00481280, 111 bytes) FUN_00481280 - ref: int FUN_00481280(undefined4 param_1,
// int param_2)
int FUN_00481280_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x004812f0, 99 bytes) FUN_004812f0 - ref: int FUN_004812f0(undefined4 * param_1,
// undefined4 * param_2, int param_3)
int FUN_004812f0_Stub(void *a0, void *a1, int a2)
{
    return 0;
}
// STUB(0x00481378, 33 bytes) FUN_00481378 - ref: int FUN_00481378(int param_1, int
// param_2, undefined4 * param_3)
int FUN_00481378_Stub(int a0, int a1, void *a2)
{
    return 0;
}
// STUB(0x0048139c, 42 bytes) FUN_0048139c - ref: undefined4 * FUN_0048139c(undefined4 *
// param_1, undefined4 * param_2, undefined4 * param_3)
void *FUN_0048139c_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x004813c8, 56 bytes) FUN_004813c8 - ref: undefined FUN_004813c8(int param_1)
void FUN_004813c8_Stub(int a0)
{
}
// STUB(0x00481400, 66 bytes) FUN_00481400 - ref: undefined FUN_00481400(int param_1)
void FUN_00481400_Stub(int a0)
{
}
// STUB(0x00481444, 138 bytes) FUN_00481444 - ref: undefined FUN_00481444(undefined4 *
// param_1, int param_2)
void FUN_00481444_Stub(void *a0, int a1)
{
}
// STUB(0x004814d0, 31 bytes) FUN_004814d0 - ref: int FUN_004814d0(int param_1, int
// param_2)
int FUN_004814d0_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x004814f0, 31 bytes) FUN_004814f0 - ref: int FUN_004814f0(int param_1, int
// param_2)
int FUN_004814f0_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00481510, 142 bytes) FUN_00481510 - ref: undefined4 * FUN_00481510(undefined4 *
// param_1)
void *FUN_00481510_Stub(void *a0)
{
    return 0;
}
// STUB(0x004815a0, 28 bytes) FUN_004815a0 - ref: undefined FUN_004815a0(int param_1, int
// param_2)
void FUN_004815a0_Stub(int a0, int a1)
{
}
// STUB(0x004815bc, 54 bytes) FUN_004815bc - ref: int FUN_004815bc(int param_1, int
// param_2)
int FUN_004815bc_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x004815f4, 26 bytes) FUN_004815f4 - ref: undefined4 FUN_004815f4(undefined4
// param_1)
int FUN_004815f4_Stub(int a0)
{
    return 0;
}
// STUB(0x00481684, 26 bytes) FUN_00481684 - ref: undefined4 FUN_00481684(undefined4
// param_1)
int FUN_00481684_Stub(int a0)
{
    return 0;
}
// STUB(0x00481714, 26 bytes) FUN_00481714 - ref: undefined4 FUN_00481714(undefined4
// param_1)
int FUN_00481714_Stub(int a0)
{
    return 0;
}
// STUB(0x004817a4, 26 bytes) FUN_004817a4 - ref: undefined4 FUN_004817a4(undefined4
// param_1)
int FUN_004817a4_Stub(int a0)
{
    return 0;
}
// STUB(0x00481834, 26 bytes) FUN_00481834 - ref: undefined4 FUN_00481834(undefined4
// param_1)
int FUN_00481834_Stub(int a0)
{
    return 0;
}
// STUB(0x004818c4, 26 bytes) FUN_004818c4 - ref: undefined4 FUN_004818c4(undefined4
// param_1)
int FUN_004818c4_Stub(int a0)
{
    return 0;
}
// STUB(0x004818e0, 55 bytes) FUN_004818e0 - ref: int FUN_004818e0(int param_1, undefined4
// param_2)
int FUN_004818e0_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00481918, 49 bytes) FUN_00481918 - ref: int FUN_00481918(int param_1)
int FUN_00481918_Stub(int a0)
{
    return 0;
}
// STUB(0x0048194c, 33 bytes) FUN_0048194c - ref: undefined FUN_0048194c(undefined4
// param_1, undefined4 * param_2)
void FUN_0048194c_Stub(int a0, void *a1)
{
}
// STUB(0x00481970, 25 bytes) FUN_00481970 - ref: undefined FUN_00481970(int param_1, int
// param_2)
void FUN_00481970_Stub(int a0, int a1)
{
}
// STUB(0x0048198c, 54 bytes) FUN_0048198c - ref: int FUN_0048198c(int param_1, int
// param_2)
int FUN_0048198c_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x004819c4, 33 bytes) FUN_004819c4 - ref: undefined FUN_004819c4(undefined4
// param_1, undefined4 * param_2)
void FUN_004819c4_Stub(int a0, void *a1)
{
}
// STUB(0x004819e8, 25 bytes) FUN_004819e8 - ref: undefined FUN_004819e8(int param_1, int
// param_2)
void FUN_004819e8_Stub(int a0, int a1)
{
}
// STUB(0x00481a04, 54 bytes) FUN_00481a04 - ref: int FUN_00481a04(int param_1, int
// param_2)
int FUN_00481a04_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00481a3c, 36 bytes) FUN_00481a3c - ref: int FUN_00481a3c(int param_1, undefined4
// param_2, undefined4 param_3, int param_4, int param_5)
int FUN_00481a3c_Stub(int a0, int a1, int a2, int a3, int a4)
{
    return 0;
}
// STUB(0x00481a60, 51 bytes) FUN_00481a60 - ref: int FUN_00481a60(int param_1, undefined4
// param_2, undefined4 param_3)
int FUN_00481a60_Stub(int a0, int a1, int a2)
{
    return 0;
}
// STUB(0x00481a94, 55 bytes) FUN_00481a94 - ref: undefined FUN_00481a94(undefined4
// param_1, int param_2)
void FUN_00481a94_Stub(int a0, int a1)
{
}
// STUB(0x00481acc, 33 bytes) FUN_00481acc - ref: int FUN_00481acc(int param_1, int
// param_2, undefined4 * param_3)
int FUN_00481acc_Stub(int a0, int a1, void *a2)
{
    return 0;
}
// STUB(0x00481af0, 51 bytes) FUN_00481af0 - ref: int FUN_00481af0(int param_1, undefined4
// param_2, undefined4 param_3)
int FUN_00481af0_Stub(int a0, int a1, int a2)
{
    return 0;
}
// STUB(0x00481b24, 31 bytes) FUN_00481b24 - ref: bool FUN_00481b24(int param_1, int
// param_2)
bool FUN_00481b24_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00481b44, 92 bytes) FUN_00481b44 - ref: undefined4 * FUN_00481b44(undefined4 *
// param_1, undefined4 * param_2)
void *FUN_00481b44_Stub(void *a0, void *a1)
{
    return 0;
}
// STUB(0x00481ba0, 92 bytes) FUN_00481ba0 - ref: undefined4 * FUN_00481ba0(undefined4 *
// param_1, undefined4 * param_2)
void *FUN_00481ba0_Stub(void *a0, void *a1)
{
    return 0;
}
// STUB(0x00481bfc, 49 bytes) FUN_00481bfc - ref: undefined FUN_00481bfc(int param_1)
void FUN_00481bfc_Stub(int a0)
{
}
// STUB(0x00481ccc, 111 bytes) FUN_00481ccc - ref: int FUN_00481ccc(undefined4 param_1,
// int param_2)
int FUN_00481ccc_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00481d3c, 54 bytes) FUN_00481d3c - ref: int FUN_00481d3c(int param_1, int
// param_2)
int FUN_00481d3c_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00481d74, 42 bytes) FUN_00481d74 - ref: undefined4 FUN_00481d74(int param_1, int
// param_2)
int FUN_00481d74_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00481da0, 34 bytes) FUN_00481da0 - ref: undefined FUN_00481da0(int param_1)
void FUN_00481da0_Stub(int a0)
{
}
// STUB(0x00481dc4, 34 bytes) FUN_00481dc4 - ref: undefined FUN_00481dc4(int param_1)
void FUN_00481dc4_Stub(int a0)
{
}
// STUB(0x00481e0c, 467 bytes) FUN_00481e0c - ref: undefined FUN_00481e0c(int param_1, int
// param_2)
void FUN_00481e0c_Stub(int a0, int a1)
{
}
// STUB(0x00481fe0, 25 bytes) FUN_00481fe0 - ref: int FUN_00481fe0(int param_1, int
// param_2)
int FUN_00481fe0_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00481ffc, 36 bytes) FUN_00481ffc - ref: undefined FUN_00481ffc(int param_1)
void FUN_00481ffc_Stub(int a0)
{
}
// STUB(0x00482020, 36 bytes) FUN_00482020 - ref: undefined FUN_00482020(int param_1)
void FUN_00482020_Stub(int a0)
{
}
// STUB(0x00482044, 36 bytes) FUN_00482044 - ref: undefined FUN_00482044(int param_1)
void FUN_00482044_Stub(int a0)
{
}
// STUB(0x00482068, 36 bytes) FUN_00482068 - ref: undefined FUN_00482068(int param_1)
void FUN_00482068_Stub(int a0)
{
}
// STUB(0x0048208c, 36 bytes) FUN_0048208c - ref: undefined FUN_0048208c(int param_1)
void FUN_0048208c_Stub(int a0)
{
}
// STUB(0x004820b0, 36 bytes) FUN_004820b0 - ref: undefined FUN_004820b0(int param_1)
void FUN_004820b0_Stub(int a0)
{
}
// STUB(0x004820d4, 89 bytes) FUN_004820d4 - ref: undefined4 * FUN_004820d4(int param_1,
// undefined4 * param_2, undefined4 * param_3)
void *FUN_004820d4_Stub(int a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x00482130, 91 bytes) FUN_00482130 - ref: undefined4 * FUN_00482130(int param_1,
// undefined4 * param_2, undefined4 * param_3)
void *FUN_00482130_Stub(int a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x0048218c, 92 bytes) FUN_0048218c - ref: undefined2 * FUN_0048218c(int param_1,
// undefined2 * param_2, undefined2 * param_3)
void *FUN_0048218c_Stub(int a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x004821e8, 91 bytes) FUN_004821e8 - ref: undefined4 * FUN_004821e8(int param_1,
// undefined4 * param_2, undefined4 * param_3)
void *FUN_004821e8_Stub(int a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x00482244, 91 bytes) FUN_00482244 - ref: undefined4 * FUN_00482244(int param_1,
// undefined4 * param_2, undefined4 * param_3)
void *FUN_00482244_Stub(int a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x004822a0, 91 bytes) FUN_004822a0 - ref: undefined4 * FUN_004822a0(int param_1,
// undefined4 * param_2, undefined4 * param_3)
void *FUN_004822a0_Stub(int a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x004822fc, 53 bytes) FUN_004822fc - ref: undefined4 * FUN_004822fc(undefined4 *
// param_1, undefined4 * param_2, undefined4 * param_3)
void *FUN_004822fc_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x00482334, 48 bytes) FUN_00482334 - ref: undefined4 * FUN_00482334(undefined4 *
// param_1, undefined4 * param_2, undefined4 * param_3)
void *FUN_00482334_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x00482364, 123 bytes) FUN_00482364 - ref: undefined2 * FUN_00482364(undefined2 *
// param_1, undefined2 * param_2, undefined2 * param_3)
void *FUN_00482364_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x004823e0, 53 bytes) FUN_004823e0 - ref: undefined4 * FUN_004823e0(undefined4 *
// param_1, undefined4 * param_2, undefined4 * param_3)
void *FUN_004823e0_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x00482418, 42 bytes) FUN_00482418 - ref: undefined4 * FUN_00482418(undefined4 *
// param_1, undefined4 * param_2, undefined4 * param_3)
void *FUN_00482418_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x00482444, 42 bytes) FUN_00482444 - ref: undefined4 * FUN_00482444(undefined4 *
// param_1, undefined4 * param_2, undefined4 * param_3)
void *FUN_00482444_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x004827c8, 106 bytes) FUN_004827c8 - ref: undefined1 FUN_004827c8(int param_1,
// uint param_2)
char FUN_004827c8_Stub(int a0, unsigned int a1)
{
    return 0;
}
// STUB(0x00482834, 7050 bytes) FUN_00482834 - ref: undefined4 FUN_00482834(int param_1,
// undefined2 * param_2, uint param_3)
int FUN_00482834_Stub(int a0, void *a1, unsigned int a2)
{
    return 0;
}
// STUB(0x0048441c, 161 bytes) FUN_0048441c - ref: undefined FUN_0048441c(undefined4 *
// param_1, uint param_2)
void FUN_0048441c_Stub(void *a0, unsigned int a1)
{
}
// STUB(0x004844c0, 152 bytes) Map_AddNpc - ref: int Map_AddNpc(int npc_list, Npc *
// last_npc, Npc * new_npc)
int Map_AddNpc_Stub(int a0, void *a1, void *a2)
{
    return 0;
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
// STUB(0x004848d4, 19 bytes) FUN_004848d4 - ref: undefined FUN_004848d4(undefined4
// param_1, undefined4 param_2, undefined4 * param_3)
void FUN_004848d4_Stub(int a0, int a1, void *a2)
{
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
// STUB(0x00484cd4, 42 bytes) FUN_00484cd4 - ref: undefined4 * FUN_00484cd4(undefined4 *
// param_1, undefined4 * param_2, undefined4 * param_3)
void *FUN_00484cd4_Stub(void *a0, void *a1, void *a2)
{
    return 0;
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
// STUB(0x00484e28, 7246 bytes) Mapcontrol_LoadMap - ref: bool
// Mapcontrol_LoadMap(Mapcontrol * this, int map_id)
bool Mapcontrol_LoadMap_Stub(void *a0, int a1)
{
    return 0;
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
// STUB(0x004876c0, 195 bytes) FUN_004876c0 - ref: undefined FUN_004876c0(int param_1, int
// param_2, int param_3)
void FUN_004876c0_Stub(int a0, int a1, int a2)
{
}
#pragma warn.8057
// END GENERATED STUBS
