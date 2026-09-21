// Probe: the write-only 4-byte local the reference emits before
// `player->session_token = type_info.behavior_id;` in Player_HandlePacket's
// StatSkill Open branch. The reference stores behavior_id into a fresh slot and
// then re-reads behavior_id from type_info for the assignment; the slot is never
// read. Candidates below differ only in how the assignment's RHS is spelled.
//
//   bcc32 -D__CODEGUARD__ -v -Od -tWM -S -obuild/probe.asm tests/statopen_probe.cpp
#include <vcl.h>
#pragma hdrstop

struct NpcTypeInfo
{
    struct
    {
        int type;
        int behavior_id;
    };
    NpcTypeInfo()
    {
    }
};

struct Player
{
    int pad[33];
    int session_token;
};

NpcTypeInfo get_info(int id);

// A: named local, assignment uses the local.
void TestA(Player *player, int npc_id)
{
    NpcTypeInfo type_info = get_info(npc_id);
    if (type_info.type != 14)
        return;
    int behavior_id = type_info.behavior_id;
    player->session_token = behavior_id;
}

// B: named local, assignment re-reads the field (local then unused).
void TestB(Player *player, int npc_id)
{
    NpcTypeInfo type_info = get_info(npc_id);
    if (type_info.type != 14)
        return;
    int behavior_id = type_info.behavior_id;
    player->session_token = type_info.behavior_id;
}

// C: direct assignment (no local).
void TestC(Player *player, int npc_id)
{
    NpcTypeInfo type_info = get_info(npc_id);
    if (type_info.type != 14)
        return;
    player->session_token = type_info.behavior_id;
}

// D: assignment expression to a pre-declared local.
void TestD(Player *player, int npc_id)
{
    NpcTypeInfo type_info = get_info(npc_id);
    if (type_info.type != 14)
        return;
    int behavior_id;
    player->session_token = behavior_id = type_info.behavior_id;
}

// E: local initialized from the assignment expression.
void TestE(Player *player, int npc_id)
{
    NpcTypeInfo type_info = get_info(npc_id);
    if (type_info.type != 14)
        return;
    int behavior_id = player->session_token = type_info.behavior_id;
}
