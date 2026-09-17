#ifndef NpccontrolH
#define NpccontrolH

#include <Classes.hpp>
#include <vector>
#include "Npc.h"

class Player;
class Mapcontrol;
class Players;
class Server;
class Settings;

// The NPC runtime driver. Layout (sizeof 0x44) is pinned by the reference
// constructor (0x4ae37c) stores and the destructor; the argument order is pinned
// by the FormCreate call site. player_targets is a std::vector<Player*> and its
// begin pointer lands at +0x1c (the compiler's vector layout puts _M_start at
// the object's +4).
class Npccontrol
{
  public:
    Settings *settings;      // +0x00
    Mapcontrol *map_control; // +0x04
    Players *players;        // +0x08
    Server *server;          // +0x0c
    void *encode_scratch;    // +0x10 (operator new(8), base-253 encode buffer)
    char flag_0x14;          // +0x14
    char field_0x15;         // +0x15
    char field_0x16;         // +0x16
    char field_0x17;         // +0x17
    std::vector<Player *> player_targets; // +0x18
    int act_counter;                      // +0x38
    int talk_counter;                     // +0x3c
    int regen_counter;                    // +0x40

    Npccontrol(Mapcontrol *map, Players *players, Server *server, Settings *settings);
    ~Npccontrol();

    static void NpcControl_Tick(Npccontrol *self);
    static int Npc_GetDistance(Npccontrol *self, Npc *npc, Player *player);
    static bool Npc_IsWithinRange(Npccontrol *self, int x1, int y1, int x2, int y2);
    static bool Npc_AttackPlayer(Npccontrol *self, Npc *npc, Player *player);
    static void Npc_Wander(Npccontrol *self, Npc *npc, int map_id, int map_w, int map_h);
    static void Npc_ChaseTarget(
        Npccontrol *self, Npc *npc, Player *player, int map_id, int map_w, int map_h);
    static bool Npc_DoMove(Npccontrol *self, int map_id, int x, int y);
    static int Npc_ValidateMove(Npccontrol *self, int map_id, int x, int y);
    static String
    Packet_AppendEncoded(Npccontrol *context, unsigned int value, int width);
};

#endif
