#include <vcl.h>
#pragma hdrstop

#include "Npc.h"
int RandRange(int max);

#pragma package(smart_init)

Npc::Npc(int npc_index,
         short npc_id,
         short x,
         short y,
         short attack_dir,
         int spawn_type,
         short spawn_time)
{
    if (spawn_type >= 7)
        act_ticks = 48;
    if (spawn_type == 6)
        act_ticks = 24;
    if (spawn_type == 5)
        act_ticks = 12;
    if (spawn_type == 4)
        act_ticks = 6;
    if (spawn_type == 3)
        act_ticks = 3;
    if (spawn_type == 2)
        act_ticks = 2;
    if (spawn_type == 1)
        act_ticks = 1;
    if (spawn_type == 0)
        act_ticks = 1;

    index = npc_index;
    id = npc_id;
    this->x = x;
    this->y = y;
    direction = RandRange(4);
    pos_pending = 0;
    talk_pending = 0;
    death_ticks = act_ticks;
    this->spawn_time = spawn_time;
    wSpawn_type = spawn_time;
    TDateTime now = Now();
    nDeath_ms = DateTimeToTimeStamp(now);
    wSpawn_x = x;
    wSpawn_y = y;
    nAct_counter = 4;
    nMove_cooldown = 1;
    nAttack_dir = attack_dir;
    alive = false;
    aggressive = false;
    in_combat = false;
    chase_target_id = -1;
}

Npc::~Npc()
{
}
