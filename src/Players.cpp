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

int Players::Player_GetSpellLevel(Players *self, Player *player, int spell_id)
{
    for (PlayerSkill *iter = player->spells.begin(); iter != player->spells.end(); iter++)
    {
        if (iter->skill_id == spell_id)
            return iter->level;
    }
    return -1;
}

bool Players::Player_HasBankItem(Players *self, Player *player, int item_id)
{
    for (PlayerInventory *iter = player->bank.begin(); iter != player->bank.end(); iter++)
    {
        if (iter->item_id == item_id)
            return true;
    }
    return false;
}

void Players::Player_RemoveItemNoQuestRules(Players *self,
                                            Player *player,
                                            int item_id,
                                            int amount)
{
    player->equipment_dirty = 1;
    player->inventory_dirty = 1;
    for (PlayerInventory *iter = player->inventory.begin();
         iter != player->inventory.end();
         iter++)
    {
        if (iter->item_id == item_id)
        {
            if ((unsigned int)amount >= (unsigned int)iter->amount)
            {
                player->item_change_id = item_id;
                player->item_change_count = iter->amount;
                player->item_change_remaining = 0;
                player->inventory.erase(iter);
                return;
            }
            iter->amount = iter->amount - amount;
            player->item_change_id = item_id;
            player->item_change_count = amount;
            player->item_change_remaining = iter->amount;
            return;
        }
    }
}

void Players::Player_AddBankItem(Players *self, Player *player, int item_id, int amount)
{
    player->bank_dirty = 1;
    for (PlayerInventory *iter = player->bank.begin(); iter != player->bank.end(); iter++)
    {
        if (iter->item_id == item_id)
        {
            iter->amount = iter->amount + amount;
            if ((unsigned int)iter->amount > 120)
                iter->amount = 120;
            return;
        }
    }
    PlayerInventory item(item_id);
    item.amount = amount;
    player->bank.insert(player->bank.end(), item);
}

bool Players::Player_RemoveBankItem(Players *self, Player *player, int item_id)
{
    player->bank_dirty = 1;
    for (PlayerInventory *iter = player->bank.begin(); iter != player->bank.end(); iter++)
    {
        if (iter->item_id == item_id)
        {
            player->item_change_id = iter->item_id;
            player->item_change_count = iter->amount;
            player->bank.erase(iter);
            return true;
        }
    }
    return false;
}

bool Players::Player_AddTradeItem(Players *self, Player *player, int item_id, int amount)
{
    if (player->trade_items.size() > 9)
        return false;
    for (PlayerInventory *iter = player->trade_items.begin();
         iter != player->trade_items.end();
         iter++)
    {
        if (iter->item_id == item_id)
        {
            player->trade_accepted = false;
            iter->amount = amount;
            return true;
        }
    }
    PlayerInventory item(item_id);
    item.amount = amount;
    player->trade_accepted = false;
    player->trade_items.insert(player->trade_items.end(), item);
    return true;
}

bool Players::Player_RemoveTradeItem(Players *self, Player *player, int item_id)
{
    if (player->trade_accepted != false)
        return false;
    if (player->trade_items.size() > 5)
        return false;
    for (PlayerInventory *iter = player->trade_items.begin();
         iter != player->trade_items.end();
         iter++)
    {
        if (iter->item_id == item_id)
        {
            player->trade_items.erase(iter);
            return true;
        }
    }
    return false;
}

void Players::Player_AddSpell(Players *self, Player *player, int spell_id)
{
    for (PlayerSkill *iter = player->spells.begin(); iter != player->spells.end(); iter++)
    {
        if (iter->skill_id == spell_id)
            return;
    }
    PlayerSkill spell(spell_id);
    spell.level = 0;
    player->spells.insert(player->spells.end(), spell);
}

bool Players::Players_HasField0C(Players *self, int field_c)
{
    for (Player **iter = self->players.begin(); iter != self->players.end(); iter++)
    {
        if ((*iter)->field_0xc == field_c)
            return true;
    }
    return false;
}

int Players::Players_CountGuildInvites(Players *self, Player *player)
{
    int count = 0;
    for (Player **iter = self->players.begin(); iter != self->players.end(); iter++)
    {
        if ((*iter)->guild_inviter_id == player->player_id &&
            (*iter)->player_id != player->player_id)
            count = count + 1;
    }
    return count;
}

int Players::Players_CountGuildOnMap(Players *self, Player *player)
{
    int count;
    for (Player **iter = self->players.begin(); iter != self->players.end(); iter++)
    {
        if ((*iter)->map_id == player->map_id && (*iter)->guild_tag.Length() < 2 &&
            (*iter)->player_id != player->player_id)
            count = count + 1;
    }
    return count;
}

void Players::Players_UpdatePeakOnline(Players *self)
{
    self->idle_timeout = 0;
    for (Player **iter = self->players.begin(); iter != self->players.end(); iter++)
    {
        if ((*iter)->logged_in != false)
            self->idle_timeout = self->idle_timeout + 1;
    }
    if (self->idle_timeout > self->stat_total)
        self->stat_total = self->idle_timeout;
}

void Players::Players_GuildSetMemberInfo(Players *self,
                                         Player *player,
                                         String guild_name,
                                         String guild_tag)
{
    int count;
    for (Player **iter = self->players.begin(); iter != self->players.end(); iter++)
    {
        if ((*iter)->guild_inviter_id == player->player_id &&
            (*iter)->player_id != player->player_id)
        {
            (*iter)->guild_tag = guild_tag;
            (*iter)->guild_name = guild_name;
            (*iter)->guild_rank_name = "";
            (*iter)->guild_rank_id = 9;
            (*iter)->guild_inviter_id = -1;
            count = count + 1;
            if (count > 10)
                return;
        }
    }
}

bool Players::Players_IsAccountNameTaken(Players *self,
                                         String account_name,
                                         int player_id)
{
    if (account_name.Length() < 3)
        return false;
    for (Player **iter = self->players.begin(); iter != self->players.end(); iter++)
    {
        if (account_name == (*iter)->account_name && (*iter)->player_id != player_id)
            return true;
    }
    return false;
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
