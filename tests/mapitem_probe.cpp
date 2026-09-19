#include <vector>

struct MapItemA
{
    int item_id;
    int amount;
    char item_present;
    char respawn_enabled;
    int respawn_countdown;
    unsigned short respawn_delay;
    unsigned short alt_item_id[4];
    int alt_amount[4];

    MapItemA(int item_id);
    ~MapItemA();
};

struct MapItemB
{
    int item_id;
    int amount;
    char item_present;
    char respawn_enabled;
    int respawn_countdown;
    unsigned short respawn_delay;
    unsigned short alt_item_id0;
    unsigned short alt_item_id1;
    unsigned short alt_item_id2;
    unsigned short alt_item_id3;
    int alt_amount[4];

    MapItemB(int item_id);
    ~MapItemB();
};

struct MapItemC
{
    int item_id;
    int amount;
    char item_present;
    char respawn_enabled;
    int respawn_countdown;
    unsigned short respawn_delay;
    unsigned short alt_item_id[4];
    int alt_amount0;
    int alt_amount1;
    int alt_amount2;
    int alt_amount3;

    MapItemC(int item_id);
    ~MapItemC();
};

struct MapItemD
{
    int item_id;
    int amount;
    char item_present;
    char respawn_enabled;
    int respawn_countdown;
    unsigned short respawn_delay;
    unsigned short alt_item_id0;
    unsigned short alt_item_id1;
    unsigned short alt_item_id2;
    unsigned short alt_item_id3;
    int alt_amount0;
    int alt_amount1;
    int alt_amount2;
    int alt_amount3;

    MapItemD(int item_id);
    ~MapItemD();
};

struct MapItemE
{
    int item_id;
    int amount;
    char item_present;
    char respawn_enabled;
    int respawn_countdown;
    unsigned short respawn_delay;
    unsigned short alt_item_id[4];
    long alt_amount[4];

    MapItemE(int item_id);
    ~MapItemE();
};

struct MapItemU
{
    int item_id;
    int amount;
    char item_present;
    char respawn_enabled;
    int respawn_countdown;
    unsigned short respawn_delay;
    union
    {
        unsigned short alt_item_id[4];
        struct
        {
            unsigned short alt_item_id0;
            unsigned short alt_item_id1;
            unsigned short alt_item_id2;
            unsigned short alt_item_id3;
        };
    };
    union
    {
        int alt_amount[4];
        struct
        {
            int alt_amount0;
            int alt_amount1;
            int alt_amount2;
            int alt_amount3;
        };
    };

    MapItemU(int item_id);
    ~MapItemU();
};

void probeU(std::vector<MapItemU> &dst, const std::vector<MapItemU> &src)
{
    std::vector<MapItemU> tmp(src);
    dst = tmp;
}

void probeD(std::vector<MapItemD> &dst, const std::vector<MapItemD> &src)
{
    std::vector<MapItemD> tmp(src);
    dst = tmp;
}

void probeE(std::vector<MapItemE> &dst, const std::vector<MapItemE> &src)
{
    std::vector<MapItemE> tmp(src);
    dst = tmp;
}

void probeA(std::vector<MapItemA> &dst, const std::vector<MapItemA> &src)
{
    std::vector<MapItemA> tmp(src);
    dst = tmp;
}

void probeB(std::vector<MapItemB> &dst, const std::vector<MapItemB> &src)
{
    std::vector<MapItemB> tmp(src);
    dst = tmp;
}

void probeC(std::vector<MapItemC> &dst, const std::vector<MapItemC> &src)
{
    std::vector<MapItemC> tmp(src);
    dst = tmp;
}
