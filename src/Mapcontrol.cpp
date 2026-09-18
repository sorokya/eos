#include <vcl.h>
#pragma hdrstop

#include "Mapcontrol.h"
#include "Npc.h"
#include "Settings.h"

#pragma package(smart_init)

// Cross-unit (Packets) helpers. Their definitions live in the Packets unit; the
// controllers declare the same prototypes.
int Mapcontrol_GetCount(Mapcontrol *map_control);
MapContainer *Mapcontrol_GetByIndex(Mapcontrol *map_control, int index);

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
                            if ((unsigned short)spec_iter->value == 10 ||
                                (unsigned short)spec_iter->value == 0xb)
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
                            if ((unsigned short)spec_iter->value == 9)
                                result = 0;
                            if ((unsigned short)spec_iter->value == 0xb)
                                result = 2;
                            if ((unsigned short)spec_iter->value != 0x10)
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
                if (((unsigned short)spec_iter->value == 9 ||
                     (unsigned short)spec_iter->value == 0xb) &&
                    (unsigned short)spec_iter->ticks != 2)
                {
                    spec_iter->ticks = 2;
                    result = 1;
                }
                if ((unsigned short)spec_iter->value == 7)
                {
                    result = 1;
                    Mapcontrol_GetByIndex(map_control, map_id - 1)->has_open_doors = 1;
                    spec_iter->value = 9;
                    spec_iter->ticks = 2;
                }
                if ((unsigned short)spec_iter->value == 10)
                {
                    result = 1;
                    Mapcontrol_GetByIndex(map_control, map_id - 1)->has_open_doors = 1;
                    spec_iter->value = 0xb;
                    spec_iter->ticks = 2;
                }
                break;
            }
        }
    }
    return result;
}

int Mapcontrol::Map_GetWarpDoorAt(Mapcontrol *map_control, int map_id, int x, int y)
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
            if ((unsigned short)door_iter->x == x && (unsigned short)door_iter->y == y)
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
