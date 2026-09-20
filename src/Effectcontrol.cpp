#include <vcl.h>
#pragma hdrstop

#include "Effectcontrol.h"
#include "Map.h"
#include "Player.h"
#include "Players.h"
#include "Protocol.h"
#include "Packets.h"

#pragma package(smart_init)

EffectController::EffectController(Mapcontrol *map_control,
                                   Players *players,
                                   Server *server,
                                   Settings *settings)
{
    pEncode_scratch = (char *)operator new(8);
    this->map_control = map_control;
    this->settings = settings;
    this->players = players;
    this->server = server;
    for (int i = 0; i < 4; i++)
    {
        aState_countdown[i] = 10;
        aState_value[i] = 0;
        aState_extra[i] = 1;
    }
    nBroadcast_gate = 10;
}

EffectController::~EffectController()
{
}

String
EffectController::EncodeNumber(EffectController *self, unsigned int value, int width)
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
                self->pEncode_scratch[i] = c;
                value = quotient;
                if (quotient < 1)
                    flag = false;
                else if (i + 1 == width)
                    width++;
            }
            else
            {
                char pad = EO_NUM_EMPTY;
                self->pEncode_scratch[i] = pad;
            }
        }
    }
    catch (...)
    {
        width = 0;
    }
    String encoded_str(self->pEncode_scratch, width);
    return encoded_str;
}

void EffectController::Tick(EffectController *self)
{
    for (int countdown_slot = 0; countdown_slot < 4; countdown_slot++)
        self->aState_countdown[countdown_slot]--;
    self->nBroadcast_gate--;

    Player **player_iter;
    for (player_iter = self->players->players.begin();
         player_iter != self->players->players.end();
         player_iter++)
    {
        if ((*player_iter)->map_has_quakes != 0)
        {
            int idx = Mapcontrol_GetByIndex(self->map_control, (*player_iter)->map_id - 1)
                          ->timed_effect -
                      3;
            if (idx >= 0 && idx <= 3)
            {
                if (self->aState_countdown[idx] < 1)
                {
                    String pkt = EncodeNumber(self, 1, 1);
                    pkt.Insert(EncodeNumber(self, self->aState_extra[idx], 1),
                               pkt.Length() + 1);
                    Client_SendEncoded(self->server,
                                       *player_iter,
                                       PacketAction_Use,
                                       PacketFamily_Effect,
                                       pkt);
                }
            }
            else
                continue;
        }

        if (self->nBroadcast_gate < 1)
        {
            if ((*player_iter)->map_has_hp_drain != 0 && (*player_iter)->hp > 0)
            {
                int hp_regen = (*player_iter)->max_hp / 10;
                if (hp_regen < 1)
                    hp_regen = 1;
                if ((*player_iter)->hp <= hp_regen)
                    hp_regen = (*player_iter)->hp - 1;
                (*player_iter)->hp -= hp_regen;
                (*player_iter)->item_change_count = hp_regen;
                if (Mapcontrol_GetByIndex(self->map_control, (*player_iter)->map_id - 1)
                        ->hp_drain_others_sent != 0)
                {
                    Mapcontrol_GetByIndex(self->map_control, (*player_iter)->map_id - 1)
                        ->hp_drain_others += "";
                    Mapcontrol_GetByIndex(self->map_control, (*player_iter)->map_id - 1)
                        ->hp_drain_others_sent = 0;
                }
                Mapcontrol_GetByIndex(self->map_control, (*player_iter)->map_id - 1)
                    ->hp_drain_others.Insert(
                        EncodeNumber(self, (*player_iter)->player_id, 2),
                        Mapcontrol_GetByIndex(self->map_control,
                                              (*player_iter)->map_id - 1)
                                ->hp_drain_others.Length() +
                            1);
                Mapcontrol_GetByIndex(self->map_control, (*player_iter)->map_id - 1)
                    ->hp_drain_others.Insert(
                        EncodeNumber(self, Player::HpPercent(*player_iter), 1),
                        Mapcontrol_GetByIndex(self->map_control,
                                              (*player_iter)->map_id - 1)
                                ->hp_drain_others.Length() +
                            1);
                Mapcontrol_GetByIndex(self->map_control, (*player_iter)->map_id - 1)
                    ->hp_drain_others.Insert(
                        EncodeNumber(self, hp_regen, 2),
                        Mapcontrol_GetByIndex(self->map_control,
                                              (*player_iter)->map_id - 1)
                                ->hp_drain_others.Length() +
                            1);
            }
            if ((*player_iter)->map_has_tp_drain != 0 && (*player_iter)->tp > 0)
            {
                int tp_regen = (*player_iter)->max_tp / 10;
                if (tp_regen < 1)
                    tp_regen = 1;
                if ((*player_iter)->tp <= tp_regen)
                    tp_regen = (*player_iter)->tp - 1;
                (*player_iter)->tp -= tp_regen;
                String pkt = EncodeNumber(self, 1, 1);
                pkt.Insert(EncodeNumber(self, tp_regen, 2), pkt.Length() + 1);
                pkt.Insert(EncodeNumber(self, (*player_iter)->tp, 2), pkt.Length() + 1);
                pkt.Insert(EncodeNumber(self, (*player_iter)->max_tp, 2),
                           pkt.Length() + 1);
                Client_SendEncoded(self->server,
                                   *player_iter,
                                   PacketAction_Spec,
                                   PacketFamily_Effect,
                                   pkt);
            }
        }

        if ((*player_iter)->map_has_spikes != 0)
        {
            unsigned int spec = Mapcontrol::Mapcontrol_GetTileSpec(self->map_control,
                                                                   (*player_iter)->map_id,
                                                                   (*player_iter)->x,
                                                                   (*player_iter)->y);
            if (spec == 0x21 || spec == 0x22)
            {
                if ((*player_iter)->hp > 0)
                {
                    int dmg = (*player_iter)->max_hp / 5;
                    int died = 0;
                    if (dmg < 1)
                        dmg = 1;
                    if ((*player_iter)->hp < dmg)
                        dmg = (*player_iter)->hp;
                    (*player_iter)->hp -= dmg;
                    if ((*player_iter)->hp <= 0)
                    {
                        (*player_iter)->hp = 0;
                        died = 1;
                    }
                    String pkt = EncodeNumber(self, 2, 1);
                    pkt.Insert(EncodeNumber(self, dmg, 2), pkt.Length() + 1);
                    pkt.Insert(EncodeNumber(self, (*player_iter)->hp, 2),
                               pkt.Length() + 1);
                    pkt.Insert(EncodeNumber(self, (*player_iter)->max_hp, 2),
                               pkt.Length() + 1);
                    Client_SendEncoded(self->server,
                                       *player_iter,
                                       PacketAction_Spec,
                                       PacketFamily_Effect,
                                       pkt);
                    pkt += EncodeNumber(self, (*player_iter)->player_id, 2);
                    pkt.Insert(EncodeNumber(self, Player::HpPercent(*player_iter), 1),
                               pkt.Length() + 1);
                    pkt.Insert(EncodeNumber(self, died, 1), pkt.Length() + 1);
                    pkt.Insert(EncodeNumber(self, dmg, 2), pkt.Length() + 1);
                    Server_BroadcastNearby(self->server,
                                           *player_iter,
                                           PacketAction_Admin,
                                           PacketFamily_Effect,
                                           pkt);
                    if (died != 0)
                        Player_Respawn(self->server, *player_iter);
                }
            }
            else
            {
                Client_SendEncoded(self->server,
                                   *player_iter,
                                   PacketAction_Report,
                                   PacketFamily_Effect,
                                   "S");
            }
        }
    }

    if (self->nBroadcast_gate < 1)
    {
        Player **broadcast_iter;
        for (broadcast_iter = self->players->players.begin();
             broadcast_iter != self->players->players.end();
             broadcast_iter++)
        {
            if ((*broadcast_iter)->map_has_hp_drain != 0)
            {
                Mapcontrol_GetByIndex(self->map_control, (*broadcast_iter)->map_id - 1)
                    ->hp_drain_others_sent = 1;
                String pkt = EncodeNumber(self, (*broadcast_iter)->item_change_count, 2);
                pkt.Insert(EncodeNumber(self, (*broadcast_iter)->hp, 2),
                           pkt.Length() + 1);
                pkt.Insert(EncodeNumber(self, (*broadcast_iter)->max_hp, 2),
                           pkt.Length() + 1);
                pkt.Insert(Mapcontrol_GetByIndex(self->map_control,
                                                 (*broadcast_iter)->map_id - 1)
                               ->hp_drain_others,
                           pkt.Length() + 1);
                Client_SendEncoded(self->server,
                                   *broadcast_iter,
                                   PacketAction_TargetOther,
                                   PacketFamily_Effect,
                                   pkt);
            }
        }
    }

    for (int reroll_slot = 0; reroll_slot < 4; reroll_slot++)
    {
        if (self->aState_countdown[reroll_slot] < 1)
        {
            int base = 4 - reroll_slot;
            self->aState_countdown[reroll_slot] =
                RandRange(0x32 - 10 * (reroll_slot + 1)) + base * 4 + 2;
            self->aState_value[reroll_slot] += self->aState_countdown[reroll_slot];
            if (0x118 - 40 * reroll_slot < self->aState_value[reroll_slot])
            {
                int r7 = RandRange(7);
                if (r7 > 4 || self->aState_value[reroll_slot] > 600)
                {
                    self->aState_countdown[reroll_slot] = base + 3;
                    self->aState_value[reroll_slot] = 0;
                }
            }
            self->aState_extra[reroll_slot] = RandRange(3) + reroll_slot * 2;
            if (reroll_slot == 1)
                self->aState_extra[reroll_slot] -= 2;
        }
    }
    if (self->nBroadcast_gate < 1)
        self->nBroadcast_gate = 10;
}
