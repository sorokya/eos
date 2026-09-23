#include <vcl.h>
#pragma hdrstop

#include "Mapcontrol.h"
#include "MainForm.h"
#include "Jukeboxcontrol.h"
#include "Npc.h"
#include "Npcvalue.h"
#include "Npcvalues.h"
#include "Settings.h"
#include "Protocol.h"
#include "Players.h"
#include "Packets.h"

#pragma package(smart_init)

void Mapcontrol_AddArenaSpawn(
    MapContainer *self, int map_id, int from_x, int from_y, int to_x, int to_y);
String Mapcontrol_BuildChestItemsString(MapContainer *self, int map_id, MapCoord coords);
MapObject Mapcontrol_GetTileSpecObject(MapContainer *self, int map_id, int x, int y);
unsigned int Mapcontrol_GetTileSpecValueAt(MapContainer *self,
                                           int map_id,
                                           unsigned int x,
                                           unsigned int y);
char Mapcontrol_TryTakeQuestCooldown(MapContainer *self, int map_id);
char Mapcontrol_GetCanScroll(MapContainer *self, int map_id);
MapCoord Mapcontrol_GetRelogCoords(MapContainer *self, int map_id);
unsigned int
Mapcontrol_GetNpcIdByIndex(MapContainer *self, int map_id, unsigned int npc_index);
MapCoord
Mapcontrol_GetNpcCoordsByIndex(MapContainer *self, int map_id, unsigned int npc_index);
char Mapcontrol_IsDropTileClear(MapContainer *self, int map_id, int x, int y);
int Mapcontrol_CountWalkableNeighbors(MapContainer *self,
                                      int map_id,
                                      unsigned int x,
                                      unsigned int y);
void Mapcontrol_ResetMap(MapContainer *self, int map_id);
String Mapcontrol_ReadRawFile(MapContainer *self, int map_id);
char Mapcontrol_ReloadMap(MapContainer *self, int map_id);
bool Mapcontrol_ParseMapFile(MapContainer *self, MapItem *map, int map_id);
int Mapcontrol_GetChestKeyAt(MapContainer *self, int map_id, MapCoord coords);
void Mapcontrol_RemoveGroundItem(MapContainer *self, int map_id, int index);
bool Mapcontrol_CanDropItemAt(
    MapContainer *self, int map_id, int x, int y, int player_id);
GroundItemInfo
Mapcontrol_TakeGroundItemInfo(MapContainer *self, int map_id, int index, int player_id);

// Quest cooldown ticks a map starts with, and the most ground items one owner
// may drop on a single tile.
#define MAP_QUEST_COOLDOWN 10
#define MAP_GROUND_ITEM_MAX 9

MapContainer::MapContainer(Settings *settings)
{
    encode_scratch = new char[8];
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

MapContainer::~MapContainer()
{
}

void MapContainer::Mapcontrol_SetArenaBlock(MapContainer *self, int map_id, int block)
{
    if (map_id > 0 && map_id <= (int)self->maps.size())
        self->maps[map_id - 1].arena_block = block;
}

void Mapcontrol_AddArenaSpawn(
    MapContainer *self, int map_id, int from_x, int from_y, int to_x, int to_y)
{
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        self->maps[map_id - 1].arena_enabled = 1;
        self->maps[map_id - 1].arena_block = 4;
        self->maps[map_id - 1].arena_ticks = RandRange(0x3c);
        MapWarp value(from_x, from_y, map_id, 0, to_x, to_y);
        self->maps[map_id - 1].arena_spawn_list.insert(
            self->maps[map_id - 1].arena_spawn_list.end(), value);
    }
}

void MapContainer::Mapcontrol_IncPlayerCount(MapContainer *self, int map_id)
{
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        self->maps[map_id - 1].player_count++;
        self->maps[map_id - 1].npc_act_ticks = 0xca;
    }
}

void MapContainer::Mapcontrol_DecPlayerCount(MapContainer *self, int map_id)
{
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        if (self->maps[map_id - 1].player_count > 0)
            self->maps[map_id - 1].player_count--;
    }
}

void MapContainer::Mapcontrol_SetTileBits(
    MapContainer *self, MapItem *map, int x, int y, int code)
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

String Mapcontrol_BuildChestItemsString(MapContainer *self, int map_id, MapCoord coords)
{
    String result = "N";
    vector<MapChest>::iterator chest_iter;
    vector<ChestItem>::iterator item_iter;
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        for (chest_iter = self->maps[map_id - 1].chest_list.begin();
             chest_iter != self->maps[map_id - 1].chest_list.end();
             chest_iter++)
        {
            if ((unsigned short)chest_iter->x == coords.x &&
                (unsigned short)chest_iter->y == coords.y)
            {
                result = "";
                for (item_iter = chest_iter->slots.begin();
                     item_iter != chest_iter->slots.end();
                     item_iter++)
                {
                    if (item_iter->item_present != false)
                    {
                        result.Insert(
                            MapContainer::EncodeNumber(self, item_iter->item_id, 2),
                            result.Length() + 1);
                        result.Insert(
                            MapContainer::EncodeNumber(self, item_iter->amount, 3),
                            result.Length() + 1);
                    }
                }
                break;
            }
        }
    }
    return result;
}

int MapContainer::Mapcontrol_GetWarpMap(MapContainer *self, int map_id, int x, int y)
{
    int result = 0;
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        int tile_offset = self->maps[map_id - 1].width * 2 * y + x * 2;
        if (self->maps[map_id - 1].tile_bits[tile_offset])
        {
            for (vector<MapWarp>::iterator warp_iter =
                     self->maps[map_id - 1].warp_list.begin();
                 warp_iter != self->maps[map_id - 1].warp_list.end();
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

int MapContainer::Mapcontrol_GetWarpLevelReq(MapContainer *self, int map_id, int x, int y)
{
    int result = 1000;
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        for (vector<MapWarp>::iterator warp_iter =
                 self->maps[map_id - 1].warp_list.begin();
             warp_iter != self->maps[map_id - 1].warp_list.end();
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

int MapContainer::Mapcontrol_GetWarpX(MapContainer *self, int map_id, int x, int y)
{
    int result = 1000;
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        for (vector<MapWarp>::iterator warp_iter =
                 self->maps[map_id - 1].warp_list.begin();
             warp_iter != self->maps[map_id - 1].warp_list.end();
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

int MapContainer::Mapcontrol_GetWarpY(MapContainer *self, int map_id, int x, int y)
{
    int result = 1000;
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        for (vector<MapWarp>::iterator warp_iter =
                 self->maps[map_id - 1].warp_list.begin();
             warp_iter != self->maps[map_id - 1].warp_list.end();
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

unsigned char MapContainer::Mapcontrol_ToggleDoor(MapContainer *self,
                                                  int map_id,
                                                  unsigned int x,
                                                  unsigned int y)
{
    unsigned char result = 0;
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        for (vector<MapObject>::iterator spec_iter =
                 self->maps[map_id - 1].tile_specs.begin();
             spec_iter != self->maps[map_id - 1].tile_specs.end();
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
                    self->maps[map_id - 1].has_open_doors = 1;
                    spec_iter->value = MapTileSpec_Chest;
                    spec_iter->ticks = 2;
                }
                if ((unsigned short)spec_iter->value == MapTileSpec_Reserved10)
                {
                    result = 1;
                    self->maps[map_id - 1].has_open_doors = 1;
                    spec_iter->value = MapTileSpec_Reserved11;
                    spec_iter->ticks = 2;
                }
                break;
            }
        }
    }
    return result;
}

MapObject Mapcontrol_GetTileSpecObject(MapContainer *self, int map_id, int x, int y)
{
    map_id <= 0 ? (map_id == 1) : 0;
    vector<MapObject>::iterator spec_iter = self->maps[map_id - 1].tile_specs.begin();
    while (spec_iter != self->maps[map_id - 1].tile_specs.end())
    {
        if ((unsigned short)spec_iter->x == x && (unsigned short)spec_iter->y == y)
            break;
        spec_iter++;
    }
    return *spec_iter;
}

unsigned int
MapContainer::Mapcontrol_GetTileSpec(MapContainer *self, int map_id, int x, int y)
{
    unsigned int result = 0xffffffff;
    for (vector<MapObject>::iterator spec_iter =
             self->maps[map_id - 1].tile_specs.begin();
         spec_iter != self->maps[map_id - 1].tile_specs.end();
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

unsigned int Mapcontrol_GetTileSpecValueAt(MapContainer *self,
                                           int map_id,
                                           unsigned int x,
                                           unsigned int y)
{
    unsigned int result = 0xffffffff;
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        int tile_offset = self->maps[map_id - 1].width * 2 * y + x * 2;
        if (self->maps[map_id - 1].tile_bits[tile_offset])
        {
            if (!self->maps[map_id - 1].tile_bits[tile_offset + 1])
            {
                for (vector<MapObject>::iterator spec_iter =
                         self->maps[map_id - 1].tile_specs.begin();
                     spec_iter != self->maps[map_id - 1].tile_specs.end();
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

char Mapcontrol_TryTakeQuestCooldown(MapContainer *self, int map_id)
{
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        if (self->maps[map_id - 1].quest_cooldown < 1)
        {
            self->maps[map_id - 1].quest_cooldown = MAP_QUEST_COOLDOWN;
            return 1;
        }
    }
    return 0;
}

char Mapcontrol_GetCanScroll(MapContainer *self, int map_id)
{
    char result = 0;
    if (map_id > 0 && map_id <= (int)self->maps.size())
        result = self->maps[map_id - 1].can_scroll;
    return result;
}

MapCoord Mapcontrol_GetRelogCoords(MapContainer *self, int map_id)
{
    MapCoord coords;
    coords.x = 0;
    coords.y = 0;
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        coords.x = self->maps[map_id - 1].relog_x;
        coords.y = self->maps[map_id - 1].relog_y;
        if (coords.x >= (int)self->maps[map_id - 1].width ||
            coords.y >= (int)self->maps[map_id - 1].height)
        {
            coords.x = 0;
            coords.y = 0;
        }
    }
    return coords;
}

bool MapContainer::Mapcontrol_AggroChildNpcs(MapContainer *self, int map_id)
{
    bool found_child = false;
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        TDateTime now = Now();
        for (vector<Npc *>::iterator npc_iter = self->maps[map_id - 1].npc_list.begin();
             npc_iter != self->maps[map_id - 1].npc_list.end();
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

bool MapContainer::Mapcontrol_KillChildNpcs(MapContainer *self, int map_id)
{
    bool found_child = false;
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        self->maps[map_id - 1].boss_alive = false;
        TDateTime now = Now();
        for (vector<Npc *>::iterator npc_iter = self->maps[map_id - 1].npc_list.begin();
             npc_iter != self->maps[map_id - 1].npc_list.end();
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

unsigned int
Mapcontrol_GetNpcIdByIndex(MapContainer *self, int map_id, unsigned int npc_index)
{
    unsigned int result = 0xffffffff;
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        for (vector<Npc *>::iterator npc_iter = self->maps[map_id - 1].npc_list.begin();
             npc_iter != self->maps[map_id - 1].npc_list.end();
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

MapCoord
Mapcontrol_GetNpcCoordsByIndex(MapContainer *self, int map_id, unsigned int npc_index)
{
    MapCoord coords;
    coords.x = -1;
    coords.y = -1;
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        for (vector<Npc *>::iterator npc_iter = self->maps[map_id - 1].npc_list.begin();
             npc_iter != self->maps[map_id - 1].npc_list.end();
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

bool MapContainer::Mapcontrol_IsOccupied(MapContainer *self, int map_id, int x, int y)
{
    bool result = false;
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        for (vector<Npc *>::iterator npc_iter = self->maps[map_id - 1].npc_list.begin();
             npc_iter != self->maps[map_id - 1].npc_list.end();
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

int MapContainer::Mapcontrol_CountNpcsChasingPlayer(MapContainer *self,
                                                    int map_id,
                                                    int player_id)
{
    int chase_count = 0;
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        for (vector<Npc *>::iterator npc_iter = self->maps[map_id - 1].npc_list.begin();
             npc_iter != self->maps[map_id - 1].npc_list.end();
             npc_iter++)
        {
            if ((*npc_iter)->chase_target_id == player_id)
                chase_count++;
        }
    }
    return chase_count;
}

char Mapcontrol_IsDropTileClear(MapContainer *self, int map_id, int x, int y)
{
    char result = 1;
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        int tile_offset = self->maps[map_id - 1].width * 2 * y + x * 2;
        if (x >= 0 && y >= 0 && x < (int)self->maps[map_id - 1].width &&
            y < (int)self->maps[map_id - 1].height)
        {
            if (self->maps[map_id - 1].tile_bits[tile_offset])
            {
                if (!self->maps[map_id - 1].tile_bits[tile_offset + 1])
                {
                    result = 0;
                    for (vector<MapObject>::iterator spec_iter =
                             self->maps[map_id - 1].tile_specs.begin();
                         spec_iter != self->maps[map_id - 1].tile_specs.end();
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
                if (self->maps[map_id - 1].tile_bits[tile_offset + 1])
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

bool MapContainer::Mapcontrol_IsTileClear(MapContainer *self, int map_id, int x, int y)
{
    bool result = true;
    if (map_id > 0 && (int)self->maps.size() >= map_id)
    {
        int tile_offset = self->maps[map_id - 1].width * 2 * y + x * 2;
        if (x >= 0 && y >= 0 && x < (int)self->maps[map_id - 1].width &&
            y < (int)self->maps[map_id - 1].height)
        {
            if (self->maps[map_id - 1].tile_bits[tile_offset])
            {
                if (!self->maps[map_id - 1].tile_bits[tile_offset + 1])
                    result = false;
            }
            else
            {
                if (self->maps[map_id - 1].tile_bits[tile_offset + 1])
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

bool MapContainer::Mapcontrol_IsTileWalkable(MapContainer *self, int map_id, int x, int y)
{
    bool result = false;
    if (map_id > 0 && (int)self->maps.size() >= map_id)
    {
        int tile_offset = self->maps[map_id - 1].width * 2 * y + x * 2;
        if (x >= 0 && y >= 0 && x < (int)self->maps[map_id - 1].width &&
            y < (int)self->maps[map_id - 1].height)
        {
            if (self->maps[map_id - 1].tile_bits[tile_offset])
            {
                if (self->maps[map_id - 1].tile_bits[tile_offset + 1])
                {
                    result = true;
                }
                else
                {
                    for (vector<MapObject>::iterator spec_iter =
                             self->maps[map_id - 1].tile_specs.begin();
                         spec_iter != self->maps[map_id - 1].tile_specs.end();
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

int Mapcontrol_CountWalkableNeighbors(MapContainer *self,
                                      int map_id,
                                      unsigned int x,
                                      unsigned int y)
{
    int result = 0;
    if (MapContainer::Mapcontrol_GetWalkableStatus(self, map_id, x - 1, y, 1) == 0)
        result = result + 1;
    if (MapContainer::Mapcontrol_GetWalkableStatus(self, map_id, x, y - 1, 1) == 0)
        result = result + 1;
    if (MapContainer::Mapcontrol_GetWalkableStatus(self, map_id, x + 1, y, 1) == 0)
        result = result + 1;
    if (MapContainer::Mapcontrol_GetWalkableStatus(self, map_id, x, y + 1, 1) == 0)
        result = result + 1;
    return result;
}

int MapContainer::Mapcontrol_GetWalkableStatus(
    MapContainer *self, int map_id, int x, int y, char ignore_spec_block)
{
    int result = 1;
    if (map_id > 0 && (int)self->maps.size() >= map_id)
    {
        int tile_offset = self->maps[map_id - 1].width * 2 * y + x * 2;
        if (x >= 0 && y >= 0 && x < (int)self->maps[map_id - 1].width &&
            y < (int)self->maps[map_id - 1].height)
        {
            if (self->maps[map_id - 1].tile_bits[tile_offset])
            {
                if (self->maps[map_id - 1].tile_bits[tile_offset + 1])
                {
                    result = 2;
                }
                else
                {
                    result = 1;
                    for (vector<MapObject>::iterator spec_iter =
                             self->maps[map_id - 1].tile_specs.begin();
                         spec_iter != self->maps[map_id - 1].tile_specs.end();
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
                if (self->maps[map_id - 1].tile_bits[tile_offset + 1])
                    result = 1;
                else
                    result = 0;
            }
        }
    }
    return result;
}

void MapContainer::Mapcontrol_AddTileSpec(
    MapContainer *self, MapItem *map, int x, int y, int spec)
{
    MapObject value(x, y, spec);
    map->tile_specs.insert(map->tile_specs.end(), value);
}

void MapContainer::Mapcontrol_AddWarp(MapContainer *self,
                                      MapItem *map,
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

void MapContainer::Mapcontrol_AddLockKey(
    MapContainer *self, MapItem *map, unsigned int x, unsigned int y, int key_id)
{
    bool found = false;
    for (vector<MapObject>::iterator lock_iter = map->legacy_door_key_list.begin();
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

void MapContainer::Mapcontrol_GetOrCreateChest(MapContainer *self,
                                               MapItem *map,
                                               unsigned int x,
                                               unsigned int y)
{
    bool found = false;
    for (vector<MapChest>::iterator chest_iter = map->chest_list.begin();
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

void MapContainer::Mapcontrol_AddChestSpawn(MapContainer *self,
                                            MapItem *map,
                                            unsigned int x,
                                            unsigned int y,
                                            int key_id,
                                            int slot,
                                            int item_id,
                                            int spawn_time,
                                            int amount)
{
    bool found = false;
    vector<MapChest>::iterator chest_iter = map->chest_list.begin();
    while (chest_iter != map->chest_list.end())
    {
        if ((unsigned short)chest_iter->x == x && (unsigned short)chest_iter->y == y)
        {
            found = true;
            chest_iter->key_id = key_id;
            if (slot >= (int)chest_iter->slots.size())
            {
                ChestItem extra_slot(item_id);
                extra_slot.item_present = false;
                extra_slot.respawn_enabled = true;
                extra_slot.respawn_countdown = spawn_time;
                extra_slot.respawn_delay = spawn_time;
                extra_slot.amount = amount;
                extra_slot.alt_item_id0 = item_id;
                extra_slot.alt_amount0 = amount;
                extra_slot.alt_item_id1 = 0;
                extra_slot.alt_item_id2 = 0;
                extra_slot.alt_item_id3 = 0;
                chest_iter->slots.insert(chest_iter->slots.end(), extra_slot);
            }
            else
            {
                if (chest_iter->slots[slot].alt_item_id1 > 0)
                {
                    if (chest_iter->slots[slot].alt_item_id2 > 0)
                    {
                        chest_iter->slots[slot].alt_item_id3 = item_id;
                        chest_iter->slots[slot].alt_amount3 = amount;
                    }
                    else
                    {
                        chest_iter->slots[slot].alt_item_id2 = item_id;
                        chest_iter->slots[slot].alt_amount2 = amount;
                    }
                }
                else
                {
                    chest_iter->slots[slot].alt_item_id1 = item_id;
                    chest_iter->slots[slot].alt_amount1 = amount;
                }
            }
            break;
        }
        chest_iter++;
    }
    if (!found)
    {
        MapChest new_chest(x, y, key_id);
        ChestItem new_item(item_id);
        new_item.item_present = false;
        new_item.respawn_enabled = true;
        new_item.respawn_countdown = spawn_time;
        new_item.respawn_delay = spawn_time;
        new_item.amount = amount;
        new_item.alt_item_id0 = item_id;
        new_item.alt_amount0 = amount;
        new_item.alt_item_id1 = 0;
        new_item.alt_item_id2 = 0;
        new_item.alt_item_id3 = 0;
        new_chest.slots.insert(new_chest.slots.end(), new_item);
        map->chest_list.insert(map->chest_list.end(), new_chest);
    }
}

void MapContainer::Mapcontrol_LoadMaps(MapContainer *self)
{
    for (int map_id = 1; map_id <= 0xfa00 && map_id <= self->max_maps; map_id++)
    {
        if (!Mapcontrol_LoadMap(self, map_id))
        {
            MapItem value(map_id, 0, 0);
            self->maps.insert(self->maps.end(), value);
        }
    }
}

void Mapcontrol_ResetMap(MapContainer *self, int map_id)
{
    for (int i = 0; i < (int)self->maps[map_id - 1].chest_list.size(); i++)
    {
        self->maps[map_id - 1].chest_list[i].slots.clear();
    }
    for (int i = 0; i < (int)self->maps[map_id - 1].tile_bits.size(); i++)
    {
        self->maps[map_id - 1].tile_bits[i] = false;
    }
    self->maps[map_id - 1].tile_specs.clear();
    self->maps[map_id - 1].chest_list.clear();
    self->maps[map_id - 1].warp_list.clear();
    ((vector<ItemObj *> *)&self->maps[map_id - 1].ground_items)->clear();
    self->maps[map_id - 1].npc_list.clear();
    self->maps[map_id - 1].legacy_door_key_list.clear();
    self->maps[map_id - 1].boss_alive = false;
    self->maps[map_id - 1].width = 0;
    self->maps[map_id - 1].height = 0;
    self->maps[map_id - 1].buf.Delete(0, self->maps[map_id].buf.Length());
}

String Mapcontrol_ReadRawFile(MapContainer *self, int map_id)
{
    String result;
    if (self->maps[map_id - 1].buf == "")
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
        result = self->maps[map_id - 1].buf;
    }
    return result;
}

char Mapcontrol_ReloadMap(MapContainer *self, int map_id)
{
    char result = 0;
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        Mapcontrol_ResetMap(self, map_id);
        JukeBoxController_RemoveMap(GUI->jukebox_control, map_id);
        result = (char)Mapcontrol_ParseMapFile(self, &self->maps[map_id - 1], map_id);
    }
    return result;
}

bool Mapcontrol_ParseMapFile(MapContainer *self, MapItem *map, int map_id)
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
        int row_y;
        int count;
        int spec;
        int tile_count;
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
        map->width = MapContainer::DecodeNumber(self, map_buf.SubString(0x26, 1)) + 1;
        map->height = MapContainer::DecodeNumber(self, map_buf.SubString(0x27, 1)) + 1;
        map->rid1 = MapContainer::DecodeNumber(self, map_buf.SubString(4, 2));
        map->rid2 = MapContainer::DecodeNumber(self, map_buf.SubString(6, 2));
        map->map_type = MapContainer::DecodeNumber(self, map_buf.SubString(0x20, 1));
        map->timed_effect = MapContainer::DecodeNumber(self, map_buf.SubString(0x21, 1));
        map->relog_x = MapContainer::DecodeNumber(self, map_buf.SubString(0x2c, 1));
        map->relog_y = MapContainer::DecodeNumber(self, map_buf.SubString(0x2d, 1));
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
        if (map->tile_bits.size() != map->width * map->height * 2)
            map->tile_bits.resize(map->width * map->height * 2, 0);
        if (MapContainer::DecodeNumber(self, map_buf.SubString(0x2b, 1)) == 0)
            map->can_scroll = 1;
        map->filesize = size;
        if (self->start_map == map_id || self->memory_map != 0)
        {
            map->buf_copied = true;
            map->buf = map_buf;
        }
        else
            map->buf_copied = false;
        map_buf.Delete(1, 0x2e);
        count = MapContainer::DecodeNumber(self, map_buf.SubString(1, 1));
        map_buf.Delete(1, 1);
        for (int i = 0; i < count; i++)
        {
            int npc_count = MapContainer::DecodeNumber(self, map_buf.SubString(8, 1));
            for (int j = 0; j < npc_count; j++)
            {
                int index = map->npc_list.size() + 1;
                Npc *npc =
                    new Npc(index,
                            MapContainer::DecodeNumber(self, map_buf.SubString(3, 2)),
                            MapContainer::DecodeNumber(self, map_buf.SubString(1, 1)),
                            MapContainer::DecodeNumber(self, map_buf.SubString(2, 1)),
                            0,
                            MapContainer::DecodeNumber(self, map_buf.SubString(5, 1)),
                            MapContainer::DecodeNumber(self, map_buf.SubString(6, 2)));
                NpcValue npc_value = NpcValues::GetNpc(
                    GUI->npc_values,
                    MapContainer::DecodeNumber(self, map_buf.SubString(3, 2)));
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
                    ((short *)&npc->pad_0x34)[k] = 0;
                if (npc_value.element_weakness > 0 && npc_value.element_weakness < 7)
                    ((short *)&npc->pad_0x34)[npc_value.element_weakness] =
                        npc_value.element_weakness_damage;
                map->npc_list.insert((Npc **)map->npc_list.end(), npc);
            }
            map_buf.Delete(1, 8);
        }
        count = MapContainer::DecodeNumber(self, map_buf.SubString(1, 1));
        map_buf.Delete(1, 1);
        for (int i = 0; i < count; i++)
        {
            unsigned int key_x =
                MapContainer::DecodeNumber(self, map_buf.SubString(1, 1));
            unsigned int key_y =
                MapContainer::DecodeNumber(self, map_buf.SubString(2, 1));
            int key_id = MapContainer::DecodeNumber(self, map_buf.SubString(3, 2));
            MapContainer::Mapcontrol_AddLockKey(self, map, key_x, key_y, key_id);
            map_buf.Delete(1, 4);
        }
        count = MapContainer::DecodeNumber(self, map_buf.SubString(1, 1));
        map_buf.Delete(1, 1);
        for (int i = 0; i < count; i++)
        {
            unsigned int chest_x =
                MapContainer::DecodeNumber(self, map_buf.SubString(1, 1));
            unsigned int chest_y =
                MapContainer::DecodeNumber(self, map_buf.SubString(2, 1));
            int chest_key = MapContainer::DecodeNumber(self, map_buf.SubString(3, 2));
            int chest_slot = MapContainer::DecodeNumber(self, map_buf.SubString(5, 1));
            int chest_item = MapContainer::DecodeNumber(self, map_buf.SubString(6, 2));
            int chest_time = MapContainer::DecodeNumber(self, map_buf.SubString(8, 2));
            int chest_amount =
                MapContainer::DecodeNumber(self, map_buf.SubString(0xa, 3));
            MapContainer::Mapcontrol_AddChestSpawn(self,
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
        count = MapContainer::DecodeNumber(self, map_buf.SubString(1, 1));
        map_buf.Delete(1, 1);
        for (int i = 0; i < count; i++)
        {
            row_y = MapContainer::DecodeNumber(self, map_buf.SubString(1, 1));
            tile_count = MapContainer::DecodeNumber(self, map_buf.SubString(2, 1));
            map_buf.Delete(1, 2);
            for (int k = 0; k < tile_count; k++)
            {
                spec = MapContainer::DecodeNumber(self, map_buf.SubString(1, 1));
                int code = MapContainer::DecodeNumber(self, map_buf.SubString(2, 1));
                if (code == 0 || code == 0x12)
                    MapContainer::Mapcontrol_SetTileBits(self, map, spec, row_y, 1);
                if (code > 0 && code <= 0x11)
                {
                    MapContainer::Mapcontrol_SetTileBits(self, map, spec, row_y, 2);
                    MapContainer::Mapcontrol_AddTileSpec(
                        self, map, spec, row_y, code - 1);
                }
                if (code == 0x13 || code == 0x1d)
                {
                    MapContainer::Mapcontrol_SetTileBits(self, map, spec, row_y, 2);
                    MapContainer::Mapcontrol_AddTileSpec(self, map, spec, row_y, 0x10);
                }
                if (code > 0x13 && code <= 0x1b)
                {
                    MapContainer::Mapcontrol_SetTileBits(self, map, spec, row_y, 2);
                    MapContainer::Mapcontrol_AddTileSpec(
                        self, map, spec, row_y, code - 1);
                }
                if (code == 0x1c)
                {
                    MapContainer::Mapcontrol_SetTileBits(self, map, spec, row_y, 2);
                    MapContainer::Mapcontrol_AddTileSpec(
                        self, map, spec, row_y, code - 1);
                    JukeBoxController::Add(GUI->jukebox_control, map_id);
                }
                if (code == 9)
                    MapContainer::Mapcontrol_GetOrCreateChest(self, map, spec, row_y);
                if (code == 0x20)
                    MapContainer::Mapcontrol_AddTileSpec(
                        self, map, spec, row_y, code - 1);
                if (code > 0x21 && code < 0x25)
                {
                    MapContainer::Mapcontrol_AddTileSpec(
                        self, map, spec, row_y, code - 1);
                    map->has_spikes = 1;
                }
                map_buf.Delete(1, 2);
            }
        }
        count = MapContainer::DecodeNumber(self, map_buf.SubString(1, 1));
        map_buf.Delete(1, 1);
        for (int i = 0; i < count; i++)
        {
            row_y = MapContainer::DecodeNumber(self, map_buf.SubString(1, 1));
            tile_count = MapContainer::DecodeNumber(self, map_buf.SubString(2, 1));
            map_buf.Delete(1, 2);
            for (int k = 0; k < tile_count; k++)
            {
                spec = MapContainer::DecodeNumber(self, map_buf.SubString(1, 1));
                int lock_key = MapContainer::DecodeNumber(self, map_buf.SubString(7, 2));
                MapContainer::Mapcontrol_SetTileBits(self, map, spec, row_y, 3);
                MapContainer::Mapcontrol_AddWarp(
                    self,
                    map,
                    spec,
                    row_y,
                    MapContainer::DecodeNumber(self, map_buf.SubString(2, 2)),
                    MapContainer::DecodeNumber(self, map_buf.SubString(6, 1)),
                    MapContainer::DecodeNumber(self, map_buf.SubString(4, 1)),
                    MapContainer::DecodeNumber(self, map_buf.SubString(5, 1)));
                if (lock_key > 0)
                {
                    MapContainer::Mapcontrol_AddTileSpec(self, map, spec, row_y, 0xa);
                    MapContainer::Mapcontrol_SetTileBits(self, map, spec, row_y, 2);
                    if (lock_key > 1)
                        MapContainer::Mapcontrol_AddLockKey(
                            self, map, spec, row_y, lock_key);
                }
                map_buf.Delete(1, 8);
            }
        }
    }
    catch (...)
    {
        FileClose(file_handle);
        return 0;
    }
    return 1;
}

bool MapContainer::Mapcontrol_LoadMap(MapContainer *self, int map_id)
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
        int row_y;
        int count;
        int spec;
        int tile_count;
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
        MapItem map(map_id,
                    MapContainer::DecodeNumber(self, map_buf.SubString(0x26, 1)) + 1,
                    MapContainer::DecodeNumber(self, map_buf.SubString(0x27, 1)) + 1);
        map.rid1 = MapContainer::DecodeNumber(self, map_buf.SubString(4, 2));
        map.rid2 = MapContainer::DecodeNumber(self, map_buf.SubString(6, 2));
        map.map_type = MapContainer::DecodeNumber(self, map_buf.SubString(0x20, 1));
        map.timed_effect = MapContainer::DecodeNumber(self, map_buf.SubString(0x21, 1));
        map.relog_x = MapContainer::DecodeNumber(self, map_buf.SubString(0x2c, 1));
        map.relog_y = MapContainer::DecodeNumber(self, map_buf.SubString(0x2d, 1));
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
        if (MapContainer::DecodeNumber(self, map_buf.SubString(0x2b, 1)) == 0)
            map.can_scroll = 1;
        map.filesize = size;
        if (self->start_map == map_id || self->memory_map != 0)
        {
            map.buf_copied = true;
            map.buf = map_buf;
        }
        else
            map.buf_copied = false;
        map_buf.Delete(1, 0x2e);
        count = MapContainer::DecodeNumber(self, map_buf.SubString(1, 1));
        map_buf.Delete(1, 1);
        for (int i = 0; i < count; i++)
        {
            int npc_count = MapContainer::DecodeNumber(self, map_buf.SubString(8, 1));
            for (int j = 0; j < npc_count; j++)
            {
                int index = map.npc_list.size() + 1;
                Npc *npc =
                    new Npc(index,
                            MapContainer::DecodeNumber(self, map_buf.SubString(3, 2)),
                            MapContainer::DecodeNumber(self, map_buf.SubString(1, 1)),
                            MapContainer::DecodeNumber(self, map_buf.SubString(2, 1)),
                            0,
                            MapContainer::DecodeNumber(self, map_buf.SubString(5, 1)),
                            MapContainer::DecodeNumber(self, map_buf.SubString(6, 2)));
                NpcValue npc_value = NpcValues::GetNpc(
                    GUI->npc_values,
                    MapContainer::DecodeNumber(self, map_buf.SubString(3, 2)));
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
                    ((short *)&npc->pad_0x34)[k] = 0;
                if (npc_value.element_weakness > 0 && npc_value.element_weakness < 7)
                    ((short *)&npc->pad_0x34)[npc_value.element_weakness] =
                        npc_value.element_weakness_damage;
                map.npc_list.insert((Npc **)map.npc_list.end(), npc);
            }
            map_buf.Delete(1, 8);
        }
        count = MapContainer::DecodeNumber(self, map_buf.SubString(1, 1));
        map_buf.Delete(1, 1);
        for (int i = 0; i < count; i++)
        {
            unsigned int key_x =
                MapContainer::DecodeNumber(self, map_buf.SubString(1, 1));
            unsigned int key_y =
                MapContainer::DecodeNumber(self, map_buf.SubString(2, 1));
            int key_id = MapContainer::DecodeNumber(self, map_buf.SubString(3, 2));
            MapContainer::Mapcontrol_AddLockKey(self, &map, key_x, key_y, key_id);
            map_buf.Delete(1, 4);
        }
        count = MapContainer::DecodeNumber(self, map_buf.SubString(1, 1));
        map_buf.Delete(1, 1);
        for (int i = 0; i < count; i++)
        {
            unsigned int chest_x =
                MapContainer::DecodeNumber(self, map_buf.SubString(1, 1));
            unsigned int chest_y =
                MapContainer::DecodeNumber(self, map_buf.SubString(2, 1));
            int chest_key = MapContainer::DecodeNumber(self, map_buf.SubString(3, 2));
            int chest_slot = MapContainer::DecodeNumber(self, map_buf.SubString(5, 1));
            int chest_item = MapContainer::DecodeNumber(self, map_buf.SubString(6, 2));
            int chest_time = MapContainer::DecodeNumber(self, map_buf.SubString(8, 2));
            int chest_amount =
                MapContainer::DecodeNumber(self, map_buf.SubString(0xa, 3));
            MapContainer::Mapcontrol_AddChestSpawn(self,
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
        count = MapContainer::DecodeNumber(self, map_buf.SubString(1, 1));
        map_buf.Delete(1, 1);
        for (int i = 0; i < count; i++)
        {
            row_y = MapContainer::DecodeNumber(self, map_buf.SubString(1, 1));
            tile_count = MapContainer::DecodeNumber(self, map_buf.SubString(2, 1));
            map_buf.Delete(1, 2);
            for (int k = 0; k < tile_count; k++)
            {
                spec = MapContainer::DecodeNumber(self, map_buf.SubString(1, 1));
                int code = MapContainer::DecodeNumber(self, map_buf.SubString(2, 1));
                if (code == 0 || code == 0x12)
                    MapContainer::Mapcontrol_SetTileBits(self, &map, spec, row_y, 1);
                if (code > 0 && code <= 0x11)
                {
                    MapContainer::Mapcontrol_SetTileBits(self, &map, spec, row_y, 2);
                    MapContainer::Mapcontrol_AddTileSpec(
                        self, &map, spec, row_y, code - 1);
                }
                if (code == 0x13 || code == 0x1d)
                {
                    MapContainer::Mapcontrol_SetTileBits(self, &map, spec, row_y, 2);
                    MapContainer::Mapcontrol_AddTileSpec(self, &map, spec, row_y, 0x10);
                }
                if (code > 0x13 && code <= 0x1b)
                {
                    MapContainer::Mapcontrol_SetTileBits(self, &map, spec, row_y, 2);
                    MapContainer::Mapcontrol_AddTileSpec(
                        self, &map, spec, row_y, code - 1);
                }
                if (code == 0x1c)
                {
                    MapContainer::Mapcontrol_SetTileBits(self, &map, spec, row_y, 2);
                    MapContainer::Mapcontrol_AddTileSpec(
                        self, &map, spec, row_y, code - 1);
                    JukeBoxController::Add(GUI->jukebox_control, map_id);
                }
                if (code == 9)
                    MapContainer::Mapcontrol_GetOrCreateChest(self, &map, spec, row_y);
                if (code == 0x20)
                    MapContainer::Mapcontrol_AddTileSpec(
                        self, &map, spec, row_y, code - 1);
                if (code > 0x21 && code < 0x25)
                {
                    MapContainer::Mapcontrol_AddTileSpec(
                        self, &map, spec, row_y, code - 1);
                    map.has_spikes = 1;
                }
                map_buf.Delete(1, 2);
            }
        }
        count = MapContainer::DecodeNumber(self, map_buf.SubString(1, 1));
        map_buf.Delete(1, 1);
        for (int i = 0; i < count; i++)
        {
            row_y = MapContainer::DecodeNumber(self, map_buf.SubString(1, 1));
            tile_count = MapContainer::DecodeNumber(self, map_buf.SubString(2, 1));
            map_buf.Delete(1, 2);
            for (int k = 0; k < tile_count; k++)
            {
                spec = MapContainer::DecodeNumber(self, map_buf.SubString(1, 1));
                int lock_key = MapContainer::DecodeNumber(self, map_buf.SubString(7, 2));
                MapContainer::Mapcontrol_SetTileBits(self, &map, spec, row_y, 3);
                MapContainer::Mapcontrol_AddWarp(
                    self,
                    &map,
                    spec,
                    row_y,
                    MapContainer::DecodeNumber(self, map_buf.SubString(2, 2)),
                    MapContainer::DecodeNumber(self, map_buf.SubString(6, 1)),
                    MapContainer::DecodeNumber(self, map_buf.SubString(4, 1)),
                    MapContainer::DecodeNumber(self, map_buf.SubString(5, 1)));
                if (lock_key > 0)
                {
                    MapContainer::Mapcontrol_AddTileSpec(self, &map, spec, row_y, 0xa);
                    MapContainer::Mapcontrol_SetTileBits(self, &map, spec, row_y, 2);
                    if (lock_key > 1)
                        MapContainer::Mapcontrol_AddLockKey(
                            self, &map, spec, row_y, lock_key);
                }
                map_buf.Delete(1, 8);
            }
        }
        self->maps.insert(self->maps.end(), map);
    }
    catch (...)
    {
        FileClose(file_handle);
        return false;
    }
    return true;
}

String MapContainer::EncodeNumber(MapContainer *self, unsigned int value, int width)
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
                rem = value % EO_NUM_MAX;
                c = rem + 1;
                ((char *)self->encode_scratch)[i] = c;
                value = quotient;
                if (quotient < 1)
                    flag = false;
                else if (i + 1 == width)
                    width++;
            }
            else
            {
                char pad = EO_NUM_EMPTY;
                ((char *)self->encode_scratch)[i] = pad;
            }
        }
    }
    catch (...)
    {
        width = 0;
    }
    String encoded_str((char *)self->encode_scratch, width);
    return encoded_str;
}

int MapContainer::DecodeNumber(MapContainer *self, String value)
{
    int result = 0;
    try
    {
        for (int digit_index = 1; digit_index <= value.Length(); digit_index++)
        {
            char c = value[digit_index];
            unsigned char ch = c;
            if (ch == EO_NUM_EMPTY || ch == 0)
                break;
            int v = ch;
            v = v - 1;
            if (digit_index == 1)
                result = result + v;
            if (digit_index == 2)
                result = result + v * EO_NUM_MAX;
            if (digit_index == 3)
                result = result + v * EO_NUM_MAX_2;
            if (digit_index == 4)
                result = result + v * EO_NUM_MAX_3;
        }
    }
    catch (...)
    {
        result = 0;
    }
    return result;
}

int MapContainer::Mapcontrol_GetChestSlotCount(MapContainer *self,
                                               int map_id,
                                               MapCoord coords)
{
    int result = -1;
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        for (vector<MapChest>::iterator chest_iter =
                 self->maps[map_id - 1].chest_list.begin();
             chest_iter != self->maps[map_id - 1].chest_list.end();
             chest_iter++)
        {
            if ((unsigned short)chest_iter->x == coords.x &&
                (unsigned short)chest_iter->y == coords.y)
            {
                result = chest_iter->slots.size();
                break;
            }
        }
    }
    return result;
}

int MapContainer::Mapcontrol_GetWarpDoorAt(MapContainer *self,
                                           int map_id,
                                           MapCoord coords)
{
    int result = 0;
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        for (vector<MapObject>::iterator door_iter =
                 self->maps[map_id - 1].legacy_door_key_list.begin();
             door_iter != self->maps[map_id - 1].legacy_door_key_list.end();
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

int Mapcontrol_GetChestKeyAt(MapContainer *self, int map_id, MapCoord coords)
{
    int result = 0;
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        for (vector<MapChest>::iterator chest_iter =
                 self->maps[map_id - 1].chest_list.begin();
             chest_iter != self->maps[map_id - 1].chest_list.end();
             chest_iter++)
        {
            if ((unsigned short)chest_iter->x == coords.x &&
                (unsigned short)chest_iter->y == coords.y)
            {
                result = (unsigned short)chest_iter->key_id;
                break;
            }
        }
    }
    return result;
}

void MapContainer::Mapcontrol_AddChestItem(
    MapContainer *self, int map_id, MapCoord coords, int item_id, int amount)
{
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        for (vector<MapChest>::iterator chest_iter =
                 self->maps[map_id - 1].chest_list.begin();
             chest_iter != self->maps[map_id - 1].chest_list.end();
             chest_iter++)
        {
            if ((unsigned short)chest_iter->x == coords.x &&
                (unsigned short)chest_iter->y == coords.y)
            {
                bool found = false;
                for (vector<ChestItem>::iterator item_iter = chest_iter->slots.begin();
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
                    ChestItem new_item(item_id);
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

int MapContainer::Mapcontrol_AddGroundItem(MapContainer *self,
                                           int map_id,
                                           unsigned int item_id,
                                           int x,
                                           int y,
                                           unsigned int amount,
                                           int owner_player_id,
                                           unsigned short protect_ticks)
{
    int item_index = -1;
    if (map_id > 0 && map_id <= (int)self->maps.size() && x >= 0 && y >= 0 &&
        x < (int)self->maps[map_id - 1].width && y < (int)self->maps[map_id - 1].height)
    {
        ItemObj *item = new ItemObj();
        item->index = self->maps[map_id - 1].next_ground_item_id;
        item->item_id = item_id;
        item->x = x;
        item->y = y;
        item->amount = amount;
        item->drop_time = DateTimeToTimeStamp(Now());
        item->owner_player_id = owner_player_id;
        item->protect_ticks = protect_ticks;
        if (self->maps[map_id - 1].next_ground_item_id == 15000)
            Mapcontrol_PurgeGroundItemsInRange(self, map_id, 15000, 30000);
        if (self->maps[map_id - 1].next_ground_item_id == 30000)
            Mapcontrol_PurgeGroundItemsInRange(self, map_id, 30000, 45000);
        if (self->maps[map_id - 1].next_ground_item_id == 45000)
            Mapcontrol_PurgeGroundItemsInRange(self, map_id, 45000, 60000);
        if (self->maps[map_id - 1].next_ground_item_id == 60000)
            Mapcontrol_PurgeGroundItemsInRange(self, map_id, 0, 15000);
        if (self->maps[map_id - 1].next_ground_item_id >= 60000)
            self->maps[map_id - 1].next_ground_item_id = 0;
        ((vector<ItemObj *> *)&self->maps[map_id - 1].ground_items)
            ->insert(((vector<ItemObj *> *)&self->maps[map_id - 1].ground_items)->end(),
                     item);
        item_index = self->maps[map_id - 1].next_ground_item_id;
        self->maps[map_id - 1].next_ground_item_id =
            self->maps[map_id - 1].next_ground_item_id + 1;
    }
    return item_index;
}

void Mapcontrol_RemoveGroundItem(MapContainer *self, int map_id, int index)
{
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        for (vector<ItemObj *>::iterator cursor =
                 ((vector<ItemObj *> *)&self->maps[map_id - 1].ground_items)->begin();
             cursor != ((vector<ItemObj *> *)&self->maps[map_id - 1].ground_items)->end();
             cursor++)
        {
            if ((*cursor)->index == index)
            {
                ItemObj *item = *cursor;
                ((vector<ItemObj *> *)&self->maps[map_id - 1].ground_items)
                    ->erase(cursor);
                delete item;
                break;
            }
        }
    }
}

ItemStack MapContainer::Mapcontrol_TakeChestItem(MapContainer *self,
                                                 int map_id,
                                                 MapCoord coords,
                                                 int item_id)
{
    ItemStack result;
    result.id = -1;
    result.amount = -1;
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        for (vector<MapChest>::iterator chest_iter =
                 self->maps[map_id - 1].chest_list.begin();
             chest_iter != self->maps[map_id - 1].chest_list.end();
             chest_iter++)
        {
            if ((unsigned short)chest_iter->x == coords.x &&
                (unsigned short)chest_iter->y == coords.y)
            {
                bool found = false;
                for (vector<ChestItem>::iterator item_iter = chest_iter->slots.begin();
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

bool Mapcontrol_CanDropItemAt(MapContainer *self, int map_id, int x, int y, int player_id)
{
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        int count = 0;
        for (vector<ItemObj *>::iterator cursor =
                 ((vector<ItemObj *> *)&self->maps[map_id - 1].ground_items)->begin();
             cursor != ((vector<ItemObj *> *)&self->maps[map_id - 1].ground_items)->end();
             cursor++)
        {
            if ((*cursor)->x == x && (*cursor)->y == y)
            {
                if ((*cursor)->owner_player_id == player_id)
                {
                    count = count + 1;
                    if (count > MAP_GROUND_ITEM_MAX)
                        return 0;
                }
                else
                {
                    TTimeStamp now = DateTimeToTimeStamp(Now());
                    int elapsed = now.Date - (*cursor)->drop_time.Date;
                    int ms = now.Time - (*cursor)->drop_time.Time;
                    elapsed = ms / MS_PER_SECOND + elapsed * SECONDS_PER_DAY;
                    if ((*cursor)->protect_ticks > elapsed)
                        return 0;
                }
            }
        }
    }
    return 1;
}

GroundItemInfo
Mapcontrol_TakeGroundItemInfo(MapContainer *self, int map_id, int index, int player_id)
{
    GroundItemInfo result;
    result.x = -1;
    result.y = -1;
    if (map_id > 0 && map_id <= (int)self->maps.size())
    {
        for (vector<ItemObj *>::iterator cursor =
                 ((vector<ItemObj *> *)&self->maps[map_id - 1].ground_items)->begin();
             cursor != ((vector<ItemObj *> *)&self->maps[map_id - 1].ground_items)->end();
             cursor++)
        {
            if ((*cursor)->index == index)
            {
                TTimeStamp now = DateTimeToTimeStamp(Now());
                int elapsed = now.Date - (*cursor)->drop_time.Date;
                int ms = now.Time - (*cursor)->drop_time.Time;
                elapsed = ms / MS_PER_SECOND + elapsed * SECONDS_PER_DAY;
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
void MapContainer::Mapcontrol_PurgeGroundItemsInRange(MapContainer *self,
                                                      int map_id,
                                                      int range_low,
                                                      int range_high)
{
    vector<ItemObj *>::iterator cursor =
        ((vector<ItemObj *> *)&self->maps[map_id - 1].ground_items)->begin();
    while (cursor != ((vector<ItemObj *> *)&self->maps[map_id - 1].ground_items)->end())
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
            ItemObj *item = *cursor;
            cursor = ((vector<ItemObj *> *)&self->maps[map_id - 1].ground_items)
                         ->erase(cursor);
            delete item;
        }
    }
}
