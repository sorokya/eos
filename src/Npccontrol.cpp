#include <vcl.h>
#pragma hdrstop

#include "Npccontrol.h"
#include "Player.h"

#pragma package(smart_init)

// Players-unit operations (unit not yet reconstructed). Their argument shapes
// are pinned by this unit's call sites; the reference uses the container's own
// begin()/end() template COMDATs.
Player **Players_Iter_Begin(Players *players);
Player **Players_Iter_End(Players *players);

// Mapcontrol-unit operations (unit not yet reconstructed). Argument shapes are
// pinned by this unit's call sites.
int Map_IsWalkableNPC(Mapcontrol *map, int map_id, int x, int y, int flag);
bool Map_IsOccupied(Mapcontrol *map, int map_id, int x, int y);

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
    if (self->flag_0x14 == 0)
    {
        self->player_targets.clear();
        for (Player **it = Players_Iter_Begin(self->players);
             it != Players_Iter_End(self->players);
             it++)
        {
            if ((*it)->logged_in && (*it)->map_id == map_id)
                self->player_targets.insert(self->player_targets.end(), *it);
        }
        self->flag_0x14 = 1;
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
    if (npc->nAttack_dir == 0)
    {
        if (npc->y < map_h)
        {
            if (Map_IsWalkableNPC(self->map_control, map_id, npc->x, npc->y + 1, 0) == 0)
            {
                if (!Map_IsOccupied(self->map_control, map_id, npc->x, npc->y + 1))
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
    else if (npc->nAttack_dir == 1)
    {
        if (npc->x >= 1)
        {
            if (Map_IsWalkableNPC(self->map_control, map_id, npc->x - 1, npc->y, 0) == 0)
            {
                if (!Map_IsOccupied(self->map_control, map_id, npc->x - 1, npc->y))
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
    else if (npc->nAttack_dir == 2)
    {
        if (npc->y >= 1)
        {
            if (Map_IsWalkableNPC(self->map_control, map_id, npc->x, npc->y - 1, 0) == 0)
            {
                if (!Map_IsOccupied(self->map_control, map_id, npc->x, npc->y - 1))
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
    else if (npc->nAttack_dir == 3 && npc->x < map_w)
    {
        if (Map_IsWalkableNPC(self->map_control, map_id, npc->x + 1, npc->y, 0) == 0)
        {
            if (!Map_IsOccupied(self->map_control, map_id, npc->x + 1, npc->y))
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
    if (self->flag_0x14 == 0)
    {
        self->player_targets.clear();
        for (Player **it = Players_Iter_Begin(self->players);
             it != Players_Iter_End(self->players);
             it++)
        {
            if ((*it)->logged_in && (*it)->map_id == map_id)
                self->player_targets.insert(self->player_targets.end(), *it);
        }
        self->flag_0x14 = 1;
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
