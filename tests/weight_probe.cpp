struct ItemValues
{
    int GetWeight(int item_id) const;
};

struct MainForm
{
    ItemValues* item_values;
};

extern MainForm* MAINFORM;

struct Player
{
    char pad[0x13c];
    int weight_current;
    int weight_max;
    int& Weight() { return weight_current; }
};

static int GetWeight(int item_id)
{
    return MAINFORM->item_values->GetWeight(item_id);
}

// B: expanded assignment (our site 2 form)
void B(Player* player, int amount)
{
    player->weight_current = player->weight_current - GetWeight(1) * amount;
    if (player->weight_current < 0)
        player->weight_current = 0;
}

// U: multiply operand order swapped
void U(Player* player, int amount)
{
    player->weight_current = player->weight_current - amount * GetWeight(1);
    if (player->weight_current < 0)
        player->weight_current = 0;
}

// V: via inline accessor returning a reference
void V(Player* player, int amount)
{
    player->Weight() = player->Weight() - GetWeight(1) * amount;
    if (player->weight_current < 0)
        player->weight_current = 0;
}

// W: array index access
void W(Player* player, int amount)
{
    player[0].weight_current = player[0].weight_current - GetWeight(1) * amount;
    if (player->weight_current < 0)
        player->weight_current = 0;
}

// X: RHS field via char pointer arithmetic
void X(Player* player, int amount)
{
    player->weight_current =
        *(int*)((char*)player + 0x13c) - GetWeight(1) * amount;
    if (player->weight_current < 0)
        player->weight_current = 0;
}

// Y: LHS via pointer-to-member
void Y(Player* player, int amount)
{
    int Player::*m = &Player::weight_current;
    player->*m = player->*m - GetWeight(1) * amount;
    if (player->weight_current < 0)
        player->weight_current = 0;
}

// Z: unsigned amount
void Z(Player* player, unsigned int amount)
{
    player->weight_current = player->weight_current - GetWeight(1) * amount;
    if (player->weight_current < 0)
        player->weight_current = 0;
}

// AA: short amount
void AA(Player* player, short amount)
{
    player->weight_current = player->weight_current - GetWeight(1) * amount;
    if (player->weight_current < 0)
        player->weight_current = 0;
}

// AB: compound with product-first multiply
void AB(Player* player, int amount)
{
    player->weight_current -= amount * GetWeight(1);
    if (player->weight_current < 0)
        player->weight_current = 0;
}

// AC: expanded, getter via deref arrow chain
void AC(Player* player, int amount)
{
    player->weight_current =
        player->weight_current - MAINFORM->item_values->GetWeight(1) * amount;
    if (player->weight_current < 0)
        player->weight_current = 0;
}
