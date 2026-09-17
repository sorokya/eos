#include <vcl.h>
#pragma hdrstop

#include <vector>

#include "Weddings.h"
#include "Protocol.h"
#include "Mainform.h"
#include "Itemvalues.h"

#pragma package(smart_init)

class Player
{
  public:
    char pad_00[0xac];
    String partner_name;
    char pad_b0[0x2c];
    int map_id;
    int x;
    int y;
    char pad_e8[0x54];
    int weight_current;
};

extern Player *Players_GetById(Players *self, int id);
extern void Player_AddItem(Players *self, Player *player, int item_id, int amount);
extern void
Client_SendEncoded(Server *self, Player *player, int type, int sub_type, String data);
extern void
Server_BroadcastToMap(Server *self, int map_id, int type, int sub_type, String data);

WeddingController::WeddingController(Players *players, Server *server)
{
    encode_scratch = new char[8];
    this->players = players;
    this->server = server;
}

WeddingController::~WeddingController()
{
}

bool WeddingController::Has(WeddingController *self, int map_id, int priest_line)
{
    for (std::vector<Wedding *>::iterator it = self->active_weddings.begin();
         it != self->active_weddings.end();
         it++)
    {
        if ((*it)->map_id == map_id && (*it)->priest_line == priest_line)
            return true;
    }
    return false;
}

void WeddingController::Add(WeddingController *self,
                            int map_id,
                            int priest_line,
                            int player1_id,
                            String player1_name,
                            int player2_id,
                            String player2_name)
{
    Wedding *wedding = new Wedding(map_id, priest_line);
    wedding->step = 0;
    wedding->countdown = 0x14;
    wedding->player1_id = player1_id;
    wedding->player2_id = player2_id;
    wedding->player1_name = player1_name;
    wedding->player2_name = player2_name;
    self->active_weddings.insert(self->active_weddings.end(), wedding);
}

void WeddingController::Confirm(WeddingController *self,
                                int map_id,
                                int priest_line,
                                int player_id)
{
    for (std::vector<Wedding *>::iterator it = self->active_weddings.begin();
         it != self->active_weddings.end();
         it++)
    {
        if ((*it)->map_id == map_id && (*it)->priest_line == priest_line)
        {
            if ((*it)->player1_id == player_id && (*it)->field_10 == 0)
            {
                (*it)->field_10 = 1;
                (*it)->step++;
                (*it)->countdown = 0x14;
            }
            if ((*it)->player2_id == player_id && (*it)->field_1c == 0)
            {
                (*it)->field_1c = 1;
                (*it)->step++;
                (*it)->countdown = 0x19;
            }
            if ((*it)->field_10 != 0 && (*it)->field_1c != 0)
            {
                Player *player1 = Players_GetById(self->players, (*it)->player1_id);
                Player *player2 = Players_GetById(self->players, (*it)->player2_id);
                if (player1 != 0 && player2 != 0)
                {
                    player1->partner_name = (*it)->player2_name;
                    player2->partner_name = (*it)->player1_name;
                    int item = 0x176;
                    int weight;

                    Player_AddItem(self->players, player1, item, 1);
                    player1->weight_current +=
                        ItemValues::Eif_GetWeight(GUI->item_values, item);
                    weight = player1->weight_current;
                    if (weight > 0xfa)
                        weight = 0xfa;
                    String data = AppendEncoded(self, item, 2);
                    data.Insert(AppendEncoded(self, 1, 3), data.Length() + 1);
                    data.Insert(AppendEncoded(self, weight, 1), data.Length() + 1);
                    Client_SendEncoded(self->server,
                                       player1,
                                       PacketAction_Obtain,
                                       PacketFamily_Item,
                                       data);

                    Player_AddItem(self->players, player2, item, 1);
                    player2->weight_current +=
                        ItemValues::Eif_GetWeight(GUI->item_values, item);
                    weight = player2->weight_current;
                    if (weight > 0xfa)
                        weight = 0xfa;
                    data = AppendEncoded(self, item, 2);
                    data.Insert(AppendEncoded(self, 1, 3), data.Length() + 1);
                    data.Insert(AppendEncoded(self, weight, 1), data.Length() + 1);
                    Client_SendEncoded(self->server,
                                       player2,
                                       PacketAction_Obtain,
                                       PacketFamily_Item,
                                       data);
                }
            }
        }
    }
}

void WeddingController::Tick(WeddingController *self)
{
    for (std::vector<Wedding *>::iterator it = self->active_weddings.begin();
         it != self->active_weddings.end();
         it++)
    {
        if ((*it)->countdown > 0)
        {
            (*it)->countdown--;
            if ((*it)->step == 0)
            {
                if ((*it)->countdown == 0x13)
                {
                    BroadcastPriestLine(
                        self, *it, "Very well, the ceremony will start in 20 seconds.");
                }
                if ((*it)->countdown == 0x12)
                {
                    Server_BroadcastToMap(self->server,
                                          (*it)->map_id,
                                          PacketAction_Player,
                                          PacketFamily_Jukebox,
                                          AppendEncoded(self, 0x28, 1));
                }
            }
            if ((*it)->step == 1)
            {
                if ((*it)->countdown == 0x19)
                {
                    String line = "we are here at the invitation of ";
                    line.Insert((*it)->player1_name, line.Length() + 1);
                    line.Insert(" and ", line.Length() + 1);
                    line.Insert((*it)->player2_name, line.Length() + 1);
                    line.Insert(", who have come before us to join together in marriage.",
                                line.Length() + 1);
                    BroadcastPriestLine(self, *it, line);
                }
                if ((*it)->countdown == 0x13)
                {
                    BroadcastPriestLine(
                        self,
                        *it,
                        "their relationship is based on love, respect, and a "
                        "determination to face the future together in health or "
                        "sickness, in joy and sorrow.");
                }
                if ((*it)->countdown == 0xd)
                {
                    String line = (*it)->player1_name;
                    line.Insert(", do you take ", line.Length() + 1);
                    line.Insert((*it)->player2_name, line.Length() + 1);
                    line.Insert(
                        " to be your partner, and promise to love, comfort and stay "
                        "together as long as you both shall live?",
                        line.Length() + 1);
                    BroadcastPriestLine(self, *it, line);
                }
                if ((*it)->countdown == 0xa && BothPresent(self, *it))
                {
                    Player *player = Players_GetById(self->players, (*it)->player1_id);
                    if (player != 0)
                    {
                        Client_SendEncoded(self->server,
                                           player,
                                           PacketAction_Reply,
                                           PacketFamily_Priest,
                                           AppendEncoded(self, 6, 2));
                    }
                }
            }
            if ((*it)->step == 2)
            {
                if ((*it)->countdown == 0x13)
                {
                    Player *player = Players_GetById(self->players, (*it)->player1_id);
                    if (player != 0)
                    {
                        String data = AppendEncoded(self, (*it)->player1_id, 2);
                        data.Insert("Yes, i do", data.Length() + 1);
                        Server_BroadcastToMap(self->server,
                                              (*it)->map_id,
                                              PacketAction_Player,
                                              PacketFamily_Talk,
                                              data);
                    }
                }
                if ((*it)->countdown == 0xd)
                {
                    String line = (*it)->player2_name;
                    line.Insert(", do you take ", line.Length() + 1);
                    line.Insert((*it)->player1_name, line.Length() + 1);
                    line.Insert(
                        " to be your partner, and promise to love, comfort and stay "
                        "together as long as you both shall live?",
                        line.Length() + 1);
                    BroadcastPriestLine(self, *it, line);
                }
                if ((*it)->countdown == 0xa && BothPresent(self, *it))
                {
                    Player *player = Players_GetById(self->players, (*it)->player2_id);
                    if (player != 0)
                    {
                        Client_SendEncoded(self->server,
                                           player,
                                           PacketAction_Reply,
                                           PacketFamily_Priest,
                                           AppendEncoded(self, 6, 2));
                    }
                }
            }
            if ((*it)->step == 3)
            {
                if ((*it)->countdown == 0x18)
                {
                    Player *player = Players_GetById(self->players, (*it)->player2_id);
                    if (player != 0)
                    {
                        String data = AppendEncoded(self, (*it)->player2_id, 2);
                        data.Insert("Yes, i do", data.Length() + 1);
                        Server_BroadcastToMap(self->server,
                                              (*it)->map_id,
                                              PacketAction_Player,
                                              PacketFamily_Talk,
                                              data);
                    }
                }
                if ((*it)->countdown == 0x12 && BothPresent(self, *it))
                {
                    BroadcastPriestLine(
                        self,
                        *it,
                        "Let these rings be given and received as a token of "
                        "your affection, sincerity and trust in one another.");
                }
                if ((*it)->countdown == 0xc)
                {
                    BroadcastPriestLine(
                        self, *it, "Please place these rings on eachothers finger..");
                }
                if ((*it)->countdown == 7 && BothPresent(self, *it))
                {
                    String data = AppendEncoded(self, (*it)->player1_id, 2);
                    data.Insert(AppendEncoded(self, 1, 3), data.Length() + 1);
                    data.Insert(AppendEncoded(self, (*it)->player2_id, 2),
                                data.Length() + 1);
                    data.Insert(AppendEncoded(self, 1, 3), data.Length() + 1);
                    Server_BroadcastToMap(self->server,
                                          (*it)->map_id,
                                          PacketAction_Player,
                                          PacketFamily_Effect,
                                          data);
                }
                if ((*it)->countdown == 6 && BothPresent(self, *it))
                {
                    String line = (*it)->player1_name;
                    line.Insert(" and ", line.Length() + 1);
                    line.Insert((*it)->player2_name, line.Length() + 1);
                    line.Insert(
                        " have consented together in marriage. And are now partners for "
                        "as long you both shall live.",
                        line.Length() + 1);
                    BroadcastPriestLine(self, *it, line);
                    Player *player1 = Players_GetById(self->players, (*it)->player1_id);
                    Player *player2 = Players_GetById(self->players, (*it)->player2_id);
                    if (player1 != 0 && player2 != 0)
                    {
                        String data = AppendEncoded(self, player1->x, 1);
                        data.Insert(AppendEncoded(self, player1->y, 1),
                                    data.Length() + 1);
                        data.Insert(AppendEncoded(self, 0xb, 2), data.Length() + 1);
                        data.Insert(AppendEncoded(self, player2->x, 1),
                                    data.Length() + 1);
                        data.Insert(AppendEncoded(self, player2->y, 1),
                                    data.Length() + 1);
                        data.Insert(AppendEncoded(self, 0xb, 2), data.Length() + 1);
                        Server_BroadcastToMap(self->server,
                                              (*it)->map_id,
                                              PacketAction_Agree,
                                              PacketFamily_Effect,
                                              data);
                    }
                }
                if ((*it)->countdown == 1)
                {
                    BroadcastPriestLine(self, *it, "Congratulations to the couple!");
                }
            }
            if ((*it)->countdown <= 0)
            {
                if ((*it)->step > 0)
                {
                    if ((*it)->step == 1 || (*it)->step == 2)
                    {
                        BroadcastPriestLine(
                            self, *it, "Im sorry, something went wrong..");
                    }
                    Wedding *wedding = *it;
                    self->active_weddings.erase(it);
                    delete wedding;
                    break;
                }
                if ((*it)->step == 0)
                {
                    if (!BothPresent(self, *it))
                    {
                        BroadcastPriestLine(
                            self, *it, "Im sorry, something went wrong..");
                        Wedding *wedding = *it;
                        self->active_weddings.erase(it);
                        delete wedding;
                        break;
                    }
                    (*it)->step = 1;
                    (*it)->countdown = 0x1a;
                }
            }
        }
    }
}

void WeddingController::BroadcastPriestLine(WeddingController *self,
                                            Wedding *record,
                                            String text)
{
    String data = AppendEncoded(self, record->priest_line, 2);
    data.Insert(text, data.Length() + 1);
    Server_BroadcastToMap(
        self->server, record->map_id, PacketAction_Dialog, PacketFamily_Npc, data);
}

bool WeddingController::BothPresent(WeddingController *self, Wedding *record)
{
    Player *player1 = Players_GetById(self->players, record->player1_id);
    Player *player2 = Players_GetById(self->players, record->player2_id);
    if (player1 == 0 || player2 == 0)
        return false;
    if (!(player1->map_id == record->map_id && player2->map_id == record->map_id))
        return false;
    return true;
}

String
WeddingController::AppendEncoded(WeddingController *self, unsigned int value, int width)
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
    String result(self->encode_scratch, width);
    return result;
}
