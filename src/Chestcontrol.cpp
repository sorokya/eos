#include <vcl.h>
#pragma hdrstop

#include "Chestcontrol.h"
#include "Protocol.h"
#include "Map.h"
#include "Player.h"
#include "Players.h"
#include "Packets.h"

#pragma package(smart_init)

ChestController::ChestController(Mapcontrol *map_control,
                                 Players *players,
                                 Server *server,
                                 Settings *settings)
{
    encode_scratch = (char *)operator new(8);
    this->map_control = map_control;
    this->settings = settings;
    this->players = players;
    this->server = server;
}

ChestController::~ChestController()
{
}

void ChestController::Tick(ChestController *self)
{
    MapContainer *map;
    MapChest *chest;
    MapItem *item;
    int max_items;
    int item_index;
    Player **player_iter;
    MapChest *chest_iter;
    MapItem *item_iter;

    for (map = MapVector_Begin(self->map_control);
         MapVector_End(self->map_control) != map;
         map++)
    {
        map->chests_dirty = 0;
        for (chest = map->chest_list.begin(); map->chest_list.end() != chest; chest++)
        {
            for (item = chest->slots.begin(); chest->slots.end() != item; item++)
            {
                if (item->respawn_enabled != 0 && item->item_present == 0)
                {
                    item->respawn_countdown--;
                    if (item->respawn_countdown <= 0)
                    {
                        map->chests_dirty = 1;
                        chest->updated = 1;
                        item->item_present = 1;
                        item->respawn_countdown = item->respawn_delay;
                        max_items = 1;
                        if (max_items == 1 && item->alt_item_id[1] > 0)
                            max_items++;
                        if (max_items == 2 && item->alt_item_id[2] > 0)
                            max_items++;
                        if (max_items == 3 && item->alt_item_id[3] > 0)
                            max_items++;
                        item_index = RandRange(max_items);
                        if (item_index == 0)
                        {
                            item->item_id = item->alt_item_id[0];
                            item->amount = item->alt_amount[0];
                        }
                        if (item_index == 1)
                        {
                            item->item_id = item->alt_item_id[1];
                            item->amount = item->alt_amount[1];
                        }
                        if (item_index == 2)
                        {
                            item->item_id = item->alt_item_id[2];
                            item->amount = item->alt_amount[2];
                        }
                        if (item_index == 3)
                        {
                            item->item_id = item->alt_item_id[3];
                            item->amount = item->alt_amount[3];
                        }
                    }
                }
            }
        }
    }

    for (player_iter = self->players->players.begin();
         self->players->players.end() != player_iter;
         player_iter++)
    {
        if ((*player_iter)->logged_in != 0 && (*player_iter)->map_id > 0 &&
            (*player_iter)->map_id <= Mapcontrol_GetCount(self->map_control) &&
            Mapcontrol_GetByIndex(self->map_control, (*player_iter)->map_id - 1)
                    ->chests_dirty != 0)
        {
            try
            {
                String pkt = "";
                for (chest_iter = Mapcontrol_GetByIndex(self->map_control,
                                                        (*player_iter)->map_id - 1)
                                      ->chest_list.begin();
                     Mapcontrol_GetByIndex(self->map_control, (*player_iter)->map_id - 1)
                         ->chest_list.end() != chest_iter;
                     chest_iter++)
                {
                    if (chest_iter->updated != 0)
                    {
                        chest_iter->updated = 0;
                        if (InRange(self,
                                    chest_iter->x,
                                    chest_iter->y,
                                    (*player_iter)->x,
                                    (*player_iter)->y))
                        {
                            pkt = "";
                            for (item_iter = chest_iter->slots.begin();
                                 chest_iter->slots.end() != item_iter;
                                 item_iter++)
                            {
                                if (item_iter->item_present != 0)
                                {
                                    pkt.Insert(AppendEncoded(self, item_iter->item_id, 2),
                                               pkt.Length() + 1);
                                    pkt.Insert(AppendEncoded(self, item_iter->amount, 3),
                                               pkt.Length() + 1);
                                }
                            }
                            Client_SendEncoded(self->server,
                                               *player_iter,
                                               PacketAction_Agree,
                                               PacketFamily_Chest,
                                               pkt);
                        }
                    }
                }
            }
            catch (...)
            {
            }
        }
    }
}

String
ChestController::AppendEncoded(ChestController *self, unsigned int value, int width)
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
                self->encode_scratch[i] = c;
                value = quotient;
                if (quotient < 1)
                    flag = false;
                else if (i + 1 == width)
                    width++;
            }
            else
            {
                char pad = 0xfe;
                self->encode_scratch[i] = pad;
            }
        }
    }
    catch (...)
    {
        width = 0;
    }
    String encoded_str(self->encode_scratch, width);
    return encoded_str;
}

bool ChestController::InRange(
    ChestController *self, int x, int y, int player_x, int player_y)
{
    bool result = false;
    int dx = player_x - x;
    int dy = player_y - y;
    if (dx < 0)
        dx = 0 - dx;
    if (dy < 0)
        dy = 0 - dy;
    if (dx + dy <= 1)
        result = true;
    return result;
}
