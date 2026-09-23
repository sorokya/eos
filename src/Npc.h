#ifndef NpcH
#define NpcH

#include <Classes.hpp>
#include <SysUtils.hpp>

// Layout recovered from the reference (Npc unit, 0x47a868..0x47aa2c). sizeof is
// 0x94. The constructor (0x47a868) constructs the three AnsiString buffers at
// +0x68/+0x70/+0x78, writes the act-rate tier to act_ticks, copies the spawn
// coordinates, stamps nDeath_ms with DateTimeToTimeStamp(Now()) and clears the
// pending/combat flags. The destructor (0x47aa2c) clears
// attack_buffer/talk_buffer/pos_buffer in that order; it is user-declared,
// because the reference body enters an EH frame (___InitExceptBlockLDTC) with a
// function-body scope of 8 and a member-destruction counter of 3, which the
// implicit destructor does not emit. Every offset is pinned by the constructor
// stores and the destructor.
struct Npc
{
    unsigned short index;                   // +0x00
    unsigned short id;                      // +0x02
    unsigned short x;                       // +0x04
    unsigned short y;                       // +0x06
    short direction;                        // +0x08
    short pad_0xa;                          // +0x0a
    int min_damage;                         // +0x0c
    int max_damage;                         // +0x10
    int armor;                              // +0x14
    int accuracy;                           // +0x18
    int evade;                              // +0x1c
    int hp;                                 // +0x20
    int max_hp;                             // +0x24
    int hp_regen;                           // +0x28
    short nHp_pct;                          // +0x2c
    short pad_0x2e;                         // +0x2e
    int element_weakness;                   // +0x30
    short pad_0x34;                         // +0x34
    short element_weakness_damage_table[6]; // +0x36
    bool alive;                             // +0x42
    int death_ticks;                        // +0x44
    TTimeStamp nDeath_ms;                   // +0x48
    unsigned short wSpawn_type;             // +0x50
    short spawn_time;                       // +0x52
    unsigned short wSpawn_x;                // +0x54
    unsigned short wSpawn_y;                // +0x56
    unsigned short wDrop_item_id;           // +0x58
    unsigned short wDrop_amount;            // +0x5a
    int act_ticks;                          // +0x5c
    short nAct_counter;                     // +0x60
    short nMove_cooldown;                   // +0x62
    short nAttack_dir;                      // +0x64
    char pos_pending;                       // +0x66
    String pos_buffer;                      // +0x68
    char talk_pending;                      // +0x6c
    String talk_buffer;                     // +0x70
    char attack_pending;                    // +0x74
    String attack_buffer;                   // +0x78
    short boss;                             // +0x7c
    short child;                            // +0x7e
    bool aggressive;                        // +0x80
    bool in_combat;                         // +0x81
    short nLeash_timer;                     // +0x82
    int target_player_id;                   // +0x84
    int chase_target_id;                    // +0x88
    int nStuck_pos;                         // +0x8c
    char nStuck_pos_y[4];                   // +0x90

    Npc(int npc_index,
        short npc_id,
        short x,
        short y,
        short attack_dir,
        int spawn_type,
        short spawn_time);
    ~Npc();
};

#endif
