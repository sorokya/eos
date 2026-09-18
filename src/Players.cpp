#include <vcl.h>
#pragma hdrstop

#include <stdlib.h>

#include "Players.h"
#include "Mainform.h"
#include "Gamecontrol.h"
#include "Itemvalues.h"
#include "Settings.h"
#include "Protocol.h"

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
        if (ItemValues::Eif_GetType((*MAINFORM)->item_values, iter->item_id) ==
            ItemType_Key)
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
    player->drop_counter = 20;
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

// BEGIN GENERATED STUBS (scripts/genstubs.py)
#pragma warn - 8057
// STUB(0x00407b30, 80 bytes) FUN_00407b30 - ref: undefined FUN_00407b30(int param_1, byte
// param_2)
void FUN_00407b30_Stub(int a0, unsigned char a1)
{
}
// STUB(0x00407b80, 934 bytes) Players_Tick - ref: void Players_Tick(Players * this)
void Players_Tick_Stub(void *a0)
{
}
// STUB(0x00407f40, 36 bytes) FUN_00407f40 - ref: undefined FUN_00407f40(int param_1)
void FUN_00407f40_Stub(int a0)
{
}
// STUB(0x00407f64, 36 bytes) FUN_00407f64 - ref: int FUN_00407f64(int param_1)
int FUN_00407f64_Stub(int a0)
{
    return 0;
}
// STUB(0x00407f88, 25 bytes) FUN_00407f88 - ref: int FUN_00407f88(int param_1, int
// param_2)
int FUN_00407f88_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00407fb0, 101 bytes) FUN_00407fb0 - ref: undefined4 * FUN_00407fb0(int param_1,
// undefined4 * param_2)
void *FUN_00407fb0_Stub(int a0, void *a1)
{
    return 0;
}
// STUB(0x00408024, 92 bytes) FUN_00408024 - ref: undefined4 * FUN_00408024(int param_1,
// undefined4 * param_2, undefined4 * param_3)
void *FUN_00408024_Stub(int a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x00408098, 88 bytes) FUN_00408098 - ref: undefined4 * FUN_00408098(undefined4 *
// param_1, undefined4 * param_2, undefined4 * param_3)
void *FUN_00408098_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x004081c8, 381 bytes) FUN_004081c8 - ref: undefined4 FUN_004081c8(Players *
// players, int socket)
int FUN_004081c8_Stub(void *a0, int a1)
{
    return 0;
}
// STUB(0x0040894c, 510 bytes) FUN_0040894c - ref: undefined FUN_0040894c(Players *
// players, int param_2)
void FUN_0040894c_Stub(void *a0, int a1)
{
}
// STUB(0x00408b4c, 36 bytes) FUN_00408b4c - ref: undefined FUN_00408b4c(ItemStackVector *
// param_1)
void FUN_00408b4c_Stub(void *a0)
{
}
// STUB(0x00408b70, 36 bytes) FUN_00408b70 - ref: undefined FUN_00408b70(int param_1)
void FUN_00408b70_Stub(int a0)
{
}
// STUB(0x00408b94, 36 bytes) FUN_00408b94 - ref: undefined FUN_00408b94(int param_1)
void FUN_00408b94_Stub(int a0)
{
}
// STUB(0x00408bb8, 101 bytes) FUN_00408bb8 - ref: undefined4 * FUN_00408bb8(int param_1,
// undefined4 * param_2)
void *FUN_00408bb8_Stub(int a0, void *a1)
{
    return 0;
}
// STUB(0x00408c38, 91 bytes) FUN_00408c38 - ref: undefined4 *
// FUN_00408c38(ItemStackVector * param_1, ItemStack * param_2, ItemStack * param_3)
void *FUN_00408c38_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x00408c94, 11 bytes) Player_Spells_Iter_Start - ref: PlayerSpell *
// Player_Spells_Iter_Start(PlayerSpellVector * param_1)
void *Player_Spells_Iter_Start_Stub(void *a0)
{
    return 0;
}
// STUB(0x00408cac, 91 bytes) FUN_00408cac - ref: undefined4 * FUN_00408cac(int param_1,
// undefined4 * param_2, undefined4 * param_3)
void *FUN_00408cac_Stub(int a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x00408d08, 11 bytes) FUN_00408d08 - ref: undefined4 FUN_00408d08(int param_1)
int FUN_00408d08_Stub(int a0)
{
    return 0;
}
// STUB(0x00408d20, 92 bytes) FUN_00408d20 - ref: undefined4 * FUN_00408d20(int param_1,
// undefined4 * param_2, undefined4 * param_3)
void *FUN_00408d20_Stub(int a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x00408d7c, 42 bytes) FUN_00408d7c - ref: undefined4 * FUN_00408d7c(undefined4 *
// param_1, undefined4 * param_2, undefined4 * param_3)
void *FUN_00408d7c_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x00408e18, 48 bytes) FUN_00408e18 - ref: undefined4 * FUN_00408e18(undefined4 *
// param_1, undefined4 * param_2, undefined4 * param_3)
void *FUN_00408e18_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x00408e88, 53 bytes) FUN_00408e88 - ref: undefined4 * FUN_00408e88(undefined4 *
// param_1, undefined4 * param_2, undefined4 * param_3)
void *FUN_00408e88_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x0040902c, 14592 bytes) Character_BuildSaveQuery - ref: AnsiString *
// Character_BuildSaveQuery(AnsiString * out_str, Players * players, Player * player, int
// flags)
void *Character_BuildSaveQuery_Stub(void *a0, void *a1, void *a2, int a3)
{
    return 0;
}
// STUB(0x0040c92c, 59 bytes) FUN_0040c92c - ref: undefined4 FUN_0040c92c(int param_1, int
// param_2)
int FUN_0040c92c_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x0040c968, 219 bytes) FUN_0040c968 - ref: undefined4 FUN_0040c968(int param_1,
// undefined4 param_2, int param_3)
int FUN_0040c968_Stub(int a0, int a1, int a2)
{
    return 0;
}
// STUB(0x0040cc34, 74 bytes) FUN_0040cc34 - ref: int FUN_0040cc34(undefined4 param_1, int
// param_2, int param_3)
int FUN_0040cc34_Stub(int a0, int a1, int a2)
{
    return 0;
}
// STUB(0x0040ccc8, 69 bytes) FUN_0040ccc8 - ref: undefined4 FUN_0040ccc8(undefined4
// param_1, int param_2, int param_3)
int FUN_0040ccc8_Stub(int a0, int a1, int a2)
{
    return 0;
}
// STUB(0x0040cd10, 1231 bytes) FUN_0040cd10 - ref: undefined4 FUN_0040cd10(undefined4
// param_1, int param_2, int param_3, int param_4)
int FUN_0040cd10_Stub(int a0, int a1, int a2, int a3)
{
    return 0;
}
// STUB(0x0040d248, 1177 bytes) FUN_0040d248 - ref: undefined4 FUN_0040d248(undefined4
// param_1, int param_2, int param_3, int param_4)
int FUN_0040d248_Stub(int a0, int a1, int a2, int a3)
{
    return 0;
}
// STUB(0x0040dc48, 11 bytes) FUN_0040dc48 - ref: undefined4 FUN_0040dc48(int param_1)
int FUN_0040dc48_Stub(int a0)
{
    return 0;
}
// STUB(0x0040dec8, 207 bytes) FUN_0040dec8 - ref: undefined FUN_0040dec8(undefined4
// param_1, int param_2, int param_3, uint param_4)
void FUN_0040dec8_Stub(int a0, int a1, int a2, unsigned int a3)
{
}
// STUB(0x0040dfe4, 223 bytes) FUN_0040dfe4 - ref: undefined FUN_0040dfe4(undefined4
// param_1, int param_2, int param_3, int param_4)
void FUN_0040dfe4_Stub(int a0, int a1, int a2, int a3)
{
}
// STUB(0x0040e0c4, 127 bytes) FUN_0040e0c4 - ref: undefined4 FUN_0040e0c4(undefined4
// param_1, int param_2, int param_3)
int FUN_0040e0c4_Stub(int a0, int a1, int a2)
{
    return 0;
}
// STUB(0x0040e144, 266 bytes) FUN_0040e144 - ref: undefined4 FUN_0040e144(undefined4
// param_1, int param_2, int param_3, int param_4)
int FUN_0040e144_Stub(int a0, int a1, int a2, int a3)
{
    return 0;
}
// STUB(0x0040e250, 134 bytes) FUN_0040e250 - ref: undefined4 FUN_0040e250(undefined4
// param_1, int param_2, int param_3)
int FUN_0040e250_Stub(int a0, int a1, int a2)
{
    return 0;
}
// STUB(0x0040e2d8, 189 bytes) FUN_0040e2d8 - ref: undefined4 FUN_0040e2d8(undefined4
// param_1, int param_2, int param_3)
int FUN_0040e2d8_Stub(int a0, int a1, int a2)
{
    return 0;
}
// STUB(0x0040e3e0, 152 bytes) FUN_0040e3e0 - ref: int FUN_0040e3e0(int param_1,
// undefined4 * param_2, undefined4 * param_3)
int FUN_0040e3e0_Stub(int a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x0040e484, 19 bytes) FUN_0040e484 - ref: undefined FUN_0040e484(undefined4
// param_1, undefined4 param_2, undefined4 * param_3)
void FUN_0040e484_Stub(int a0, int a1, void *a2)
{
}
// STUB(0x0040e6d0, 90 bytes) FUN_0040e6d0 - ref: undefined FUN_0040e6d0(undefined4
// param_1, undefined4 * param_2)
void FUN_0040e6d0_Stub(int a0, void *a1)
{
}
// STUB(0x0040e748, 48 bytes) FUN_0040e748 - ref: undefined4 * FUN_0040e748(undefined4 *
// param_1, undefined4 * param_2, undefined4 * param_3)
void *FUN_0040e748_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x0040e778, 38 bytes) FUN_0040e778 - ref: int FUN_0040e778(int param_1)
int FUN_0040e778_Stub(int a0)
{
    return 0;
}
// STUB(0x0040e7b8, 111 bytes) FUN_0040e7b8 - ref: int FUN_0040e7b8(undefined4 param_1,
// int param_2)
int FUN_0040e7b8_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x0040e828, 99 bytes) FUN_0040e828 - ref: int FUN_0040e828(undefined4 * param_1,
// undefined4 * param_2, int param_3)
int FUN_0040e828_Stub(void *a0, void *a1, int a2)
{
    return 0;
}
// STUB(0x0040e8c0, 33 bytes) FUN_0040e8c0 - ref: int FUN_0040e8c0(int param_1, int
// param_2, undefined4 * param_3)
int FUN_0040e8c0_Stub(int a0, int a1, void *a2)
{
    return 0;
}
// STUB(0x0040e8e4, 11 bytes) FUN_0040e8e4 - ref: undefined4 FUN_0040e8e4(int param_1)
int FUN_0040e8e4_Stub(int a0)
{
    return 0;
}
// STUB(0x0040e8f0, 11 bytes) FUN_0040e8f0 - ref: undefined4 FUN_0040e8f0(int param_1)
int FUN_0040e8f0_Stub(int a0)
{
    return 0;
}
// STUB(0x0040e8fc, 25 bytes) FUN_0040e8fc - ref: undefined FUN_0040e8fc(int param_1, int
// param_2)
void FUN_0040e8fc_Stub(int a0, int a1)
{
}
// STUB(0x0040e918, 54 bytes) FUN_0040e918 - ref: int FUN_0040e918(int param_1, int
// param_2)
int FUN_0040e918_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x0040e974, 88 bytes) FUN_0040e974 - ref: int FUN_0040e974(undefined4 param_1, int
// param_2, int param_3)
int FUN_0040e974_Stub(int a0, int a1, int a2)
{
    return 0;
}
// STUB(0x0040e9cc, 89 bytes) FUN_0040e9cc - ref: undefined4 FUN_0040e9cc(undefined4
// param_1, int param_2, int param_3)
int FUN_0040e9cc_Stub(int a0, int a1, int a2)
{
    return 0;
}
// STUB(0x0040ea28, 101 bytes) FUN_0040ea28 - ref: undefined4 * FUN_0040ea28(int param_1,
// undefined4 * param_2)
void *FUN_0040ea28_Stub(int a0, void *a1)
{
    return 0;
}
// STUB(0x0040ea90, 20 bytes) FUN_0040ea90 - ref: undefined FUN_0040ea90(undefined4
// param_1, int param_2)
void FUN_0040ea90_Stub(int a0, int a1)
{
}
// STUB(0x0040eaa4, 5213 bytes) FUN_0040eaa4 - ref: undefined1 FUN_0040eaa4(undefined4
// param_1, int param_2)
char FUN_0040eaa4_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x0040ff54, 88 bytes) FUN_0040ff54 - ref: int FUN_0040ff54(int param_1, int
// param_2)
int FUN_0040ff54_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x0040ffac, 106 bytes) FUN_0040ffac - ref: int FUN_0040ffac(int param_1, int
// param_2)
int FUN_0040ffac_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x00410018, 357 bytes) FUN_00410018 - ref: undefined FUN_00410018(int param_1, int
// param_2)
void FUN_00410018_Stub(int a0, int a1)
{
}
// STUB(0x00410750, 108 bytes) FUN_00410750 - ref: undefined FUN_00410750(int param_1)
void FUN_00410750_Stub(int a0)
{
}
// STUB(0x00410dac, 11 bytes) FUN_00410dac - ref: undefined4 FUN_00410dac(int param_1)
int FUN_00410dac_Stub(int a0)
{
    return 0;
}
// STUB(0x00410dc8, 11 bytes) FUN_00410dc8 - ref: undefined4 FUN_00410dc8(int param_1)
int FUN_00410dc8_Stub(int a0)
{
    return 0;
}
#pragma warn.8057
// END GENERATED STUBS
