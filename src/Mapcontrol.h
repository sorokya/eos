#ifndef MapcontrolH
#define MapcontrolH

#include <Classes.hpp>
#include <vector>

#include "Map.h"

class Settings;

struct MapCoord
{
    struct
    {
        int x;
        int y;
    };
    MapCoord()
    {
    }
};

struct ItemStack
{
    struct
    {
        int id;
        int amount;
    };
    ItemStack()
    {
    }
};

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

// Layout recovered from the reference constructor (Mapcontrol unit,
// 0x47ad3c..0x487d38). The constructor runs the std::vector<MapContainer>
// default constructor first (member auto-init), then allocates the 8-byte
// encode scratch buffer, stores the Settings pointer and copies the start/
// rescue/map-limit settings, then loads every map. sizeof is 0x44 (pinned by
// the `operator new` size in Mainform's FormCreate).
class Mapcontrol
{
  public:
    std::vector<MapContainer> maps; // +0x00
    unsigned short start_map;       // +0x20
    int start_x;                    // +0x24
    int start_y;                    // +0x28
    short rescue_map;               // +0x2c
    int rescue_x;                   // +0x30
    int rescue_y;                   // +0x34
    unsigned short max_maps;        // +0x38
    char memory_map;                // +0x3a
    char pad_0x3b;                  // +0x3b
    Settings *settings;             // +0x3c
    void *encode_scratch;           // +0x40

    Mapcontrol(Settings *settings);
    ~Mapcontrol();

    static void Mapcontrol_LoadMaps(Mapcontrol *self);
    static bool Mapcontrol_LoadMap(Mapcontrol *self, int map_id);
    static void Mapcontrol_IncPlayerCount(Mapcontrol *self, int map_id);
    static void Mapcontrol_DecPlayerCount(Mapcontrol *self, int map_id);
    static void Mapcontrol_SetArenaBlock(Mapcontrol *self, int map_id, int block);
    static void
    Mapcontrol_SetTileBits(Mapcontrol *self, MapContainer *map, int x, int y, int code);
    static int
    Mapcontrol_CountNpcsChasingPlayer(Mapcontrol *self, int map_id, int player_id);
    static bool Mapcontrol_AggroChildNpcs(Mapcontrol *self, int map_id);
    static bool Mapcontrol_KillChildNpcs(Mapcontrol *self, int map_id);
    static void
    Mapcontrol_AddTileSpec(Mapcontrol *self, MapContainer *map, int x, int y, int spec);
    static void Mapcontrol_AddWarp(Mapcontrol *self,
                                   MapContainer *map,
                                   int x,
                                   int y,
                                   int dest_map,
                                   int level,
                                   int dest_x,
                                   int dest_y);
    static void Mapcontrol_AddLockKey(
        Mapcontrol *self, MapContainer *map, unsigned int x, unsigned int y, int key_id);
    static void Mapcontrol_GetOrCreateChest(Mapcontrol *self,
                                            MapContainer *map,
                                            unsigned int x,
                                            unsigned int y);
    static unsigned char
    Mapcontrol_ToggleDoor(Mapcontrol *self, int map_id, unsigned int x, unsigned int y);
    static MapItem *Mapcontrol_GetSlot(std::vector<MapItem> *slot_list, int slot);
    static int Mapcontrol_GetWarpDoorAt(Mapcontrol *self, int map_id, MapCoord coords);
    static int
    Mapcontrol_GetChestSlotCount(Mapcontrol *self, int map_id, MapCoord coords);
    static void Mapcontrol_AddChestSpawn(Mapcontrol *self,
                                         MapContainer *map,
                                         unsigned int x,
                                         unsigned int y,
                                         int key_id,
                                         int slot,
                                         int item_id,
                                         int spawn_time,
                                         int amount);
    static void Mapcontrol_AddChestItem(
        Mapcontrol *self, int map_id, MapCoord coords, int item_id, int amount);
    static ItemStack
    Mapcontrol_TakeChestItem(Mapcontrol *self, int map_id, MapCoord coords, int item_id);
    static int Mapcontrol_AddGroundItem(Mapcontrol *self,
                                        int map_id,
                                        unsigned int item_id,
                                        int x,
                                        int y,
                                        unsigned int amount,
                                        int owner_player_id,
                                        unsigned short protect_ticks);
    static void Mapcontrol_PurgeGroundItemsInRange(Mapcontrol *self,
                                                   int map_id,
                                                   int range_low,
                                                   int range_high);
    static int DecodeNumber(Mapcontrol *self, String value);
    static String EncodeNumber(Mapcontrol *self, unsigned int value, int width);
    static int Mapcontrol_GetWarpMap(Mapcontrol *self, int map_id, int x, int y);
    static int Mapcontrol_GetWarpLevelReq(Mapcontrol *self, int map_id, int x, int y);
    static int Mapcontrol_GetWarpX(Mapcontrol *self, int map_id, int x, int y);
    static int Mapcontrol_GetWarpY(Mapcontrol *self, int map_id, int x, int y);
    static unsigned int Map_GetTileSpec(Mapcontrol *self, int map_id, int x, int y);
    static bool Mapcontrol_IsOccupied(Mapcontrol *self, int map_id, int x, int y);
    static bool Mapcontrol_IsTileClear(Mapcontrol *self, int map_id, int x, int y);
    static bool Mapcontrol_IsTileWalkable(Mapcontrol *self, int map_id, int x, int y);
    static int Mapcontrol_IsWalkableNPC(
        Mapcontrol *self, int map_id, int x, int y, char ignore_spec_block);
};

int Mapcontrol_GetChestKeyAt(Mapcontrol *self, int map_id, MapCoord coords);
String Mapcontrol_BuildChestItemsString(Mapcontrol *self, int map_id, MapCoord coords);
char Mapcontrol_TryTakeQuestCooldown(Mapcontrol *self, int map_id);
char Mapcontrol_GetCanScroll(Mapcontrol *self, int map_id);
MapCoord Mapcontrol_GetRelogCoords(Mapcontrol *self, int map_id);
unsigned int
Mapcontrol_GetNpcIdByIndex(Mapcontrol *self, int map_id, unsigned int npc_index);
MapCoord
Mapcontrol_GetNpcCoordsByIndex(Mapcontrol *self, int map_id, unsigned int npc_index);
char Mapcontrol_IsDropTileClear(Mapcontrol *self, int map_id, int x, int y);
unsigned int Mapcontrol_GetTileSpecValueAt(Mapcontrol *self,
                                           int map_id,
                                           unsigned int x,
                                           unsigned int y);
int Mapcontrol_CountBlockedNeighbors(Mapcontrol *self,
                                     int map_id,
                                     unsigned int x,
                                     unsigned int y);
bool Mapcontrol_CanDropItemAt(Mapcontrol *self, int map_id, int x, int y, int player_id);
GroundItemInfo
Mapcontrol_TakeGroundItemInfo(Mapcontrol *self, int map_id, int index, int player_id);
void Mapcontrol_RemoveGroundItem(Mapcontrol *self, int map_id, int index);
char Mapcontrol_ReloadMap(Mapcontrol *self, int map_id);
void Mapcontrol_AddArenaSpawn(
    Mapcontrol *self, int map_id, int from_x, int from_y, int to_x, int to_y);
String Mapcontrol_ReadRawFile(Mapcontrol *self, int map_id);
MapObject Mapcontrol_GetTileSpecObject(Mapcontrol *self, int map_id, int x, int y);

#endif
