#include <vcl.h>
#pragma hdrstop

#include "Npccontrol.h"
#include "Player.h"
#include "Mapcontrol.h"
#include "Protocol.h"
#include "Gamecontrol.h"
#include "Mainform.h"
#include "Npcvalues.h"
#include "Players.h"
#include "Packets.h"

#pragma package(smart_init)

NpcController::NpcController(MapContainer *map,
                             Players *players,
                             Packets *server,
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
        for (Player **it = self->players->players.begin();
             it != self->players->players.end();
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
            if (MapContainer::Mapcontrol_IsWalkableNPC(
                    self->map_control, map_id, npc->x, npc->y + 1, 0) == 0)
            {
                if (!MapContainer::Mapcontrol_IsOccupied(
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
            if (MapContainer::Mapcontrol_IsWalkableNPC(
                    self->map_control, map_id, npc->x - 1, npc->y, 0) == 0)
            {
                if (!MapContainer::Mapcontrol_IsOccupied(
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
            if (MapContainer::Mapcontrol_IsWalkableNPC(
                    self->map_control, map_id, npc->x, npc->y - 1, 0) == 0)
            {
                if (!MapContainer::Mapcontrol_IsOccupied(
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
        if (MapContainer::Mapcontrol_IsWalkableNPC(
                self->map_control, map_id, npc->x + 1, npc->y, 0) == 0)
        {
            if (!MapContainer::Mapcontrol_IsOccupied(
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
        for (Player **it = self->players->players.begin();
             it != self->players->players.end();
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

String NpcController::EncodeNumber(NpcController *self, unsigned int value, int width)
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
    String encoded((char *)self->encode_scratch, width);
    return encoded;
}

bool NpcController::Npc_AttackPlayer(NpcController *mc, Npc *npc, Player *player)
{
    int accuracy = Game::Combat_CalcArmorPen(
        (*MAINFORM)->game_control, npc->accuracy, player->evasion, 0.9);
    int damage = 0;
    if (player->on_chair != false || player->sitting != false)
        accuracy = 100;
    if (RandRange(100) < accuracy)
    {
        accuracy = Game::Combat_CalcArmorPen((*MAINFORM)->game_control,
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
            damage = (int)(damage * Game::Combat_CalcElementMult(
                                        (*MAINFORM)->game_control,
                                        element,
                                        npc->element_weakness_damage_table[0],
                                        player->element_resistances[2]));
        if (element.x == 2)
            damage = (int)(damage * Game::Combat_CalcElementMult(
                                        (*MAINFORM)->game_control,
                                        element,
                                        npc->element_weakness_damage_table[1],
                                        player->element_resistances[1]));
        if (element.x == 3)
            damage = (int)(damage * Game::Combat_CalcElementMult(
                                        (*MAINFORM)->game_control,
                                        element,
                                        npc->element_weakness_damage_table[2],
                                        player->element_resistances[6]));
        if (element.x == 4)
            damage = (int)(damage * Game::Combat_CalcElementMult(
                                        (*MAINFORM)->game_control,
                                        element,
                                        npc->element_weakness_damage_table[3],
                                        player->element_resistances[3]));
        if (element.x == 5)
            damage = (int)(damage * Game::Combat_CalcElementMult(
                                        (*MAINFORM)->game_control,
                                        element,
                                        npc->element_weakness_damage_table[4],
                                        player->element_resistances[4]));
        if (element.x == 6)
            damage = (int)(damage * Game::Combat_CalcElementMult(
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
    npc->attack_buffer = EncodeNumber(mc, npc->index, 1);
    if (player->hp > 0)
        npc->attack_buffer.Insert(EncodeNumber(mc, 1, 1),
                                  npc->attack_buffer.Length() + 1);
    if (player->hp < 1)
        npc->attack_buffer.Insert(EncodeNumber(mc, 2, 1),
                                  npc->attack_buffer.Length() + 1);
    npc->attack_buffer.Insert(EncodeNumber(mc, (unsigned short)npc->nAttack_dir, 1),
                              npc->attack_buffer.Length() + 1);
    npc->attack_buffer.Insert(EncodeNumber(mc, player->player_id, 2),
                              npc->attack_buffer.Length() + 1);
    npc->attack_buffer.Insert(EncodeNumber(mc, damage, 3),
                              npc->attack_buffer.Length() + 1);
    npc->attack_buffer.Insert(EncodeNumber(mc, hp_percent, 1),
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
            *(int *)&npc->pad_0x90 = npc->y;
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
                    if (npc->direction == Direction_Left)
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
                    if (npc->direction == Direction_Down)
                        dir = 0;
                    else
                        dir = 2;
                }
            }
        }
        attempt++;
        if (dir == 0)
        {
            if (npc->y >= map_h)
                continue;
            if (npc->x == npc->nStuck_pos && npc->y + 1 == *(int *)&npc->pad_0x90)
                continue;
            if (MapContainer::Mapcontrol_IsWalkableNPC(
                    mc->map_control, map_id, npc->x, npc->y + 1, 0) != 0)
                continue;
            if (MapContainer::Mapcontrol_IsOccupied(
                    mc->map_control, map_id, npc->x, npc->y + 1))
                continue;
            int player_id = Npc_ValidateMove(mc, map_id, npc->x, npc->y + 1);
            if (player_id > 0)
            {
                if (attempt > 1)
                {
                    npc->target_player_id = player_id;
                    return;
                }
                continue;
            }
            if (attempt == 1)
                npc->nStuck_pos = -1;
            npc->target_player_id = -1;
            npc->direction = dir;
            npc->pos_pending = 1;
            npc->y = npc->y + 1;
            return;
        }
        if (dir == 1)
        {
            if (npc->x < 1)
                continue;
            if (npc->x - 1 == npc->nStuck_pos && npc->y == *(int *)&npc->pad_0x90)
                continue;
            if (MapContainer::Mapcontrol_IsWalkableNPC(
                    mc->map_control, map_id, npc->x - 1, npc->y, 0) != 0)
                continue;
            if (MapContainer::Mapcontrol_IsOccupied(
                    mc->map_control, map_id, npc->x - 1, npc->y))
                continue;
            int player_id = Npc_ValidateMove(mc, map_id, npc->x - 1, npc->y);
            if (player_id > 0)
            {
                if (attempt > 1)
                {
                    npc->target_player_id = player_id;
                    return;
                }
                continue;
            }
            if (attempt == 1)
                npc->nStuck_pos = -1;
            npc->target_player_id = -1;
            npc->direction = dir;
            npc->pos_pending = 1;
            npc->x--;
            return;
        }
        if (dir == 2)
        {
            if (npc->y < 1)
                continue;
            if (npc->x == npc->nStuck_pos && npc->y - 1 == *(int *)&npc->pad_0x90)
                continue;
            if (MapContainer::Mapcontrol_IsWalkableNPC(
                    mc->map_control, map_id, npc->x, npc->y - 1, 0) != 0)
                continue;
            if (MapContainer::Mapcontrol_IsOccupied(
                    mc->map_control, map_id, npc->x, npc->y - 1))
                continue;
            int player_id = Npc_ValidateMove(mc, map_id, npc->x, npc->y - 1);
            if (player_id > 0)
            {
                if (attempt > 1)
                {
                    npc->target_player_id = player_id;
                    return;
                }
                continue;
            }
            if (attempt == 1)
                npc->nStuck_pos = -1;
            npc->target_player_id = -1;
            npc->direction = dir;
            npc->pos_pending = 1;
            npc->y--;
            return;
        }
        if (dir == 3)
        {
            if (npc->x >= map_w)
                continue;
            if (npc->x + 1 == npc->nStuck_pos && npc->y == *(int *)&npc->pad_0x90)
                continue;
            if (MapContainer::Mapcontrol_IsWalkableNPC(
                    mc->map_control, map_id, npc->x + 1, npc->y, 0) != 0)
                continue;
            if (MapContainer::Mapcontrol_IsOccupied(
                    mc->map_control, map_id, npc->x + 1, npc->y))
                continue;
            int player_id = Npc_ValidateMove(mc, map_id, npc->x + 1, npc->y);
            if (player_id > 0)
            {
                if (attempt > 1)
                {
                    npc->target_player_id = player_id;
                    return;
                }
                continue;
            }
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

void NpcController::NpcControl_Tick(NpcController *npc_control)
{
    bool flag = true;
    npc_control->act_counter++;
    npc_control->regen_counter++;
    for (ChestItem *map = npc_control->map_control->maps.begin();
         map != npc_control->map_control->maps.end();
         map++)
    {
        map->npc_dirty = 0;
        npc_control->player_targets_valid = 0;
        for (Npc **npc = (Npc **)map->npc_list.begin();
             npc != (Npc **)map->npc_list.end();
             npc++)
        {
            if ((*npc)->alive == false)
            {
                if (npc_control->act_counter % 6 == 0)
                {
                    TDateTime now = Now();
                    TTimeStamp stamp = DateTimeToTimeStamp(now);
                    int date_delta = stamp.Date - (*npc)->nDeath_ms.Date;
                    int time_delta = stamp.Time - (*npc)->nDeath_ms.Time;
                    int elapsed =
                        time_delta / MS_PER_SECOND + date_delta * SECONDS_PER_DAY;
                    if ((int)(unsigned short)(*npc)->spawn_time <= elapsed)
                    {
                        int sx = (*npc)->wSpawn_x;
                        int sy = (*npc)->wSpawn_y;
                        if (sx > 1 && sy > 1 && sx + 2 < map->width &&
                            sy + 2 < map->height)
                        {
                            sx = RandRange(5) + (*npc)->wSpawn_x - 2;
                            sy = RandRange(5) + (*npc)->wSpawn_y - 2;
                        }
                        if (!MapContainer::Mapcontrol_IsTileClear(
                                npc_control->map_control, map->rid, sx, sy))
                        {
                            (*npc)->spawn_time = (*npc)->spawn_time + 1;
                        }
                        else
                        {
                            if ((unsigned short)(*npc)->child > 0 &&
                                map->boss_alive == false)
                            {
                                (*npc)->spawn_time = (*npc)->spawn_time + 1;
                            }
                            else
                            {
                                (*npc)->alive = true;
                                (*npc)->spawn_time =
                                    RandRange(0x14) + (*npc)->wSpawn_type;
                                (*npc)->act_ticks = (*npc)->death_ticks;
                                (*npc)->x = sx;
                                (*npc)->y = sy;
                                (*npc)->pos_pending = 1;
                                map->npc_dirty = 1;
                                (*npc)->direction = RandRange(4);
                                (*npc)->nMove_cooldown = RandRange(3) + 2;
                                if ((unsigned short)(*npc)->boss > 0)
                                    map->boss_alive = true;
                                if (24 <= (*npc)->death_ticks)
                                {
                                    (*npc)->x = (*npc)->wSpawn_x;
                                    (*npc)->y = (*npc)->wSpawn_y;
                                    (*npc)->direction = (*npc)->wSpawn_type % 4;
                                }
                                (*npc)->aggressive = false;
                                (*npc)->nStuck_pos = -1;
                                (*npc)->target_player_id = -1;
                                (*npc)->chase_target_id = -1;
                                (*npc)->nLeash_timer = 0;
                                (*npc)->hp = NpcValues::GetMaxHp((*MAINFORM)->npc_values,
                                                                 (*npc)->id);
                                (*npc)->max_hp = (*npc)->hp;
                                (*npc)->hp_regen = (*npc)->max_hp / 10;
                                (*npc)->hp_regen = (*npc)->hp_regen + 1;
                                NpcDropInfo drop = NpcValues::GetDrop(
                                    (*MAINFORM)->npc_values, (*npc)->id);
                                (*npc)->wDrop_item_id = drop.item_id;
                                (*npc)->wDrop_amount = drop.amount;
                                (*npc)->pos_buffer =
                                    EncodeNumber(npc_control, (*npc)->index, 1);
                                (*npc)->pos_buffer.Insert(
                                    EncodeNumber(npc_control, (*npc)->x, 1),
                                    (*npc)->pos_buffer.Length() + 1);
                                (*npc)->pos_buffer.Insert(
                                    EncodeNumber(npc_control, (*npc)->y, 1),
                                    (*npc)->pos_buffer.Length() + 1);
                                (*npc)->pos_buffer.Insert(
                                    EncodeNumber(npc_control,
                                                 (unsigned short)(*npc)->direction,
                                                 1),
                                    (*npc)->pos_buffer.Length() + 1);
                            }
                        }
                    }
                }
            }
            else
            {
                if (0x1c2 < npc_control->regen_counter)
                {
                    (*npc)->hp = (*npc)->hp + (*npc)->hp_regen;
                    if ((*npc)->hp > (*npc)->max_hp)
                        (*npc)->hp = (*npc)->max_hp;
                }
                if ((*npc)->aggressive != false)
                {
                    (*npc)->nLeash_timer = (*npc)->nLeash_timer - 1;
                    if ((unsigned short)(*npc)->nLeash_timer <= 0 &&
                        0x1e < (*npc)->nHp_pct)
                    {
                        (*npc)->aggressive = false;
                        (*npc)->chase_target_id = -1;
                    }
                }
                if (1 <= map->npc_act_ticks)
                {
                    (*npc)->pos_pending = 0;
                    (*npc)->talk_pending = 0;
                    (*npc)->attack_pending = 0;
                    if (map->player_count < 1)
                        map->npc_act_ticks = map->npc_act_ticks - 1;
                    if ((*npc)->act_ticks == 0)
                        (*npc)->act_ticks = 1;
                    if (0x17 < npc_control->act_counter)
                    {
                        npc_control->talk_counter++;
                        if (4 < npc_control->talk_counter)
                        {
                            npc_control->talk_counter = 0;
                            String line =
                                NpcValues::RollTalk((*MAINFORM)->npc_values, (*npc)->id);
                            if (0 < line.Length())
                            {
                                map->npc_dirty = 1;
                                (*npc)->talk_pending = 1;
                                (*npc)->talk_buffer =
                                    EncodeNumber(npc_control, (*npc)->index, 1);
                                (*npc)->talk_buffer.Insert(
                                    EncodeNumber(npc_control, line.Length(), 1),
                                    (*npc)->talk_buffer.Length() + 1);
                                (*npc)->talk_buffer.Insert(
                                    line, (*npc)->talk_buffer.Length() + 1);
                            }
                        }
                    }

                    if ((*npc)->aggressive != false || (*npc)->in_combat != false)
                    {
                        if (npc_control->act_counter % (*npc)->act_ticks == 0)
                        {
                            ++(*npc)->nAct_counter;
                            if (2 <= (unsigned short)(*npc)->nAct_counter)
                            {
                                (*npc)->nAct_counter = 0;
                                (*npc)->nMove_cooldown = (*npc)->nMove_cooldown - 1;
                                Player *target = NULL;
                                int distance = 10000;
                                if (0 < (*npc)->chase_target_id)
                                {
                                    if ((*npc)->target_player_id < 0)
                                        target = Players::Players_GetById(
                                            npc_control->players,
                                            (*npc)->chase_target_id);
                                    else
                                        target = Players::Players_GetById(
                                            npc_control->players,
                                            (*npc)->target_player_id);
                                    if (target == NULL)
                                        (*npc)->chase_target_id = -1;
                                    else if (target->map_id != map->rid)
                                    {
                                        (*npc)->chase_target_id = -1;
                                        target = NULL;
                                    }
                                }
                                if (target != NULL)
                                {
                                    distance = Npc_GetDistance(npc_control, *npc, target);
                                    if (target->in_party != false && 1 < distance)
                                    {
                                        for (int i = 0; i < PARTY_MAX_MEMBERS; i++)
                                        {
                                            Player *member = Players::Players_GetById(
                                                npc_control->players,
                                                target->party_ids[i]);
                                            if (member != NULL &&
                                                member->map_id == target->map_id)
                                            {
                                                int d = Npc_GetDistance(
                                                    npc_control, *npc, member);
                                                if (d <= distance)
                                                {
                                                    distance = d;
                                                    target = member;
                                                }
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    if (npc_control->player_targets_valid == 0)
                                    {
                                        npc_control->player_targets.clear();
                                        for (Player **it =
                                                 npc_control->players->players.begin();
                                             it != npc_control->players->players.end();
                                             it++)
                                        {
                                            if ((*it)->logged_in != false &&
                                                (*it)->map_id == map->rid)
                                                npc_control->player_targets.insert(
                                                    npc_control->player_targets.end(),
                                                    *it);
                                        }
                                        npc_control->player_targets_valid = 1;
                                    }
                                    for (Player **it =
                                             npc_control->player_targets.begin();
                                         it != npc_control->player_targets.end();
                                         it++)
                                    {
                                        int d = Npc_GetDistance(npc_control, *npc, *it);
                                        if (d <= distance)
                                        {
                                            distance = d;
                                            target = *it;
                                        }
                                    }
                                }

                                if (target == NULL)
                                {
                                    (*npc)->chase_target_id = -1;
                                    continue;
                                }
                                else if (distance <= 1)
                                    goto attack;
                                else if ((*npc)->chase_target_id < 0)
                                {
                                    if (11 < distance)
                                        continue;
                                    goto chase;
                                }
                                else
                                {
                                    if (distance <= 16)
                                        goto chase;
                                    (*npc)->chase_target_id = -1;
                                    continue;
                                }
                            chase:
                                Npc_ChaseTarget(npc_control,
                                                *npc,
                                                target,
                                                map->rid,
                                                map->width,
                                                map->height);
                                if ((*npc)->pos_pending != 0)
                                {
                                    map->npc_dirty = 1;
                                    (*npc)->pos_buffer =
                                        EncodeNumber(npc_control, (*npc)->index, 1);
                                    (*npc)->pos_buffer.Insert(
                                        EncodeNumber(npc_control, (*npc)->x, 1),
                                        (*npc)->pos_buffer.Length() + 1);
                                    (*npc)->pos_buffer.Insert(
                                        EncodeNumber(npc_control, (*npc)->y, 1),
                                        (*npc)->pos_buffer.Length() + 1);
                                    (*npc)->pos_buffer.Insert(
                                        EncodeNumber(npc_control,
                                                     (unsigned short)(*npc)->direction,
                                                     1),
                                        (*npc)->pos_buffer.Length() + 1);
                                }
                                continue;
                            attack:
                                if (Npc_AttackPlayer(npc_control, *npc, target))
                                {
                                    (*npc)->chase_target_id = -1;
                                    flag = false;
                                }
                                map->npc_dirty = 1;
                                (*npc)->attack_pending = 1;
                            aggro_done:;
                            }
                        }
                    }
                    else
                    {
                        if (npc_control->act_counter % (*npc)->act_ticks == 0)
                        {
                            ++(*npc)->nAct_counter;
                            if (2 <= (unsigned short)(*npc)->nAct_counter)
                            {
                                (*npc)->nAct_counter = 0;
                                (*npc)->nMove_cooldown = (*npc)->nMove_cooldown - 1;
                                Npc_Wander(
                                    npc_control, *npc, map->rid, map->width, map->height);
                                if ((*npc)->pos_pending != 0)
                                {
                                    map->npc_dirty = 1;
                                    (*npc)->pos_buffer =
                                        EncodeNumber(npc_control, (*npc)->index, 1);
                                    (*npc)->pos_buffer.Insert(
                                        EncodeNumber(npc_control, (*npc)->x, 1),
                                        (*npc)->pos_buffer.Length() + 1);
                                    (*npc)->pos_buffer.Insert(
                                        EncodeNumber(npc_control, (*npc)->y, 1),
                                        (*npc)->pos_buffer.Length() + 1);
                                    (*npc)->pos_buffer.Insert(
                                        EncodeNumber(npc_control,
                                                     (unsigned short)(*npc)->direction,
                                                     1),
                                        (*npc)->pos_buffer.Length() + 1);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (0x17 < npc_control->act_counter)
        npc_control->act_counter = 0;
    if (0x1c2 < npc_control->regen_counter)
        npc_control->regen_counter = 0;
    Player **player;
    for (player = npc_control->players->players.begin();
         player != npc_control->players->players.end();
         player++)
    {
        if ((*player)->logged_in != false && 0 < (*player)->map_id &&
            (*player)->map_id <= (int)npc_control->map_control->maps.size() &&
            Mapcontrol_GetByIndex(npc_control->map_control, (*player)->map_id - 1)
                    ->npc_dirty != 0)
        {
            try
            {
                String pos = "";
                String talk = "";
                String attack = "";
                Npc **npc2 = (Npc **)Mapcontrol_GetByIndex(npc_control->map_control,
                                                           (*player)->map_id - 1)
                                 ->npc_list.begin();
                while ((Npc **)Mapcontrol_GetByIndex(npc_control->map_control,
                                                     (*player)->map_id - 1)
                           ->npc_list.end() != npc2)
                {
                    if ((*npc2)->pos_pending != 0 &&
                        Npc_IsWithinRange(npc_control,
                                          (*player)->x,
                                          (*player)->y,
                                          (*npc2)->x,
                                          (*npc2)->y) != false)
                        pos.Insert((*npc2)->pos_buffer, pos.Length() + 1);
                    if ((*npc2)->talk_pending != 0 &&
                        Npc_IsWithinRange(npc_control,
                                          (*player)->x,
                                          (*player)->y,
                                          (*npc2)->x,
                                          (*npc2)->y) != false)
                        talk.Insert((*npc2)->talk_buffer, talk.Length() + 1);
                    if ((*npc2)->attack_pending != 0 &&
                        Npc_IsWithinRange(npc_control,
                                          (*player)->x,
                                          (*player)->y,
                                          (*npc2)->x,
                                          (*npc2)->y) != false)
                        attack.Insert((*npc2)->attack_buffer, attack.Length() + 1);
                    npc2++;
                }
                if (4 <= pos.Length() || 4 <= talk.Length() || 4 <= attack.Length())
                {
                    String data = pos;
                    data.Insert(String((char)-1), data.Length() + 1);
                    data.Insert(attack, data.Length() + 1);
                    data.Insert(String((char)-1), data.Length() + 1);
                    data.Insert(talk, data.Length() + 1);
                    data.Insert(String((char)-1), data.Length() + 1);
                    if ((*player)->stats_dirty != 0)
                    {
                        data.Insert(EncodeNumber(npc_control, (*player)->hp, 2),
                                    data.Length() + 1);
                        data.Insert(EncodeNumber(npc_control, (*player)->tp, 2),
                                    data.Length() + 1);
                        if ((*player)->in_party != false)
                        {
                            String party_data =
                                EncodeNumber(npc_control, (*player)->player_id, 2);
                            party_data.Insert(
                                EncodeNumber(npc_control, Player::HpPercent(*player), 1),
                                party_data.Length() + 1);
                            Server_BroadcastToParty(npc_control->server,
                                                    *player,
                                                    PacketAction_Agree,
                                                    PacketFamily_Party,
                                                    party_data);
                        }
                        (*player)->stats_dirty = 0;
                    }
                    Client_SendEncoded(npc_control->server,
                                       *player,
                                       PacketAction_Player,
                                       PacketFamily_Npc,
                                       data);
                }
            }
            catch (...)
            {
            }
        }
    }
    if (flag != false)
        return;
    for (player = npc_control->players->players.begin();
         player != npc_control->players->players.end();
         player++)
    {
        if ((*player)->logged_in != false && 0 < (*player)->map_id &&
            (*player)->map_id <= (int)npc_control->map_control->maps.size() &&
            (*player)->dead != false)
            Player_Respawn(npc_control->server, *player);
    }
    return;
}
