#ifndef PlayersH
#define PlayersH

#include <Classes.hpp>
#include <SysUtils.hpp>
#include <ScktComp.hpp>
#include <vector>

#include "Player.h"

class Settings;
class Mysqlcontrols;
class Server;
class Players;

String Character_BuildSaveQuery(Players *players, Player *player, int flag);

// Player manager. Layout recovered from the reference (Players unit,
// 0x407948..0x410de4): a std::vector<Player *> at +0, the 100000-entry
// socket-handle index at +0x20, the Settings/Mysqlcontrols back-references at
// +0x61aa0/+0x61aa4, the removal/dirty flag at +0x61aa8, the idle-timeout
// counter at +0x61aac and the stat total at +0x61ab0. sizeof is 0x61ab4.
class Players
{
  public:
    std::vector<Player *> players; // +0x00000
    Player *by_id[100000];         // +0x00020
    Settings *settings;            // +0x61aa0
    Mysqlcontrols *mysql_controls; // +0x61aa4
    char dirty;                    // +0x61aa8
    char pad_61aa9[3];             // +0x61aa9
    int idle_timeout;              // +0x61aac
    int stat_total;                // +0x61ab0

    Players(Settings *settings, Mysqlcontrols *mysql_controls);

    static void Players_Tick(Players *self);
    static bool Players_Add(Players *self, TCustomWinSocket *socket);
    static void Players_MarkRemoving(Players *self, TCustomWinSocket *socket);
    static void Players_Remove(Players *self, TCustomWinSocket *socket);
    static int Players_ActiveCount(Players *self);
    static bool Players_IsPlayerAt(Players *self, int map_id, int x, int y);
    static bool CharName_Validate(Players *self, Player *player, String name);
    static bool Player_HasKeyItem(Players *self, Player *player, int key_item_id);
    static bool Player_HasSpellId(Players *self, Player *player, int spell_id);
    static int Players_GetItemAmount(Players *self, Player *player, int item_id);
    static int Players_CountArenaPlayers(Players *self, int map_id);
    static void Party_AddNewMember(Players *self, Player *member, Player *new_member);
    static void Player_LeaveParty(Players *self, Player *player);
    static void Player_RegenHpTp(Players *self, Player *player);
    static Player *Players_GetById(Players *self, int player_id);
    static Player *Players_FindByName(Players *self, String name);
    static Player *Players_GetByMapTile(Players *self, int map_id, int x, int y);
    static int Player_TryLevelUp(Server *server, Player *player);
    static void Player_LevelUp(Server *server, Player *player);
    static int Players_GetActiveCount(Players *self);
    static int Players_GetIdleTimeout(Players *self);
    static int Players_GetStatTotal(Players *self);
    static void Players_MarkDirty(Players *self);
    static void Player_AddItem(Players *self, Player *player, int item_id, int amount);
    static bool Player_RemoveItem(Players *self, Player *player, int item_id, int amount);
    static int Player_GetSpellLevel(Players *self, Player *player, int spell_id);
    static bool Player_HasBankItem(Players *self, Player *player, int item_id);
    static void
    Player_RemoveItemNoQuestRules(Players *self, Player *player, int item_id, int amount);
    static void
    Player_AddBankItem(Players *self, Player *player, int item_id, int amount);
    static bool Player_RemoveBankItem(Players *self, Player *player, int item_id);
    static bool
    Player_AddTradeItem(Players *self, Player *player, int item_id, int amount);
    static bool Player_RemoveTradeItem(Players *self, Player *player, int item_id);
    static void Player_AddSpell(Players *self, Player *player, int spell_id);
    static bool Players_HasField0C(Players *self, int field_c);
    static int Players_CountGuildInvites(Players *self, Player *player);
    static int Players_CountGuildOnMap(Players *self, Player *player);
    static void Players_UpdatePeakOnline(Players *self);
    static void Players_GuildSetMemberInfo(Players *self,
                                           Player *player,
                                           String guild_name,
                                           String guild_tag);
    static bool
    Players_IsAccountNameTaken(Players *self, String account_name, int player_id);
    static char Player_UnequipAll(Players *self, Player *player);
    static bool Player_EquipItem(Players *self, Player *player, int item_id, int slot);
    static bool Player_UnequipItem(Players *self, Player *player, int item_id, int slot);
};
#endif
