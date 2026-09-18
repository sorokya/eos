#include <vcl.h>
#pragma hdrstop

#include <stdlib.h>

#include "Players.h"
#include "Mainform.h"
#include "Gamecontrol.h"
#include "Itemvalues.h"
#include "Settings.h"

#pragma package(smart_init)

extern TGUI **MAINFORM;

int RandRange(int max)
{
    return (max ? (int)(_lrand() % max) : 0);
}

Players::Players(Settings *settings, Mysqlcontrols *mysql_controls)
{
    idle_timeout = 0;
    stat_total = 0;
    dirty = 0;
    this->settings = settings;
    this->mysql_controls = mysql_controls;
    for (int i = 0; i < 100000; i++)
        by_id[i] = 0;
}

int Players::Players_ActiveCount(Players *self)
{
    return self->players.end() - self->players.begin();
}

bool Players::Players_Add(Players *self, TCustomWinSocket *socket)
{
    if (socket->SocketHandle >= 100000)
        return false;
    if (self->by_id[socket->SocketHandle] != 0)
        return false;
    int same_ip = 0;
    for (Player **iter = self->players.begin(); iter != self->players.end(); iter++)
    {
        if (Socket_GetRemoteIP((*iter)->socket) != Socket_GetRemoteIP(socket))
            continue;
        same_ip++;
        if (Settings::GetMaxConnections(self->settings) <= same_ip)
            return false;
    }
    Player *player = new Player(socket);
    self->by_id[socket->SocketHandle] = player;
    self->players.insert(self->players.end(), player);
    return true;
}

void Players::Players_MarkRemoving(Players *self, TCustomWinSocket *socket)
{
    if (self->by_id[socket->SocketHandle] != 0)
    {
        Player *player = self->by_id[socket->SocketHandle];
        player->removing = true;
        self->dirty = 1;
    }
}

int Players::Players_GetActiveCount(Players *self)
{
    return Players_ActiveCount(self);
}

int Players::Players_GetIdleTimeout(Players *self)
{
    return self->idle_timeout;
}

int Players::Players_GetStatTotal(Players *self)
{
    return self->stat_total;
}

void Players::Players_MarkDirty(Players *self)
{
    self->dirty = 1;
}

Player *Players::Players_GetById(Players *self, int player_id)
{
    if (player_id > 0 && player_id < 100000)
        return self->by_id[player_id];
    return 0;
}

bool Players::Players_IsPlayerAt(Players *self, int map_id, int x, int y)
{
    for (Player **iter = self->players.begin(); iter != self->players.end(); iter++)
    {
        if ((*iter)->x == x && (*iter)->y == y && (*iter)->map_id == map_id)
            return true;
    }
    return false;
}

Player *Players::Players_GetByMapTile(Players *self, int map_id, int x, int y)
{
    for (Player **iter = self->players.begin(); iter != self->players.end(); iter++)
    {
        if ((*iter)->map_id == map_id && (*iter)->x == x && (*iter)->y == y)
            return *iter;
    }
    return 0;
}

int Players::Players_CountArenaPlayers(Players *self, int map_id)
{
    int count = 0;
    for (Player **iter = self->players.begin(); iter != self->players.end(); iter++)
    {
        if ((*iter)->map_id == map_id && (*iter)->arena_queued != false)
            count = count + 1;
    }
    return count;
}

int Players::Players_GetItemAmount(Players *self, Player *player, int item_id)
{
    for (PlayerInventory *iter = player->inventory.begin();
         iter != player->inventory.end();
         iter++)
    {
        if (iter->item_id == item_id)
            return iter->amount;
    }
    return -1;
}

bool Players::Player_HasSpellId(Players *self, Player *player, int spell_id)
{
    for (PlayerSkill *iter = player->spells.begin(); iter != player->spells.end(); iter++)
    {
        if (iter->skill_id == spell_id)
            return true;
    }
    return false;
}

bool Players::Player_HasKeyItem(Players *self, Player *player, int key_item_id)
{
    for (PlayerInventory *iter = player->inventory.begin();
         iter != player->inventory.end();
         iter++)
    {
        if (ItemValues::Eif_GetType((*MAINFORM)->item_values, iter->item_id) == 9)
        {
            if (ItemValues::Eif_GetSpec1((*MAINFORM)->item_values, iter->item_id) ==
                key_item_id)
                return true;
        }
    }
    return false;
}

void Players::Player_RegenHpTp(Players *self, Player *player)
{
    player->field_0x37c = 30;
    player->field_0x380 = 20;
    player->field_0x378 = 3;
    int regen;
    if (player->sitting != false)
    {
        regen = player->max_hp / 5;
        regen = regen + 1;
    }
    else
    {
        regen = player->max_hp / 10;
        regen = regen + 1;
    }
    player->hp = player->hp + regen;
    if (player->hp > player->max_hp)
        player->hp = player->max_hp;
    if (player->sitting != false)
    {
        regen = player->max_tp / 5;
        regen = regen + 1;
    }
    else
    {
        regen = player->max_tp / 10;
        regen = regen + 1;
    }
    player->tp = player->tp + regen;
    if (player->tp > player->max_tp)
        player->tp = player->max_tp;
}

void Players::Player_LevelUp(Server *server, Player *player)
{
    player->level = player->level + 1;
    player->stat_points = player->stat_points + 3;
    player->skill_points = player->skill_points + 4;
    int hp_bonus = player->level / 10;
    player->base_hp = player->base_hp + 2;
    player->base_hp = player->base_hp + RandRange(2);
    player->base_hp = player->base_hp + hp_bonus;
    player->base_tp = player->base_tp + 2;
    player->base_tp = player->base_tp + RandRange(2);
    int sp_bonus = player->level / 20;
    player->base_sp = player->base_sp + 1;
    player->base_sp = player->base_sp + sp_bonus;
    if (player->base_hp > 64000)
        player->base_hp = 64000;
    if (player->base_tp > 64000)
        player->base_tp = 64000;
    if (player->base_sp > 64000)
        player->base_sp = 64000;
    Player::CalculateHP_TP_SP(player);
}

int Players::Player_TryLevelUp(Server *server, Player *player)
{
    int new_level = 0;
    while ((int)player->experience >= Gamecontrol::Exp_RequiredForLevel(
                                          (*MAINFORM)->game_control, player->level + 1))
    {
        Player_LevelUp(server, player);
        new_level = player->level;
    }
    return new_level;
}

void Players::Party_AddNewMember(Players *players,
                                 Player *existing_member,
                                 Player *new_member)
{
    for (int i = 0; i < 10; i++)
    {
        Player *player = Players_GetById(players, existing_member->party_ids[i]);
        if (player != 0)
            Player::AddPartyMember(player, new_member->player_id);
    }
    new_member->party_leader_id = existing_member->party_leader_id;
    Player::ClearPartyRoster(new_member);
    new_member->in_party = true;
    for (int i = 0; i < 10; i++)
        Player::AddPartyMember(new_member, existing_member->party_ids[i]);
}

void Players::Player_LeaveParty(Players *players, Player *player)
{
    bool disband = false;
    player->in_party = false;
    if (player->player_id == player->party_leader_id || player->CountPartyMembers() < 3)
        disband = true;
    for (int i = 0; i < 10; i++)
    {
        if (player->party_ids[i] != player->player_id)
        {
            Player *member = Players_GetById(players, player->party_ids[i]);
            if (member != 0)
            {
                if (disband)
                {
                    Player::ClearPartyRoster(member);
                    member->in_party = false;
                }
                else
                {
                    for (int j = 0; j < 10; j++)
                    {
                        if (member->party_ids[j] == player->player_id)
                            member->party_ids[j] = -1;
                    }
                }
            }
        }
    }
    Player::ClearPartyRoster(player);
    player->in_party = false;
}

void Players::Player_AddItem(Players *self, Player *player, int new_item, int amount)
{
    player->inventory_dirty = 1;
    for (PlayerInventory *iter = player->inventory.begin();
         iter != player->inventory.end();
         iter++)
    {
        if (iter->item_id == new_item)
        {
            iter->amount = iter->amount + amount;
            player->item_change_amount = iter->amount;
            return;
        }
    }
    PlayerInventory item(new_item);
    item.amount = amount;
    player->inventory.insert(player->inventory.end(), item);
    player->item_change_amount = amount;
}

bool Players::Player_RemoveItem(Players *self, Player *player, int item_id, int amount)
{
    player->equipment_dirty = 1;
    player->inventory_dirty = 1;
    for (PlayerInventory *iter = player->inventory.begin();
         iter != player->inventory.end();
         iter++)
    {
        if (iter->item_id == item_id)
        {
            if ((unsigned int)amount > (unsigned int)iter->amount)
            {
                player->item_change_id = -1;
                player->item_change_count = 0;
                player->item_change_remaining = 0;
                return false;
            }
            if (amount == iter->amount)
            {
                player->item_change_id = item_id;
                player->item_change_count = amount;
                player->item_change_remaining = 0;
                player->inventory.erase(iter);
                return true;
            }
            iter->amount = iter->amount - amount;
            player->item_change_id = item_id;
            player->item_change_count = amount;
            player->item_change_remaining = iter->amount;
            return true;
        }
    }
    return false;
}

bool Players::CharName_Validate(Players *players, Player *player, String name)
{
    Player **iter;
    if (player->null_string != "")
    {
        return true;
    }
    iter = players->players.begin();
    while (iter != players->players.end())
    {
        if (name == (*iter)->null_string)
            return true;
        iter++;
    }
    player->null_string = name;
    return false;
}

Player *Players::Players_FindByName(Players *self, String name)
{
    for (Player **iter = self->players.begin(); iter != self->players.end(); iter++)
    {
        if (AnsiLowerCase((*iter)->name) == AnsiLowerCase(name))
            return *iter;
    }
    return 0;
}
