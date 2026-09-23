#ifndef MapcontrolH
#define MapcontrolH

#include <Classes.hpp>
#include <vector.h>

#include "Map.h"
#include "Settings.h"
#include "Protocol.h"

struct GroundItemInfo
{
    struct
    {
        int x;
        int y;
        int item_id;
        int amount;
    };
    GroundItemInfo()
    {
    }
};

// Layout recovered from the reference constructor (MapContainer unit,
// 0x47ad3c..0x487d38). The constructor runs the vector<MapItem>
// default constructor first (member auto-init), then allocates the 8-byte
// encode scratch buffer, stores the Settings pointer and copies the start/
// rescue/map-limit settings, then loads every map. sizeof is 0x44 (pinned by
// the `operator new` size in Mainform's FormCreate).
class MapContainer
{
  public:
    vector<MapItem> maps;     // +0x00
    unsigned short start_map; // +0x20
    int start_x;              // +0x24
    int start_y;              // +0x28
    short rescue_map;         // +0x2c
    int rescue_x;             // +0x30
    int rescue_y;             // +0x34
    unsigned short max_maps;  // +0x38
    char memory_map;          // +0x3a
    char pad_0x3b;            // +0x3b
    Settings *settings;       // +0x3c
    void *encode_scratch;     // +0x40

    MapContainer(Settings *settings);
    ~MapContainer();

    static void Mapcontrol_LoadMaps(MapContainer *self);
    static bool Mapcontrol_LoadMap(MapContainer *self, int map_id);
    static void Mapcontrol_IncPlayerCount(MapContainer *self, int map_id);
    static void Mapcontrol_DecPlayerCount(MapContainer *self, int map_id);
    static void Mapcontrol_SetArenaBlock(MapContainer *self, int map_id, int block);
    static void
    Mapcontrol_SetTileBits(MapContainer *self, MapItem *map, int x, int y, int code);
    static int
    Mapcontrol_CountNpcsChasingPlayer(MapContainer *self, int map_id, int player_id);
    static bool Mapcontrol_AggroChildNpcs(MapContainer *self, int map_id);
    static bool Mapcontrol_KillChildNpcs(MapContainer *self, int map_id);
    static void
    Mapcontrol_AddTileSpec(MapContainer *self, MapItem *map, int x, int y, int spec);
    static void Mapcontrol_AddWarp(MapContainer *self,
                                   MapItem *map,
                                   int x,
                                   int y,
                                   int dest_map,
                                   int level,
                                   int dest_x,
                                   int dest_y);
    static void Mapcontrol_AddLockKey(
        MapContainer *self, MapItem *map, unsigned int x, unsigned int y, int key_id);
    static void Mapcontrol_GetOrCreateChest(MapContainer *self,
                                            MapItem *map,
                                            unsigned int x,
                                            unsigned int y);
    static unsigned char
    Mapcontrol_ToggleDoor(MapContainer *self, int map_id, unsigned int x, unsigned int y);
    static int Mapcontrol_GetWarpDoorAt(MapContainer *self, int map_id, MapCoord coords);
    static int
    Mapcontrol_GetChestSlotCount(MapContainer *self, int map_id, MapCoord coords);
    static void Mapcontrol_AddChestSpawn(MapContainer *self,
                                         MapItem *map,
                                         unsigned int x,
                                         unsigned int y,
                                         int key_id,
                                         int slot,
                                         int item_id,
                                         int spawn_time,
                                         int amount);
    static void Mapcontrol_AddChestItem(
        MapContainer *self, int map_id, MapCoord coords, int item_id, int amount);
    static ItemStack Mapcontrol_TakeChestItem(MapContainer *self,
                                              int map_id,
                                              MapCoord coords,
                                              int item_id);
    static int Mapcontrol_AddGroundItem(MapContainer *self,
                                        int map_id,
                                        unsigned int item_id,
                                        int x,
                                        int y,
                                        unsigned int amount,
                                        int owner_player_id,
                                        unsigned short protect_ticks);
    static void Mapcontrol_PurgeGroundItemsInRange(MapContainer *self,
                                                   int map_id,
                                                   int range_low,
                                                   int range_high);
    static int DecodeNumber(MapContainer *self, String value);
    static String EncodeNumber(MapContainer *self, unsigned int value, int width);
    static int Mapcontrol_GetWarpMap(MapContainer *self, int map_id, int x, int y);
    static int Mapcontrol_GetWarpLevelReq(MapContainer *self, int map_id, int x, int y);
    static int Mapcontrol_GetWarpX(MapContainer *self, int map_id, int x, int y);
    static int Mapcontrol_GetWarpY(MapContainer *self, int map_id, int x, int y);
    static unsigned int
    Mapcontrol_GetTileSpec(MapContainer *self, int map_id, int x, int y);
    static bool Mapcontrol_IsOccupied(MapContainer *self, int map_id, int x, int y);
    static bool Mapcontrol_IsTileClear(MapContainer *self, int map_id, int x, int y);
    static bool Mapcontrol_IsTileWalkable(MapContainer *self, int map_id, int x, int y);
    static int Mapcontrol_GetWalkableStatus(
        MapContainer *self, int map_id, int x, int y, char ignore_spec_block);
};

int Mapcontrol_GetChestKeyAt(MapContainer *self, int map_id, MapCoord coords);
String Mapcontrol_BuildChestItemsString(MapContainer *self, int map_id, MapCoord coords);
char Mapcontrol_TryTakeQuestCooldown(MapContainer *self, int map_id);
char Mapcontrol_GetCanScroll(MapContainer *self, int map_id);
MapCoord Mapcontrol_GetRelogCoords(MapContainer *self, int map_id);
unsigned int
Mapcontrol_GetNpcIdByIndex(MapContainer *self, int map_id, unsigned int npc_index);
MapCoord
Mapcontrol_GetNpcCoordsByIndex(MapContainer *self, int map_id, unsigned int npc_index);
char Mapcontrol_IsDropTileClear(MapContainer *self, int map_id, int x, int y);
unsigned int Mapcontrol_GetTileSpecValueAt(MapContainer *self,
                                           int map_id,
                                           unsigned int x,
                                           unsigned int y);
int Mapcontrol_CountWalkableNeighbors(MapContainer *self,
                                      int map_id,
                                      unsigned int x,
                                      unsigned int y);
bool Mapcontrol_CanDropItemAt(
    MapContainer *self, int map_id, int x, int y, int player_id);
GroundItemInfo
Mapcontrol_TakeGroundItemInfo(MapContainer *self, int map_id, int index, int player_id);
void Mapcontrol_RemoveGroundItem(MapContainer *self, int map_id, int index);
char Mapcontrol_ReloadMap(MapContainer *self, int map_id);
void Mapcontrol_AddArenaSpawn(
    MapContainer *self, int map_id, int from_x, int from_y, int to_x, int to_y);
String Mapcontrol_ReadRawFile(MapContainer *self, int map_id);
MapObject Mapcontrol_GetTileSpecObject(MapContainer *self, int map_id, int x, int y);

#endif
