#include <vcl.h>
#pragma hdrstop

#include "Player.h"
#include "Players.h"
#include "Protocol.h"

#pragma package(smart_init)

Player::Player(TCustomWinSocket *socket)
{
    this->socket = socket;
    player_id = *(int *)((char *)socket + 4);
    remote_ip = socket->RemoteAddress;
    character_slot_0 = 0;
    character_slot_1 = 0;
    character_slot_2 = 0;
    in_party = false;
    hidden = false;
    hide_online = false;
    ClearPartyRoster(this);
    initialized = false;
    removing = false;
    connected = false;
    account_logged_in = false;
    logged_in = false;
    map_switch_pending = false;
    arena_playing = false;
    arena_queued = false;
    global_chat = false;
    admin_level = AdminLevel_Player;
    account_create_cooldown = 0;
    field_0x2c = 0;
    server_encryption_multiple = RandRange(8) + 5;
    client_encryption_multiple = RandRange(8) + 5;
    sequence = 0;
    query_id = RandRange(10000000) + 10;
    session_id = RandRange(50000) + 10000;
    receive_buffer = "";
    ping_timeout = false;
    packet_count = 0;
    account_ident = -1;
    character_id = 0xffffffff;
    idle_ticks = 0;
    map_id = 0;
    stat_points = 0;
    skill_points = 0;
    hangup_ticks = 0;
    recover_ticks = 0;
    field_0xa4 = 0;
    show_players = false;
    read_pos = -1;
    read_len = -1;
    session_token = -1;
    trade_accepted = false;
    stats_dirty = 0;
    dead = false;
    bank_dirty = 0;
    inventory_dirty = 0;
    equipment_dirty = 0;
    base_stats_dirty = false;
    map_has_quakes = false;
    map_has_spikes = false;
    map_has_hp_drain = false;
    map_has_tp_drain = false;
    last_pass_ms = DateTimeToTimeStamp(Now());
    field_0x37c = 30;
    drop_counter = 20;
    field_0x378 = 3;
    attack_tokens = 40;
    attack_token_ticks = 0;
    ghost_walk_tokens = 0;
    ghost_token_ticks = 0;
    last_client_walk_tick = 9000000;
    sync_base_ahead = -1;
    sync_base_behind = -1;
    null_string = "";
    flush_queue = 0;
    cheater_flag = false;
}

Player::~Player()
{
}

int Player::HpPercent(Player *self)
{
    if (self->max_hp < 1)
        return 0;
    double percent = (float)self->hp / (float)self->max_hp * 100.0f;
    if (percent < 1.0f)
        return 0;
    if (percent > 100.0f)
        return 100;
    return (int)percent;
}

int Player::CountPartyMembers()
{
    int count = 0;
    for (int i = 0; i < 9; i++)
    {
        if (party_ids[i] > -1)
            count++;
    }
    return count;
}

void Player::AddPartyMember(Player *self, int member_id)
{
    int i = 0;
    do
    {
        if (self->party_ids[i] == member_id)
            return;
        if (self->party_ids[i] < 0)
        {
            self->party_ids[i] = member_id;
            return;
        }
        i++;
    } while (i < 9);
}

char Player::IsPartyMember(Player *self, int player_id)
{
    for (int i = 0; i < 9; i++)
    {
        if (self->party_ids[i] == player_id)
            return true;
    }
    return false;
}

void Player::ClearPartyRoster(Player *self)
{
    self->party_ids[0] = -1;
    self->party_ids[1] = -1;
    self->party_ids[2] = -1;
    self->party_ids[3] = -1;
    self->party_ids[4] = -1;
    self->party_ids[5] = -1;
    self->party_ids[6] = -1;
    self->party_ids[7] = -1;
    self->party_ids[8] = -1;
}

void Player::UpdateBaseStats(Player *self)
{
    self->adj_strength = self->base_strength + self->equip_strength_bonus;
    self->adj_wisdom = self->base_wisdom + self->equip_wisdom_bonus;
    self->adj_intelligence = self->base_intelligence + self->equip_intelligence_bonus;
    self->adj_agility = self->base_agility + self->equip_agility_bonus;
    self->adj_constitution = self->base_constitution + self->equip_constitution_bonus;
    self->adj_charisma = self->base_charisma + self->equip_charisma_bonus;
}

void Player::CalculateHP_TP_SP(Player *self)
{
    self->max_hp = self->base_hp;
    self->max_tp = self->base_tp;
    self->max_sp = self->base_sp;
    double hp_mult = (self->level + 10) / 10;
    double con_bonus = self->adj_constitution * 1.25f;
    double int_bonus = self->adj_intelligence * 1.25f;
    double wis_bonus = self->adj_wisdom * 0.75f;
    double sp_bonus1 = (self->adj_constitution + 1) / 3;
    double sp_bonus2 = (self->adj_constitution + 1) / 3;
    self->max_hp = self->max_hp + hp_mult * con_bonus;
    self->max_tp = self->max_tp + hp_mult * int_bonus;
    self->max_tp = self->max_tp + hp_mult * wis_bonus;
    self->max_sp = self->max_sp + hp_mult * sp_bonus1;
    self->max_sp = self->max_sp + hp_mult * sp_bonus2;
    self->max_hp += self->equip_bonus_hp;
    self->max_tp += self->equip_bonus_tp;
    self->weight_max = self->adj_strength + 70;
    if (self->weight_max > 250)
        self->weight_max = 250;
    if (self->max_hp > 64000)
        self->max_hp = 64000;
    if (self->max_tp > 64000)
        self->max_tp = 64000;
    if (self->max_sp > 64000)
        self->max_sp = 64000;
}
