#include <vcl.h>
#pragma hdrstop

#include "Npccontrol.h"
#include "Player.h"
#include "Mapcontrol.h"
#include "Protocol.h"
#include "Gamecontrol.h"
#include "Mainform.h"

#pragma package(smart_init)

extern TGUI **MAINFORM;

Player **Players_Iter_Begin(Players *players);
Player **Players_Iter_End(Players *players);

NpcController::NpcController(Mapcontrol *map,
                             Players *players,
                             Server *server,
                             Settings *settings)
{
    encode_scratch = operator new(8);
    map_control = map;
    this->settings = settings;
    this->players = players;
    this->server = server;
    act_counter = 0;
    talk_counter = 0;
    regen_counter = 0;
}

NpcController::~NpcController()
{
}

int NpcController::Npc_GetDistance(NpcController *self, Npc *npc, Player *player)
{
    int distance = 0;
    if (npc->y > player->y)
        distance += npc->y - player->y;
    else
        distance += player->y - npc->y;
    if (npc->x > player->x)
        distance += npc->x - player->x;
    else
        distance += player->x - npc->x;
    return distance;
}

bool NpcController::Npc_IsWithinRange(NpcController *self, int x1, int y1, int x2, int y2)
{
    bool result = false;
    int dx = x2 - x1;
    int dy = y2 - y1;
    if (dx < 0)
        dx = 0 - dx;
    if (dy < 0)
        dy = 0 - dy;
    if (x2 > x1 && y2 > y1)
    {
        if (dx + dy <= 15)
            result = true;
    }
    else if (dx + dy <= 12)
    {
        result = true;
    }
    return result;
}

bool NpcController::Npc_DoMove(NpcController *self, int map_id, int x, int y)
{
    if (self->player_targets_valid == 0)
    {
        self->player_targets.clear();
        for (Player **it = Players_Iter_Begin(self->players);
             it != Players_Iter_End(self->players);
             it++)
        {
            if ((*it)->logged_in && (*it)->map_id == map_id)
                self->player_targets.insert(self->player_targets.end(), *it);
        }
        self->player_targets_valid = 1;
    }
    for (Player **it = self->player_targets.begin(); it != self->player_targets.end();
         it++)
    {
        if ((*it)->x == x && (*it)->y == y && (*it)->map_id == map_id)
            return true;
    }
    return false;
}

void NpcController::Npc_Wander(
    NpcController *self, Npc *npc, int map_id, int map_w, int map_h)
{
    if ((unsigned short)npc->nMove_cooldown < 1 ||
        (unsigned short)npc->nMove_cooldown > 10)
    {
        npc->nAttack_dir = RandRange(5);
        npc->nMove_cooldown = RandRange(3) + 2;
    }
    if (npc->nAttack_dir == Direction_Down)
    {
        if (npc->y < map_h)
        {
            if (Mapcontrol::Map_IsWalkableNPC(
                    self->map_control, map_id, npc->x, npc->y + 1, 0) == 0)
            {
                if (!Mapcontrol::Map_IsOccupied(
                        self->map_control, map_id, npc->x, npc->y + 1))
                {
                    if (!Npc_DoMove(self, map_id, npc->x, npc->y + 1))
                    {
                        npc->direction = npc->nAttack_dir;
                        npc->pos_pending = 1;
                        npc->y = npc->y + 1;
                        return;
                    }
                }
            }
        }
    }
    else if (npc->nAttack_dir == Direction_Left)
    {
        if (npc->x >= 1)
        {
            if (Mapcontrol::Map_IsWalkableNPC(
                    self->map_control, map_id, npc->x - 1, npc->y, 0) == 0)
            {
                if (!Mapcontrol::Map_IsOccupied(
                        self->map_control, map_id, npc->x - 1, npc->y))
                {
                    if (!Npc_DoMove(self, map_id, npc->x - 1, npc->y))
                    {
                        npc->direction = npc->nAttack_dir;
                        npc->pos_pending = 1;
                        npc->x--;
                        return;
                    }
                }
            }
        }
    }
    else if (npc->nAttack_dir == Direction_Up)
    {
        if (npc->y >= 1)
        {
            if (Mapcontrol::Map_IsWalkableNPC(
                    self->map_control, map_id, npc->x, npc->y - 1, 0) == 0)
            {
                if (!Mapcontrol::Map_IsOccupied(
                        self->map_control, map_id, npc->x, npc->y - 1))
                {
                    if (!Npc_DoMove(self, map_id, npc->x, npc->y - 1))
                    {
                        npc->direction = npc->nAttack_dir;
                        npc->pos_pending = 1;
                        npc->y--;
                        return;
                    }
                }
            }
        }
    }
    else if (npc->nAttack_dir == Direction_Right && npc->x < map_w)
    {
        if (Mapcontrol::Map_IsWalkableNPC(
                self->map_control, map_id, npc->x + 1, npc->y, 0) == 0)
        {
            if (!Mapcontrol::Map_IsOccupied(
                    self->map_control, map_id, npc->x + 1, npc->y))
            {
                if (!Npc_DoMove(self, map_id, npc->x + 1, npc->y))
                {
                    npc->direction = npc->nAttack_dir;
                    npc->pos_pending = 1;
                    npc->x = npc->x + 1;
                }
            }
        }
    }
}

int NpcController::Npc_ValidateMove(NpcController *self, int map_id, int x, int y)
{
    if (self->player_targets_valid == 0)
    {
        self->player_targets.clear();
        for (Player **it = Players_Iter_Begin(self->players);
             it != Players_Iter_End(self->players);
             it++)
        {
            if ((*it)->logged_in && (*it)->map_id == map_id)
                self->player_targets.insert(self->player_targets.end(), *it);
        }
        self->player_targets_valid = 1;
    }
    for (Player **it = self->player_targets.begin(); it != self->player_targets.end();
         it++)
    {
        if ((*it)->x == x && (*it)->y == y && (*it)->map_id == map_id)
            return (*it)->player_id;
    }
    return -1;
}

String
NpcController::Packet_AppendEncoded(NpcController *context, unsigned int value, int width)
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
                ((char *)context->encode_scratch)[i] = c;
                value = quotient;
                if (quotient < 1)
                    flag = false;
                else if (i + 1 == width)
                    width++;
            }
            else
            {
                char pad = 0xfe;
                ((char *)context->encode_scratch)[i] = pad;
            }
        }
    }
    catch (...)
    {
        width = 0;
    }
    String encoded((char *)context->encode_scratch, width);
    return encoded;
}

bool NpcController::Npc_AttackPlayer(NpcController *mc, Npc *npc, Player *player)
{
    int accuracy = Gamecontrol::Combat_CalcArmorPen(
        (*MAINFORM)->game_control, npc->accuracy, player->evasion, 0.9);
    int damage = 0;
    if (player->on_chair != false || player->sitting != false)
        accuracy = 100;
    if (RandRange(100) < accuracy)
    {
        accuracy =
            Gamecontrol::Combat_CalcArmorPen((*MAINFORM)->game_control,
                                             (npc->min_damage + npc->max_damage) / 2,
                                             player->armor,
                                             0.8);
        double d = npc->min_damage;
        if (d < 1.0)
            d = 1.0;
        d = 0.01L * d;
        d = accuracy * d;
        d = RandRange(npc->max_damage - npc->min_damage + 2) + d;
        damage = (int)d;
        if (damage < 1)
            damage = 1;
    }
    if (npc->element_weakness > 0)
    {
        MapCoord element;
        element.x = npc->element_weakness;
        element.y = 0;
        if (element.x == 1)
            damage = (int)(damage * Gamecontrol::Combat_CalcElementMult(
                                        (*MAINFORM)->game_control,
                                        element,
                                        npc->element_weakness_damage_table[0],
                                        player->element_resistances[2]));
        if (element.x == 2)
            damage = (int)(damage * Gamecontrol::Combat_CalcElementMult(
                                        (*MAINFORM)->game_control,
                                        element,
                                        npc->element_weakness_damage_table[1],
                                        player->element_resistances[1]));
        if (element.x == 3)
            damage = (int)(damage * Gamecontrol::Combat_CalcElementMult(
                                        (*MAINFORM)->game_control,
                                        element,
                                        npc->element_weakness_damage_table[2],
                                        player->element_resistances[6]));
        if (element.x == 4)
            damage = (int)(damage * Gamecontrol::Combat_CalcElementMult(
                                        (*MAINFORM)->game_control,
                                        element,
                                        npc->element_weakness_damage_table[3],
                                        player->element_resistances[3]));
        if (element.x == 5)
            damage = (int)(damage * Gamecontrol::Combat_CalcElementMult(
                                        (*MAINFORM)->game_control,
                                        element,
                                        npc->element_weakness_damage_table[4],
                                        player->element_resistances[4]));
        if (element.x == 6)
            damage = (int)(damage * Gamecontrol::Combat_CalcElementMult(
                                        (*MAINFORM)->game_control,
                                        element,
                                        npc->element_weakness_damage_table[5],
                                        player->element_resistances[5]));
    }
    if ((unsigned short)npc->nAttack_dir == player->direction)
        damage = damage + damage / 2;
    if (npc->x == player->x)
    {
        if (player->y < npc->y)
            npc->nAttack_dir = 2;
        if (player->y > npc->y)
            npc->nAttack_dir = 0;
    }
    else
    {
        if (player->x < npc->x)
            npc->nAttack_dir = 1;
        if (player->x > npc->x)
            npc->nAttack_dir = 3;
    }
    player->hp = player->hp - damage;
    if (player->hp <= 0)
    {
        player->dead = true;
        player->hp = 0;
    }
    int hp_percent = (player->hp * 100) / player->max_hp;
    player->stats_dirty = 1;
    npc->attack_buffer = Packet_AppendEncoded(mc, npc->index, 1);
    if (player->hp > 0)
        npc->attack_buffer.Insert(Packet_AppendEncoded(mc, 1, 1),
                                  npc->attack_buffer.Length() + 1);
    if (player->hp < 1)
        npc->attack_buffer.Insert(Packet_AppendEncoded(mc, 2, 1),
                                  npc->attack_buffer.Length() + 1);
    npc->attack_buffer.Insert(
        Packet_AppendEncoded(mc, (unsigned short)npc->nAttack_dir, 1),
        npc->attack_buffer.Length() + 1);
    npc->attack_buffer.Insert(Packet_AppendEncoded(mc, player->player_id, 2),
                              npc->attack_buffer.Length() + 1);
    npc->attack_buffer.Insert(Packet_AppendEncoded(mc, damage, 3),
                              npc->attack_buffer.Length() + 1);
    npc->attack_buffer.Insert(Packet_AppendEncoded(mc, hp_percent, 1),
                              npc->attack_buffer.Length() + 1);
    if (player->hp == 0)
        return true;
    return false;
}

void NpcController::Npc_ChaseTarget(
    NpcController *mc, Npc *npc, Player *player, int map_id, int map_w, int map_h)
{
    int dir;
    if (player->x < npc->x)
    {
        if (player->y == npc->y)
            dir = 1;
        if (player->y < npc->y)
        {
            if (npc->y - player->y < npc->x - player->x)
                dir = 1;
            else
                dir = 2;
        }
        if (player->y > npc->y)
        {
            if (player->y - npc->y < npc->x - player->x)
                dir = 1;
            else
                dir = 0;
        }
    }
    if (player->x > npc->x)
    {
        if (player->y == npc->y)
            dir = 3;
        if (player->y < npc->y)
        {
            if (npc->y - player->y < player->x - npc->x)
                dir = 3;
            else
                dir = 2;
        }
        if (player->y > npc->y)
        {
            if (player->y - npc->y < player->x - npc->x)
                dir = 3;
            else
                dir = 0;
        }
    }
    if (player->x == npc->x)
    {
        dir = 2;
        if (player->y > npc->y)
            dir = 0;
    }
    int last_dir = dir;
    int attempt = 0;
    while (attempt < 4)
    {
        if (attempt == 3)
        {
            if (dir == 0 || dir == 2)
            {
                if (last_dir == 1)
                    dir = 3;
                else
                    dir = 1;
            }
            else if (last_dir == 2)
                dir = 0;
            else
                dir = 2;
        }
        if (attempt == 2)
        {
            npc->nStuck_pos = npc->x;
            *(int *)&npc->pad_90 = npc->y;
            dir = dir + 2;
            if (dir > 3)
                dir = dir - 4;
        }
        if (attempt == 1)
        {
            if (dir == 0 || dir == 2)
            {
                if (player->x < npc->x)
                    dir = 1;
                if (player->x > npc->x)
                    dir = 3;
                if (player->x == npc->x)
                {
                    if (npc->direction == 1)
                        dir = 1;
                    else
                        dir = 3;
                }
            }
            else
            {
                if (player->y < npc->y)
                    dir = 2;
                if (player->y > npc->y)
                    dir = 0;
                if (player->y == npc->y)
                {
                    if (npc->direction == 0)
                        dir = 0;
                    else
                        dir = 2;
                }
            }
        }
        attempt++;
        if (dir == 0)
        {
            if (npc->y < map_h)
            {
                if (npc->x != npc->nStuck_pos || npc->y + 1 != *(int *)&npc->pad_90)
                {
                    if (Mapcontrol::Map_IsWalkableNPC(
                            mc->map_control, map_id, npc->x, npc->y + 1, 0) == 0)
                    {
                        if (!Mapcontrol::Map_IsOccupied(
                                mc->map_control, map_id, npc->x, npc->y + 1))
                        {
                            int player_id =
                                Npc_ValidateMove(mc, map_id, npc->x, npc->y + 1);
                            if (player_id > 0)
                            {
                                if (attempt > 1)
                                {
                                    npc->target_player_id = player_id;
                                    return;
                                }
                                continue;
                            }
                            else
                            {
                                if (attempt == 1)
                                    npc->nStuck_pos = -1;
                                npc->target_player_id = -1;
                                npc->direction = dir;
                                npc->pos_pending = 1;
                                npc->y = npc->y + 1;
                                return;
                            }
                        }
                    }
                }
            }
        }
        else if (dir == 1)
        {
            if (npc->x >= 1)
            {
                if (npc->x - 1 != npc->nStuck_pos || npc->y != *(int *)&npc->pad_90)
                {
                    if (Mapcontrol::Map_IsWalkableNPC(
                            mc->map_control, map_id, npc->x - 1, npc->y, 0) == 0)
                    {
                        if (!Mapcontrol::Map_IsOccupied(
                                mc->map_control, map_id, npc->x - 1, npc->y))
                        {
                            int player_id =
                                Npc_ValidateMove(mc, map_id, npc->x - 1, npc->y);
                            if (player_id > 0)
                            {
                                if (attempt > 1)
                                {
                                    npc->target_player_id = player_id;
                                    return;
                                }
                                continue;
                            }
                            else
                            {
                                if (attempt == 1)
                                    npc->nStuck_pos = -1;
                                npc->target_player_id = -1;
                                npc->direction = dir;
                                npc->pos_pending = 1;
                                npc->x--;
                                return;
                            }
                        }
                    }
                }
            }
        }
        else if (dir == 2)
        {
            if (npc->y >= 1)
            {
                if (npc->x != npc->nStuck_pos || npc->y - 1 != *(int *)&npc->pad_90)
                {
                    if (Mapcontrol::Map_IsWalkableNPC(
                            mc->map_control, map_id, npc->x, npc->y - 1, 0) == 0)
                    {
                        if (!Mapcontrol::Map_IsOccupied(
                                mc->map_control, map_id, npc->x, npc->y - 1))
                        {
                            int player_id =
                                Npc_ValidateMove(mc, map_id, npc->x, npc->y - 1);
                            if (player_id > 0)
                            {
                                if (attempt > 1)
                                {
                                    npc->target_player_id = player_id;
                                    return;
                                }
                                continue;
                            }
                            else
                            {
                                if (attempt == 1)
                                    npc->nStuck_pos = -1;
                                npc->target_player_id = -1;
                                npc->direction = dir;
                                npc->pos_pending = 1;
                                npc->y--;
                                return;
                            }
                        }
                    }
                }
            }
        }
        else if (dir == 3)
        {
            if (npc->x < map_w)
            {
                if (npc->x + 1 != npc->nStuck_pos || npc->y != *(int *)&npc->pad_90)
                {
                    if (Mapcontrol::Map_IsWalkableNPC(
                            mc->map_control, map_id, npc->x + 1, npc->y, 0) == 0)
                    {
                        if (!Mapcontrol::Map_IsOccupied(
                                mc->map_control, map_id, npc->x + 1, npc->y))
                        {
                            int player_id =
                                Npc_ValidateMove(mc, map_id, npc->x + 1, npc->y);
                            if (player_id > 0)
                            {
                                if (attempt > 1)
                                {
                                    npc->target_player_id = player_id;
                                    return;
                                }
                                continue;
                            }
                            else
                            {
                                if (attempt == 1)
                                    npc->nStuck_pos = -1;
                                npc->target_player_id = -1;
                                npc->direction = dir;
                                npc->pos_pending = 1;
                                npc->x = npc->x + 1;
                                return;
                            }
                        }
                    }
                }
            }
        }
    }
}

// BEGIN GENERATED STUBS (scripts/genstubs.py)
#pragma warn - 8057
// STUB(0x004ae45c, 6064 bytes) NpcControl_Tick - ref: void NpcControl_Tick(Npccontrol *
// npc_control)
void NpcControl_Tick_Stub(void *a0)
{
}
// STUB(0x004afd20, 1648 bytes) Npc_AttackPlayer - ref: void Npc_AttackPlayer(Npccontrol *
// mc, Npc * npc, Player * player)
void Npc_AttackPlayer_Stub(void *a0, void *a1, void *a2)
{
}
// STUB(0x004b06b0, 1942 bytes) Npc_ChaseTarget - ref: void Npc_ChaseTarget(Npccontrol *
// mc, Npc * npc, Player * player, int map_id, int map_w, int map_h)
void Npc_ChaseTarget_Stub(void *a0, void *a1, void *a2, int a3, int a4, int a5)
{
}
#pragma warn.8057
// END GENERATED STUBS
