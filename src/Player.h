#ifndef PlayerH
#define PlayerH

#include <Classes.hpp>
#include <SysUtils.hpp>
#include <ScktComp.hpp>
#include <vector>

#include "Playerinventory.h"
#include "Playerquest.h"
#include "Playercommand.h"
#include "Playerskill.h"

int RandRange(int max);

// Layout recovered from the reference (Player unit, 0x410e04..0x411f64).
// sizeof is 0x3f8, pinned by the `operator new(0x3f8)` at 0x4082d5 and by the
// destructor's member-destruction counter (0x1c = 28 destructible members).
// Offsets are pinned by Player_Init's stores, the destructor, Player_HpPercent,
// the party helpers, Player_UpdateBaseStats and Player_CalculateHP_TP_SP.
// Gaps Ghidra could not type are filled with explicit padding so every field
// lands on its observed offset.
class Player
{
  public:
    bool connected;                           // +0x000
    bool initialized;                         // +0x001
    bool removing;                            // +0x002
    bool account_logged_in;                   // +0x003
    bool logged_in;                           // +0x004
    bool ping_timeout;                        // +0x005
    char pad_06[2];                           // +0x006
    int player_id;                            // +0x008
    int field_0xc;                            // +0x00c
    int query_id;                             // +0x010
    String field_0x14;                        // +0x014
    String signup;                            // +0x018
    int server_encryption_multiple;           // +0x01c
    int client_encryption_multiple;           // +0x020
    int sequence;                             // +0x024
    int account_create_cooldown;              // +0x028
    int field_0x2c;                           // +0x02c
    int session_id;                           // +0x030
    int field_0x34;                           // +0x034
    String receive_buffer;                    // +0x038
    String hdid;                              // +0x03c
    String remote_ip;                         // +0x040
    String account_name;                      // +0x044
    String field_0x48;                        // +0x048
    int packet_count;                         // +0x04c
    int remove_timer;                         // +0x050
    bool arena_queued;                        // +0x054
    bool arena_playing;                       // +0x055
    char pad_56[2];                           // +0x056
    int arena_kills;                          // +0x058
    char bank_dirty;                          // +0x05c
    char inventory_dirty;                     // +0x05d
    char equipment_dirty;                     // +0x05e
    bool base_stats_dirty;                    // +0x05f
    int character_id;                         // +0x060
    int class_id;                             // +0x064
    int account_id;                           // +0x068
    int guild_rank_id;                        // +0x06c
    int read_pos;                             // +0x070
    int read_len;                             // +0x074
    int read_break;                           // +0x078
    int npc_index;                            // +0x07c
    int field_0x80;                           // +0x080
    int session_token;                        // +0x084
    int guild_inviter_id;                     // +0x088
    String field_0x8c;                        // +0x08c
    bool trade_accepted;                      // +0x090
    char pad_91[3];                           // +0x091
    int trade_value;                          // +0x094
    int admin_level;                          // +0x098
    int hangup_ticks;                         // +0x09c
    int recover_ticks;                        // +0x0a0
    char field_0xa4;                          // +0x0a4
    char pad_a5[3];                           // +0x0a5
    String name;                              // +0x0a8
    String partner_name;                      // +0x0ac
    String title;                             // +0x0b0
    String guild_rank_name;                   // +0x0b4
    String guild_tag;                         // +0x0b8
    String guild_name;                        // +0x0bc
    int experience;                           // +0x0c0
    int level;                                // +0x0c4
    bool show_players;                        // +0x0c8
    char pad_c9[3];                           // +0x0c9
    int gender;                               // +0x0cc
    int hair_style;                           // +0x0d0
    int hair_color;                           // +0x0d4
    int skin;                                 // +0x0d8
    int map_id;                               // +0x0dc
    int x;                                    // +0x0e0
    int y;                                    // +0x0e4
    int direction;                            // +0x0e8
    int warp_map;                             // +0x0ec
    int warp_state;                           // +0x0f0
    short warp_x;                             // +0x0f4
    short warp_y;                             // +0x0f6
    int target_map;                           // +0x0f8
    short target_x;                           // +0x0fc
    short target_y;                           // +0x0fe
    bool warp_pending;                        // +0x100
    bool map_switch_pending;                  // +0x101
    char pad_102[2];                          // +0x102
    int base_hp;                              // +0x104
    int max_hp;                               // +0x108
    int hp;                                   // +0x10c
    int base_tp;                              // +0x110
    int max_tp;                               // +0x114
    int tp;                                   // +0x118
    int base_sp;                              // +0x11c
    int max_sp;                               // +0x120
    int usage;                                // +0x124
    int home_id;                              // +0x128
    String home_name;                         // +0x12c
    int money_bank;                           // +0x130
    int locker_bank;                          // +0x134
    int karma;                                // +0x138
    int weight_current;                       // +0x13c
    int weight_max;                           // +0x140
    int stat_points;                          // +0x144
    int skill_points;                         // +0x148
    short min_damage;                         // +0x14c
    short max_damage;                         // +0x14e
    short accuracy;                           // +0x150
    short evasion;                            // +0x152
    short armor;                              // +0x154
    short class_min_damage;                   // +0x156
    short class_max_damage;                   // +0x158
    short class_accuracy;                     // +0x15a
    short class_evasion;                      // +0x15c
    short class_armor;                        // +0x15e
    short element_resistances[7];             // +0x160
    char pad_16e[2];                          // +0x16e
    int equip_bonus_hp;                       // +0x170
    int equip_bonus_tp;                       // +0x174
    int equip_strength_bonus;                 // +0x178
    int equip_wisdom_bonus;                   // +0x17c
    int equip_intelligence_bonus;             // +0x180
    int equip_agility_bonus;                  // +0x184
    int equip_constitution_bonus;             // +0x188
    int equip_charisma_bonus;                 // +0x18c
    int base_strength;                        // +0x190
    int base_wisdom;                          // +0x194
    int base_intelligence;                    // +0x198
    int base_agility;                         // +0x19c
    int base_constitution;                    // +0x1a0
    int base_charisma;                        // +0x1a4
    int adj_strength;                         // +0x1a8
    int adj_wisdom;                           // +0x1ac
    int adj_intelligence;                     // +0x1b0
    int adj_agility;                          // +0x1b4
    int adj_constitution;                     // +0x1b8
    int adj_charisma;                         // +0x1bc
    int boots_graphic_id;                     // +0x1c0
    int accessory_graphic_id;                 // +0x1c4
    int gloves_graphic_id;                    // +0x1c8
    int armor_graphic_id;                     // +0x1cc
    int belt_graphic_id;                      // +0x1d0
    int necklace_graphic_id;                  // +0x1d4
    int hat_graphic_id;                       // +0x1d8
    int shield_graphic_id;                    // +0x1dc
    int weapon_graphic_id;                    // +0x1e0
    int ring1_graphic_id;                     // +0x1e4
    int ring2_graphic_id;                     // +0x1e8
    int armlet1_graphic_id;                   // +0x1ec
    int armlet2_graphic_id;                   // +0x1f0
    int bracer1_graphic_id;                   // +0x1f4
    int bracer2_graphic_id;                   // +0x1f8
    int boots_item_id;                        // +0x1fc
    int accessory_item_id;                    // +0x200
    int gloves_item_id;                       // +0x204
    int armor_item_id;                        // +0x208
    int belt_item_id;                         // +0x20c
    int necklace_item_id;                     // +0x210
    int hat_item_id;                          // +0x214
    int shield_item_id;                       // +0x218
    int weapon_item_id;                       // +0x21c
    int ring1_item_id;                        // +0x220
    int ring2_item_id;                        // +0x224
    int armlet1_item_id;                      // +0x228
    int armlet2_item_id;                      // +0x22c
    int bracer1_item_id;                      // +0x230
    int bracer2_item_id;                      // +0x234
    std::vector<PlayerInventory> inventory;   // +0x238
    std::vector<PlayerInventory> trade_items; // +0x258
    std::vector<PlayerInventory> bank;        // +0x278
    std::vector<PlayerSkill> spells;          // +0x298
    std::vector<PlayerQuest> quest_trackers;  // +0x2b8
    std::vector<PlayerQuest> quest_history;   // +0x2d8
    bool in_party;                            // +0x2f8
    char pad_2f9[3];                          // +0x2f9
    int party_leader_id;                      // +0x2fc
    int party_ids[10];                        // +0x300
    String invblob1;                          // +0x328
    String invblob2;                          // +0x32c
    String skillblob;                         // +0x330
    String quest_cache;                       // +0x334
    String quest_blob;                        // +0x338
    union
    {
        struct
        {
            Player *character_slot_0; // +0x33c
            Player *character_slot_1; // +0x340
            Player *character_slot_2; // +0x344
        };
        Player *character_slots[3]; // +0x33c
    };
    char stats_dirty;                        // +0x348
    bool dead;                               // +0x349
    char pad_34a[2];                         // +0x34a
    int last_client_walk_tick;               // +0x34c
    int sync_base_ahead;                     // +0x350
    int sync_base_behind;                    // +0x354
    TTimeStamp last_pass_ms;                 // +0x358
    int ghost_walk_tokens;                   // +0x360
    int ghost_token_ticks;                   // +0x364
    int attack_tokens;                       // +0x368
    int attack_token_ticks;                  // +0x36c
    int queued_spell_id;                     // +0x370
    int expected_cast_timestamp;             // +0x374
    int field_0x378;                         // +0x378
    int field_0x37c;                         // +0x37c
    int drop_counter;                        // +0x380
    int walk_tick;                           // +0x384
    int field_0x388;                         // +0x388
    std::vector<PlayerCommand> action_queue; // +0x38c
    char fast_action;                        // +0x3ac
    char flush_queue;                        // +0x3ad
    char pad_3ae[2];                         // +0x3ae
    TCustomWinSocket *socket;                // +0x3b0 (socket object pointer)
    char pad_3b4[12];                        // +0x3b4
    String null_string;                      // +0x3c0
    int item_change_id;                      // +0x3c4
    int item_change_count;                   // +0x3c8
    int item_change_remaining;               // +0x3cc
    int item_change_amount;                  // +0x3d0
    int equip_result;                        // +0x3d4
    int equip_result_count;                  // +0x3d8
    bool map_has_quakes;                     // +0x3dc
    bool map_has_hp_drain;                   // +0x3dd
    bool map_has_tp_drain;                   // +0x3de
    bool map_has_spikes;                     // +0x3df
    int idle_ticks;                          // +0x3e0
    bool on_chair;                           // +0x3e4
    bool sitting;                            // +0x3e5
    bool hidden;                             // +0x3e6
    bool hide_online;                        // +0x3e7
    bool cheater_flag;                       // +0x3e8
    bool global_chat;                        // +0x3e9
    char pad_3ea[6];                         // +0x3ea
    TDateTime enter_game_timestamp;          // +0x3f0

    Player(TCustomWinSocket *socket);
    ~Player();

    static int HpPercent(Player *self);
    int CountPartyMembers();
    static void AddPartyMember(Player *self, int member_id);
    static char IsPartyMember(Player *self, int player_id);
    static void ClearPartyRoster(Player *self);
    static void UpdateBaseStats(Player *self);
    static void CalculateHP_TP_SP(Player *self);
};

#endif
