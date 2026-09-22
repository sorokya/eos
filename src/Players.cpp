#include <vcl.h>
#pragma hdrstop

#include <stdlib.h>

#include "Players.h"
#include "MainForm.h"
#include "Gamecontrol.h"
#include "Itemvalues.h"
#include "Settings.h"
#include "Mysqlcontrols.h"
#include "Filecache.h"
#include "Protocol.h"
#include "Packets.h"

#pragma package(smart_init)

#define PLAYER_UNEQUIP_SLOT(slot, graphic, set_flag)                                     \
    if (ItemValues::GetSpecial(GUI->item_values, player->slot) == 5)                     \
    {                                                                                    \
        ItemValue *item = ItemValues::GetByIndex(GUI->item_values, player->slot - 1);    \
        if (item->element < 7)                                                           \
            player->element_resistances[item->element] =                                 \
                player->element_resistances[item->element] + item->element_damage;       \
        player->min_damage -= item->min_damage;                                          \
        player->max_damage -= item->max_damage;                                          \
        player->accuracy -= item->accuracy;                                              \
        player->evasion -= item->evade;                                                  \
        player->armor -= item->armor;                                                    \
        player->equip_bonus_hp -= item->hp;                                              \
        player->equip_bonus_tp -= item->tp;                                              \
        player->equip_strength_bonus -= item->strength;                                  \
        player->equip_wisdom_bonus -= item->wisdom;                                      \
        player->equip_intelligence_bonus -= item->intelligence;                          \
        player->equip_agility_bonus -= item->agility;                                    \
        player->equip_constitution_bonus -= item->constitution;                          \
        player->equip_charisma_bonus -= item->charisma;                                  \
        player->slot = 0;                                                                \
        player->graphic = 0;                                                             \
        set_flag;                                                                        \
    }

#define EQUIP_SLOT_TYPE(itype, slot, graphic, result)                                    \
    if (type == itype && player->slot <= 0)                                              \
    {                                                                                    \
        player->equip_result = result;                                                   \
        player->slot = item_id;                                                          \
        player->graphic = ItemValues::GetSpec1ForTypes(GUI->item_values, item_id);       \
    }

#define EQUIP_SLOT_ID(itype, slot, graphic, result)                                      \
    if (type == itype && player->slot <= 0)                                              \
    {                                                                                    \
        player->equip_result = result;                                                   \
        player->slot = item_id;                                                          \
        player->graphic = item_id;                                                       \
    }

#define EQUIP_SLOT_PAIR(itype, field, graphic, index)                                    \
    if (type == itype && player->field <= 0 && slot == index)                            \
    {                                                                                    \
        player->equip_result = 1;                                                        \
        player->field = item_id;                                                         \
        player->graphic = item_id;                                                       \
    }

#define UNEQUIP_SLOT(itype, field, graphic, result)                                      \
    if (type == itype && player->field == item_id)                                       \
    {                                                                                    \
        player->field = 0;                                                               \
        player->graphic = 0;                                                             \
        player->equip_result = result;                                                   \
    }

#define UNEQUIP_SLOT_PAIR(itype, field, graphic, index)                                  \
    if (type == itype && player->field == item_id && slot == index)                      \
    {                                                                                    \
        player->field = 0;                                                               \
        player->graphic = 0;                                                             \
        player->equip_result = 1;                                                        \
    }

Players::Players(Settings *settings, mySQLdb *mysql_controls)
{
    idle_timeout = 0;
    stat_total = 0;
    dirty = 0;
    this->settings = settings;
    this->mysql_controls = mysql_controls;
    for (int i = 0; i < SOCKET_HANDLE_MAX; i++)
        by_id[i] = 0;
}

Players::~Players()
{
}

void Players::Players_Tick(Players *self)
{
    while (self->dirty)
    {
        self->dirty = 0;
        for (Player **iter = self->players.begin(); iter != self->players.end(); iter++)
        {
            if ((*iter)->removing)
            {
                ((TCustomWinSocket *)(*iter)->socket)->Close();
                self->dirty = 1;
                break;
            }
        }
    }
    Player **iter;
    TTimeStamp now = DateTimeToTimeStamp(Now());
    for (iter = self->players.begin(); iter != self->players.end(); iter++)
    {
        if ((*iter)->removing == false && (*iter)->flush_queue != false)
        {
            (*iter)->flush_queue = 0;
            (*iter)->action_queue.clear();
            continue;
        }
        if ((*iter)->removing)
            continue;
        if ((*iter)->action_queue.size() <= 0)
            continue;
        int delta = now.Time - (*iter)->walk_tick;
        if (delta > 7500000)
            delta = 300;
        if (delta < 300)
        {
            if (delta < 100)
                continue;
            if ((*iter)->fast_action == 0)
                continue;
        }
        Packets *server = Mainform_GetServer(GUI);
        (*iter)->flush_queue = 0;
        (*iter)->fast_action = 0;
        if ((*iter)->action_queue[0].family == PacketFamily_Walk)
            Walk_Execute(server,
                         *iter,
                         (*iter)->action_queue[0].action,
                         &(*iter)->action_queue[0].text);
        if ((*iter)->action_queue[0].family == PacketFamily_Attack)
            Attack_Execute(server,
                           *iter,
                           (*iter)->action_queue[0].action,
                           &(*iter)->action_queue[0].text);
        if ((*iter)->action_queue[0].family == PacketFamily_Spell)
            Spell_Execute(server,
                          *iter,
                          (*iter)->action_queue[0].action,
                          &(*iter)->action_queue[0].text);
        if ((*iter)->action_queue[0].family == PacketFamily_Chair)
            Chair_Execute(server,
                          *iter,
                          (*iter)->action_queue[0].action,
                          &(*iter)->action_queue[0].text);
        if ((*iter)->action_queue[0].family == PacketFamily_Face)
        {
            (*iter)->fast_action = 1;
            Face_Execute(server,
                         *iter,
                         (*iter)->action_queue[0].action,
                         &(*iter)->action_queue[0].text);
        }
        if ((*iter)->flush_queue == 0)
            (*iter)->action_queue.erase((*iter)->action_queue.begin());
        else
        {
            (*iter)->flush_queue = 0;
            (*iter)->action_queue.clear();
        }
    }
}

bool Players::Players_Add(Players *self, TCustomWinSocket *socket)
{
    if (socket->SocketHandle >= SOCKET_HANDLE_MAX)
        return false;
    if (self->by_id[socket->SocketHandle] != 0)
        return false;
    int same_ip = 0;
    for (Player **iter = self->players.begin(); iter != self->players.end(); iter++)
    {
        if ((*iter)->socket->RemoteAddress != socket->RemoteAddress)
            continue;
        same_ip++;
        if (Settings::GetMaxClones(self->settings) <= same_ip)
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

void Players::Players_Remove(Players *self, TCustomWinSocket *socket)
{
    if (self->by_id[socket->SocketHandle] != 0)
    {
        Player *player = self->by_id[socket->SocketHandle];
        for (int i = 0; i < 3; i++)
        {
            if (player->character_slots[i] != 0)
            {
                Player *slot = player->character_slots[i];
                player->character_slots[i] = 0;
                delete slot;
            }
        }
        if (player->logged_in != false)
        {
            if (self->idle_timeout > 0)
                self->idle_timeout = self->idle_timeout - 1;
            mySQLdb::Mysql_ExecDirect(self->mysql_controls,
                                      player->account_ident,
                                      Character_BuildSaveQuery(self, player, 0));
            player->trade_items.clear();
            player->inventory.clear();
            player->bank.clear();
            player->spells.clear();
            Player::ClearPartyRoster(player);
            player->action_queue.clear();
            player->quest_trackers.clear();
            player->quest_history.clear();
        }
        for (Player **iter = self->players.begin(); iter != self->players.end(); iter++)
        {
            if ((*iter)->player_id == socket->SocketHandle)
            {
                self->players.erase(iter);
                break;
            }
        }
        self->by_id[socket->SocketHandle] = 0;
        delete player;
    }
}

String Character_BuildSaveQuery(Players *self, Player *player, int flags)
{
    if (player->logged_in == false)
        return "";
    TTimeStamp enter = DateTimeToTimeStamp(player->enter_game_timestamp);
    TTimeStamp now = DateTimeToTimeStamp(Now());
    int date_diff = now.Date - enter.Date;
    int time_diff = now.Time - enter.Time;
    date_diff = time_diff / 60000 + date_diff * 1440;
    player->usage += date_diff;
    String invblob = "";
    String skillblob = "";
    String bankblob = "";
    String questcache = "";
    String questblob = "";
    PlayerInventory *inv_it;
    for (inv_it = player->inventory.begin(); inv_it != player->inventory.end(); inv_it++)
    {
        if (inv_it->item_id <= 0)
            continue;
        invblob.Insert(IntToStr(inv_it->item_id), invblob.Length() + 1);
        invblob.Insert(":", invblob.Length() + 1);
        invblob.Insert(IntToStr(inv_it->amount), invblob.Length() + 1);
        invblob.Insert(":", invblob.Length() + 1);
    }
    PlayerInventory *bank_it;
    for (bank_it = player->bank.begin(); bank_it != player->bank.end(); bank_it++)
    {
        if (bank_it->item_id <= 0)
            continue;
        bankblob.Insert(IntToStr(bank_it->item_id), bankblob.Length() + 1);
        bankblob.Insert(":", bankblob.Length() + 1);
        bankblob.Insert(IntToStr(bank_it->amount), bankblob.Length() + 1);
        bankblob.Insert(":", bankblob.Length() + 1);
    }
    PlayerSkill *spell_it;
    for (spell_it = player->spells.begin(); spell_it != player->spells.end(); spell_it++)
    {
        if (spell_it->skill_id <= 0)
            continue;
        skillblob.Insert(IntToStr(spell_it->skill_id), skillblob.Length() + 1);
        skillblob.Insert(":", skillblob.Length() + 1);
        skillblob.Insert(IntToStr(spell_it->level), skillblob.Length() + 1);
        skillblob.Insert(":", skillblob.Length() + 1);
    }
    PlayerQuest *quest_it;
    for (quest_it = player->quest_trackers.begin();
         quest_it != player->quest_trackers.end();
         quest_it++)
    {
        if (quest_it->quest_id > 0)
        {
            questcache.Insert(IntToStr(quest_it->quest_id), questcache.Length() + 1);
            questcache.Insert(":", questcache.Length() + 1);
            questcache.Insert(IntToStr(quest_it->state_index), questcache.Length() + 1);
            questcache.Insert(":", questcache.Length() + 1);
            questcache.Insert(IntToStr(quest_it->version), questcache.Length() + 1);
            questcache.Insert(":", questcache.Length() + 1);
            for (int j = 0; j < 5; j++)
            {
                questcache.Insert(IntToStr(quest_it->counters[j]),
                                  questcache.Length() + 1);
                questcache.Insert(":", questcache.Length() + 1);
            }
        }
    }
    for (quest_it = player->quest_history.begin();
         quest_it != player->quest_history.end();
         quest_it++)
    {
        if (quest_it->quest_id > 0)
        {
            questblob.Insert(IntToStr(quest_it->quest_id), questblob.Length() + 1);
            questblob.Insert(":", questblob.Length() + 1);
        }
    }
    invblob = invblob + "EOF";
    skillblob = skillblob + "EOF";
    bankblob = bankblob + "EOF";
    questcache = questcache + "EOF";
    questblob = questblob + "EOF";
    String invblob2;
    String bankblob2;
    String skillblob2;
    String questblob2;
    if (invblob.Length() > 255)
    {
        invblob2 = invblob.SubString(256, invblob.Length() - 255);
        invblob.Delete(256, invblob.Length() - 255);
    }
    if (bankblob.Length() > 255)
    {
        bankblob2 = bankblob.SubString(256, bankblob.Length() - 255);
        bankblob.Delete(256, bankblob.Length() - 255);
    }
    if (skillblob.Length() > 255)
    {
        skillblob2 = skillblob.SubString(256, skillblob.Length() - 255);
        skillblob.Delete(256, invblob.Length() - 255);
    }
    if (questblob.Length() > 255)
    {
        questblob2 = questblob.SubString(256, questblob.Length() - 255);
        questblob.Delete(256, questblob.Length() - 255);
    }
    String q = "UPDATE endl_characters SET ";
    q.Insert("ident_class = " + IntToStr(player->class_id), q.Length() + 1);
    q.Insert(",ident_rank = " + IntToStr(player->guild_rank_id), q.Length() + 1);
    q.Insert(",ident_guild = '" + player->guild_tag + "'", q.Length() + 1);
    q.Insert(",partner = '" + player->partner_name + "'", q.Length() + 1);
    q.Insert(",title = '" + player->title + "'", q.Length() + 1);
    q.Insert(",guild = '" + player->guild_name + "'", q.Length() + 1);
    q.Insert(",rank = '" + player->guild_rank_name + "'", q.Length() + 1);
    q.Insert(",citizenship = '" + IntToStr(player->home_id) + "'", q.Length() + 1);
    q.Insert(",experience = " + IntToStr(player->experience), q.Length() + 1);
    q.Insert(",level = " + IntToStr(player->level), q.Length() + 1);
    q.Insert(",stat_points = " + IntToStr(player->stat_points), q.Length() + 1);
    q.Insert(",skill_points = " + IntToStr(player->skill_points), q.Length() + 1);
    q.Insert(",gender = " + IntToStr(player->gender), q.Length() + 1);
    q.Insert(",hairmodal = " + IntToStr(player->hair_style), q.Length() + 1);
    q.Insert(",haircolor = " + IntToStr(player->hair_color), q.Length() + 1);
    q.Insert(",skincolor = " + IntToStr(player->skin), q.Length() + 1);
    q.Insert(",nav_map = " + IntToStr(player->map_id), q.Length() + 1);
    q.Insert(",nav_x = " + IntToStr(player->x), q.Length() + 1);
    q.Insert(",nav_y = " + IntToStr(player->y), q.Length() + 1);
    q.Insert(",nav_direction = " + IntToStr(player->direction), q.Length() + 1);
    q.Insert(",hp_max = " + IntToStr(player->base_hp), q.Length() + 1);
    q.Insert(",hp_now = " + IntToStr(player->hp), q.Length() + 1);
    q.Insert(",mp_max = " + IntToStr(player->base_tp), q.Length() + 1);
    q.Insert(",mp_now = " + IntToStr(player->tp), q.Length() + 1);
    q.Insert(",sp_max = " + IntToStr(player->base_sp), q.Length() + 1);
    if (player->base_stats_dirty != false ||
        Settings::GetSqlSmart(self->settings) == false)
    {
        q.Insert(",stat_strenght = " + IntToStr(player->base_strength), q.Length() + 1);
        q.Insert(",stat_wisdom = " + IntToStr(player->base_wisdom), q.Length() + 1);
        q.Insert(",stat_intelligence = " + IntToStr(player->base_intelligence),
                 q.Length() + 1);
        q.Insert(",stat_agility = " + IntToStr(player->base_agility), q.Length() + 1);
        q.Insert(",stat_constitution = " + IntToStr(player->base_constitution),
                 q.Length() + 1);
        q.Insert(",stat_charisma = " + IntToStr(player->base_charisma), q.Length() + 1);
        player->base_stats_dirty = false;
    }
    if (player->money_bank < 0)
        player->money_bank = 0;
    q.Insert(",clientusge = " + IntToStr(player->usage), q.Length() + 1);
    q.Insert(",money_bank = " + IntToStr(player->money_bank), q.Length() + 1);
    q.Insert(",locker_bank = " + IntToStr(player->locker_bank), q.Length() + 1);
    q.Insert(",alignment_good = " + IntToStr(player->karma), q.Length() + 1);
    if (player->equipment_dirty != false ||
        Settings::GetSqlSmart(self->settings) == false)
    {
        q.Insert(",eq_boots = " + IntToStr(player->boots_item_id), q.Length() + 1);
        q.Insert(",eq_pants = " + IntToStr(player->accessory_item_id), q.Length() + 1);
        q.Insert(",eq_gloves = " + IntToStr(player->gloves_item_id), q.Length() + 1);
        q.Insert(",eq_armor = " + IntToStr(player->armor_item_id), q.Length() + 1);
        q.Insert(",eq_belt = " + IntToStr(player->belt_item_id), q.Length() + 1);
        q.Insert(",eq_necklage = " + IntToStr(player->necklace_item_id), q.Length() + 1);
        q.Insert(",eq_hat = " + IntToStr(player->hat_item_id), q.Length() + 1);
        q.Insert(",eq_shield = " + IntToStr(player->shield_item_id), q.Length() + 1);
        q.Insert(",eq_weapon = " + IntToStr(player->weapon_item_id), q.Length() + 1);
        q.Insert(",eq_ring_l = " + IntToStr(player->ring1_item_id), q.Length() + 1);
        q.Insert(",eq_ring_r = " + IntToStr(player->ring2_item_id), q.Length() + 1);
        q.Insert(",eq_armlet_l = " + IntToStr(player->armlet1_item_id), q.Length() + 1);
        q.Insert(",eq_armlet_r = " + IntToStr(player->armlet2_item_id), q.Length() + 1);
        q.Insert(",eq_bracer_l = " + IntToStr(player->bracer1_item_id), q.Length() + 1);
        q.Insert(",eq_bracer_r = " + IntToStr(player->bracer2_item_id), q.Length() + 1);
        player->equipment_dirty = 0;
    }
    if (player->inventory_dirty != false ||
        Settings::GetSqlSmart(self->settings) == false)
    {
        q.Insert(",invblob = '" + invblob + "'", q.Length() + 1);
        q.Insert(",invblob2 = '" + invblob2 + "'", q.Length() + 1);
        player->inventory_dirty = 0;
    }
    if (player->bank_dirty != false || Settings::GetSqlSmart(self->settings) == false)
    {
        q.Insert(",invblob3 = '" + bankblob + "'", q.Length() + 1);
        q.Insert(",invblob4 = '" + bankblob2 + "'", q.Length() + 1);
        player->bank_dirty = 0;
    }
    q.Insert(",skillblob = '" + skillblob + "'", q.Length() + 1);
    q.Insert(",skillblob2 = '" + skillblob2 + "'", q.Length() + 1);
    q.Insert(",questcache = '" + questcache + "'", q.Length() + 1);
    q.Insert(",questblob = '" + questblob + "'", q.Length() + 1);
    q.Insert(",questblob2 = '" + questblob2 + "'", q.Length() + 1);
    int sit = 0;
    if (player->on_chair != false)
        sit = 1;
    if (player->sitting != false)
        sit = 2;
    q.Insert(",sitting = " + IntToStr(sit), q.Length() + 1);
    q.Insert(",online = " + IntToStr(flags), q.Length() + 1);
    q.Insert(" WHERE ident = " + IntToStr(player->character_id), q.Length() + 1);
    q.Insert(" AND ident_account = " + IntToStr(player->account_id), q.Length() + 1);
    FileCache::UpdatePlayerCache(self->mysql_controls->file_cache, (char *)player);
    return q;
}

bool Players::Players_IsAccountIdentOnline(Players *self, int field_c)
{
    for (Player **iter = self->players.begin(); iter != self->players.end(); iter++)
    {
        if ((*iter)->account_ident == field_c)
            return true;
    }
    return false;
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

bool Players::Players_IsPlayerAt(Players *self, int map_id, int x, int y)
{
    for (Player **iter = self->players.begin(); iter != self->players.end(); iter++)
    {
        if ((*iter)->x == x && (*iter)->y == y && (*iter)->map_id == map_id)
            return true;
    }
    return false;
}

bool Players::CharName_Validate(Players *self, Player *player, String name)
{
    Player **iter;
    if (player->null_string != "")
    {
        return true;
    }
    iter = self->players.begin();
    while (iter != self->players.end())
    {
        if (name == (*iter)->null_string)
            return true;
        iter++;
    }
    player->null_string = name;
    return false;
}

bool Players::Player_HasKeyItem(Players *self, Player *player, int key_item_id)
{
    for (PlayerInventory *iter = player->inventory.begin();
         iter != player->inventory.end();
         iter++)
    {
        if (ItemValues::GetType(GUI->item_values, iter->item_id) == ItemType_Key)
        {
            if (ItemValues::GetSpec1(GUI->item_values, iter->item_id) == key_item_id)
                return true;
        }
    }
    return false;
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

bool Players::Player_HasSpellId(Players *self, Player *player, int spell_id)
{
    for (PlayerSkill *iter = player->spells.begin(); iter != player->spells.end(); iter++)
    {
        if (iter->skill_id == spell_id)
            return true;
    }
    return false;
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

bool Players::Player_EquipItem(Players *self, Player *player, int item_id, int slot)
{
    player->equipment_dirty = 1;
    player->inventory_dirty = 1;
    player->equip_result = 0;
    int type = ItemValues::GetType(GUI->item_values, item_id);
    for (PlayerInventory *iter = player->inventory.begin();
         iter != player->inventory.end();
         iter++)
    {
        if (iter->item_id == item_id)
        {
            if ((unsigned int)iter->amount > 0)
            {
                player->equip_result = 0;
                EQUIP_SLOT_TYPE(ItemType_Weapon, weapon_item_id, weapon_graphic_id, 2)
                EQUIP_SLOT_TYPE(ItemType_Shield, shield_item_id, shield_graphic_id, 2)
                if (type == ItemType_Armor && player->armor_item_id <= 0 &&
                    ItemValues::GetGender(GUI->item_values, item_id) == player->gender)
                {
                    player->equip_result = 2;
                    player->armor_item_id = item_id;
                    player->armor_graphic_id =
                        ItemValues::GetSpec1ForTypes(GUI->item_values, item_id);
                }
                EQUIP_SLOT_TYPE(ItemType_Hat, hat_item_id, hat_graphic_id, 2)
                EQUIP_SLOT_TYPE(ItemType_Boots, boots_item_id, boots_graphic_id, 2)
                EQUIP_SLOT_ID(ItemType_Gloves, gloves_item_id, gloves_graphic_id, 2)
                EQUIP_SLOT_ID(
                    ItemType_Accessory, accessory_item_id, accessory_graphic_id, 1)
                EQUIP_SLOT_ID(ItemType_Belt, belt_item_id, belt_graphic_id, 1)
                EQUIP_SLOT_ID(ItemType_Necklace, necklace_item_id, necklace_graphic_id, 1)
                EQUIP_SLOT_PAIR(ItemType_Ring, ring1_item_id, ring1_graphic_id, 0)
                EQUIP_SLOT_PAIR(ItemType_Ring, ring2_item_id, ring2_graphic_id, 1)
                EQUIP_SLOT_PAIR(ItemType_Armlet, armlet1_item_id, armlet1_graphic_id, 0)
                EQUIP_SLOT_PAIR(ItemType_Armlet, armlet2_item_id, armlet2_graphic_id, 1)
                EQUIP_SLOT_PAIR(ItemType_Bracer, bracer1_item_id, bracer1_graphic_id, 0)
                EQUIP_SLOT_PAIR(ItemType_Bracer, bracer2_item_id, bracer2_graphic_id, 1)
                if (player->equip_result > 0)
                {
                    iter->amount = iter->amount - 1;
                    player->equip_result_count = iter->amount;
                    if ((unsigned int)iter->amount < 1)
                        player->inventory.erase(iter);
                    return 1;
                }
            }
            return 0;
        }
    }
    return 0;
}

bool Players::Player_UnequipItem(Players *self, Player *player, int item_id, int slot)
{
    player->equipment_dirty = 1;
    player->inventory_dirty = 1;
    player->equip_result = 0;
    int type = ItemValues::GetType(GUI->item_values, item_id);
    UNEQUIP_SLOT(ItemType_Weapon, weapon_item_id, weapon_graphic_id, 2)
    UNEQUIP_SLOT(ItemType_Shield, shield_item_id, shield_graphic_id, 2)
    UNEQUIP_SLOT(ItemType_Armor, armor_item_id, armor_graphic_id, 2)
    UNEQUIP_SLOT(ItemType_Hat, hat_item_id, hat_graphic_id, 2)
    UNEQUIP_SLOT(ItemType_Boots, boots_item_id, boots_graphic_id, 2)
    UNEQUIP_SLOT(ItemType_Gloves, gloves_item_id, gloves_graphic_id, 1)
    UNEQUIP_SLOT(ItemType_Accessory, accessory_item_id, accessory_graphic_id, 1)
    UNEQUIP_SLOT(ItemType_Belt, belt_item_id, belt_graphic_id, 1)
    UNEQUIP_SLOT(ItemType_Necklace, necklace_item_id, necklace_graphic_id, 1)
    UNEQUIP_SLOT_PAIR(ItemType_Ring, ring1_item_id, ring1_graphic_id, 0)
    UNEQUIP_SLOT_PAIR(ItemType_Ring, ring2_item_id, ring2_graphic_id, 1)
    UNEQUIP_SLOT_PAIR(ItemType_Armlet, armlet1_item_id, armlet1_graphic_id, 0)
    UNEQUIP_SLOT_PAIR(ItemType_Armlet, armlet2_item_id, armlet2_graphic_id, 1)
    UNEQUIP_SLOT_PAIR(ItemType_Bracer, bracer1_item_id, bracer1_graphic_id, 0)
    UNEQUIP_SLOT_PAIR(ItemType_Bracer, bracer2_item_id, bracer2_graphic_id, 1)
    if (player->equip_result > 0)
    {
        for (PlayerInventory *iter = player->inventory.begin();
             iter != player->inventory.end();
             iter++)
        {
            if (iter->item_id == item_id)
            {
                iter->amount = iter->amount + 1;
                player->equip_result_count = iter->amount;
                return true;
            }
        }
        PlayerInventory item(item_id);
        item.amount = 1;
        player->inventory.insert(player->inventory.end(), item);
        player->equip_result_count = 1;
        return true;
    }
    return false;
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

int Players::Player_LevelUpSpell(Players *self, Player *player, int spell_id)
{
    for (PlayerSkill *iter = player->spells.begin(); iter != player->spells.end(); iter++)
    {
        if (iter->skill_id == spell_id)
        {
            if ((unsigned int)iter->level < 100)
                iter->level++;
            return iter->level;
        }
    }
    return 0;
}

bool Players::Player_RemoveSpell(Players *self, Player *player, int spell_id)
{
    for (PlayerSkill *iter = player->spells.begin(); iter != player->spells.end(); iter++)
    {
        if (iter->skill_id == spell_id)
        {
            player->spells.erase(iter);
            return true;
        }
    }
    return false;
}

void Players::Player_ClearSpells(Players *self, Player *player)
{
    player->spells.clear();
}

char Players::Player_UnequipAll(Players *self, Player *player)
{
    char changed = 0;
    PLAYER_UNEQUIP_SLOT(weapon_item_id, weapon_graphic_id, changed = 1)
    PLAYER_UNEQUIP_SLOT(shield_item_id, shield_graphic_id, changed = 1)
    PLAYER_UNEQUIP_SLOT(armor_item_id, armor_graphic_id, changed = 1)
    PLAYER_UNEQUIP_SLOT(hat_item_id, hat_graphic_id, changed = 1)
    PLAYER_UNEQUIP_SLOT(boots_item_id, boots_graphic_id, changed = 1)
    PLAYER_UNEQUIP_SLOT(gloves_item_id, gloves_graphic_id, )
    PLAYER_UNEQUIP_SLOT(accessory_item_id, accessory_graphic_id, )
    PLAYER_UNEQUIP_SLOT(belt_item_id, belt_graphic_id, )
    PLAYER_UNEQUIP_SLOT(necklace_item_id, necklace_graphic_id, )
    PLAYER_UNEQUIP_SLOT(ring1_item_id, ring1_graphic_id, )
    PLAYER_UNEQUIP_SLOT(ring2_item_id, ring2_graphic_id, )
    PLAYER_UNEQUIP_SLOT(armlet1_item_id, armlet1_graphic_id, )
    PLAYER_UNEQUIP_SLOT(armlet2_item_id, armlet2_graphic_id, )
    PLAYER_UNEQUIP_SLOT(bracer1_item_id, bracer1_graphic_id, )
    PLAYER_UNEQUIP_SLOT(bracer2_item_id, bracer2_graphic_id, )
    return changed;
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

void Players::Party_AddNewMember(Players *self,
                                 Player *existing_member,
                                 Player *new_member)
{
    for (int i = 0; i < PARTY_MAX_MEMBERS; i++)
    {
        Player *player = Players_GetById(self, existing_member->party_ids[i]);
        if (player != 0)
            Player::AddPartyMember(player, new_member->player_id);
    }
    new_member->party_leader_id = existing_member->party_leader_id;
    Player::ClearPartyRoster(new_member);
    new_member->in_party = true;
    for (int i = 0; i < PARTY_MAX_MEMBERS; i++)
        Player::AddPartyMember(new_member, existing_member->party_ids[i]);
}

void Players::Player_LeaveParty(Players *self, Player *player)
{
    bool disband = false;
    player->in_party = false;
    if (player->player_id == player->party_leader_id || player->CountPartyMembers() < 3)
        disband = true;
    for (int i = 0; i < PARTY_MAX_MEMBERS; i++)
    {
        if (player->party_ids[i] != player->player_id)
        {
            Player *member = Players_GetById(self, player->party_ids[i]);
            if (member != 0)
            {
                if (disband)
                {
                    Player::ClearPartyRoster(member);
                    member->in_party = false;
                }
                else
                {
                    for (int j = 0; j < PARTY_MAX_MEMBERS; j++)
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

Player *Players::Players_GetById(Players *self, int player_id)
{
    if (player_id > 0 && player_id < SOCKET_HANDLE_MAX)
        return self->by_id[player_id];
    return 0;
}

Player *Players::Players_FindByName(Players *self, String name)
{
    for (Player **iter = self->players.begin(); iter != self->players.end(); iter++)
    {
        if (LowerCase((*iter)->name) == LowerCase(name))
            return *iter;
    }
    return 0;
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

int Players::Player_TryLevelUp(Players *self, Player *player)
{
    int new_level = 0;
    while ((int)player->experience >=
           Game::Exp_RequiredForLevel(GUI->game_control, player->level + 1))
    {
        Player_LevelUp(self, player);
        new_level = player->level;
    }
    return new_level;
}

void Players::Player_LevelUp(Players *self, Player *player)
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

int RandRange(int max)
{
    return (max ? (int)(_lrand() % max) : 0);
}

int Players::Players_GetActiveCount(Players *self)
{
    return self->players.size();
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
