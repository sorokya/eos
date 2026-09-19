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
    char pad_3b;                    // +0x3b
    Settings *settings;             // +0x3c
    void *encode_scratch;           // +0x40

    Mapcontrol(Settings *settings);
    ~Mapcontrol();

    static void Mapcontrol_LoadMaps(Mapcontrol *map_control);
    static bool Mapcontrol_LoadMap(Mapcontrol *map_control, int map_id);
    static void Mapcontrol_inc_player_count(Mapcontrol *map_control, int map_id);
    static void Mapcontrol_dec_player_count(Mapcontrol *map_control, int map_id);
    static void Mapcontrol_SetArenaBlock(Mapcontrol *map_control, int map_id, int block);
    static void Mapcontrol_SetTileBits(
        Mapcontrol *map_control, MapContainer *map, int x, int y, int code);
    static int
    Mapcontrol_CountNpcsChasingPlayer(Mapcontrol *map_control, int map_id, int player_id);
    static bool Mapcontrol_AggroChildNpcs(Mapcontrol *map_control, int map_id);
    static bool Mapcontrol_KillChildNpcs(Mapcontrol *map_control, int map_id);
    static void Mapcontrol_AddTileSpec(
        Mapcontrol *map_control, MapContainer *map, int x, int y, int spec);
    static void Mapcontrol_AddWarp(Mapcontrol *map_control,
                                   MapContainer *map,
                                   int x,
                                   int y,
                                   int dest_map,
                                   int level,
                                   int dest_x,
                                   int dest_y);
    static void Mapcontrol_AddLockKey(Mapcontrol *map_control,
                                      MapContainer *map,
                                      unsigned int x,
                                      unsigned int y,
                                      int key_id);
    static void Mapcontrol_GetOrCreateChest(Mapcontrol *map_control,
                                            MapContainer *map,
                                            unsigned int x,
                                            unsigned int y);
    static unsigned char Mapcontrol_ToggleDoor(Mapcontrol *map_control,
                                               int map_id,
                                               unsigned int x,
                                               unsigned int y);
    static MapItem *Itemchest_GetSlot(std::vector<MapItem> *slot_list, int slot);
    static int Map_GetWarpDoorAt(Mapcontrol *map_control, int map_id, MapCoord coords);
    static int
    Mapcontrol_GetChestSlotCount(Mapcontrol *map_control, int map_id, MapCoord coords);
    static void Mapcontrol_AddChestSpawn(Mapcontrol *map_control,
                                         MapContainer *map,
                                         unsigned int x,
                                         unsigned int y,
                                         int key_id,
                                         int slot,
                                         int item_id,
                                         int spawn_time,
                                         int amount);
    static void Mapcontrol_AddChestItem(
        Mapcontrol *map_control, int map_id, MapCoord coords, int item_id, int amount);
    static ItemStack Mapcontrol_TakeChestItem(Mapcontrol *map_control,
                                              int map_id,
                                              MapCoord coords,
                                              int item_id);
    static int Mapcontrol_AddGroundItem(Mapcontrol *map_control,
                                        int map_id,
                                        unsigned int item_id,
                                        int x,
                                        int y,
                                        unsigned int amount,
                                        int owner_player_id,
                                        unsigned short protect_ticks);
    static void Mapcontrol_PurgeGroundItemsInRange(Mapcontrol *map_control,
                                                   int map_id,
                                                   int range_low,
                                                   int range_high);
    static int Pub_DecodeNumber_Map(Mapcontrol *map_control, String value);
    static String
    Mapcontrol_AppendEncoded(Mapcontrol *map_control, unsigned int value, int width);
    static int Map_GetWarpMap(Mapcontrol *map_control, int map_id, int x, int y);
    static int Map_GetWarpLevelReq(Mapcontrol *map_control, int map_id, int x, int y);
    static int Map_GetWarpX(Mapcontrol *map_control, int map_id, int x, int y);
    static int Map_GetWarpY(Mapcontrol *map_control, int map_id, int x, int y);
    static unsigned int
    Map_GetTileSpec(Mapcontrol *map_control, int map_id, int x, int y);
    static bool Map_IsOccupied(Mapcontrol *map_control, int map_id, int x, int y);
    static bool Map_IsTileClear(Mapcontrol *map_control, int map_id, int x, int y);
    static bool Map_IsTileWalkable(Mapcontrol *map_control, int map_id, int x, int y);
    static int Map_IsWalkableNPC(
        Mapcontrol *map_control, int map_id, int x, int y, char ignore_spec_block);
};

int FUN_00486e64(int map_control, int map_id, MapCoord coords);
String FUN_0047badc(Mapcontrol *map_control, int map_id, MapCoord coords);

#endif
