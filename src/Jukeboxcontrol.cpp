#include <vcl.h>
#pragma hdrstop

#include "Jukeboxcontrol.h"

#pragma package(smart_init)

JukeBoxController::JukeBoxController()
{
    encode_scratch = (char *)operator new(8);
    recent_plays.clear();
}

String
JukeBoxController::EncodeNumber(JukeBoxController *self, unsigned int value, int width)
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

String JukeBoxController::BuildRecentTracksString(JukeBoxController *self, int map_id)
{
    String result = "";
    for (std::vector<JukeBox>::iterator it = self->recent_plays.begin();
         it != self->recent_plays.end();
         it++)
    {
        if (it->map_id != map_id)
            continue;
        result.Insert(EncodeNumber(self, map_id, 2), result.Length() + 1);
        if (it->active == 0)
            continue;
        TDateTime now = Now();
        TTimeStamp a = DateTimeToTimeStamp(it->last_play);
        TTimeStamp b = DateTimeToTimeStamp(now);
        int days = b.Date - a.Date;
        int ms = b.Time - a.Time;
        int secs = ms / 1000 + days * 86400;
        if (secs < 90)
            result.Insert(it->track_name, result.Length() + 1);
        else
            it->active = 0;
    }
    return result;
}

bool JukeBoxController::TryPlayTrack(JukeBoxController *self, int map_id, String track)
{
    bool played = false;
    for (std::vector<JukeBox>::iterator it = self->recent_plays.begin();
         it != self->recent_plays.end();
         it++)
    {
        if (it->map_id != map_id)
            continue;
        if (it->active)
        {
            TDateTime now = Now();
            TTimeStamp a = DateTimeToTimeStamp(it->last_play);
            TTimeStamp b = DateTimeToTimeStamp(now);
            int days = b.Date - a.Date;
            int ms = b.Time - a.Time;
            int secs = ms / 1000 + days * 86400;
            if (secs > 90)
                it->active = 0;
        }
        if (it->active == 0)
        {
            played = true;
            it->active = 1;
            it->last_play = Now();
            it->track_name = track;
            break;
        }
    }
    return played;
}

// BEGIN GENERATED STUBS (scripts/genstubs.py)
#pragma warn - 8057
// STUB(0x004a9e14, 96 bytes) FUN_004a9e14 - ref: undefined FUN_004a9e14(int param_1, byte
// param_2)
void FUN_004a9e14_Stub(int a0, unsigned char a1)
{
}
// STUB(0x004a9e74, 111 bytes) FUN_004a9e74 - ref: undefined FUN_004a9e74(int param_1,
// undefined2 param_2)
void FUN_004a9e74_Stub(int a0, short a1)
{
}
// STUB(0x004a9ee4, 155 bytes) FUN_004a9ee4 - ref: int FUN_004a9ee4(int param_1,
// undefined2 * param_2, undefined2 * param_3)
int FUN_004a9ee4_Stub(int a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x004a9f80, 19 bytes) FUN_004a9f80 - ref: undefined FUN_004a9f80(undefined4
// param_1, undefined4 param_2, undefined2 * param_3)
void FUN_004a9f80_Stub(int a0, int a1, void *a2)
{
}
// STUB(0x004aa20c, 132 bytes) FUN_004aa20c - ref: undefined FUN_004aa20c(undefined4
// param_1, undefined2 * param_2)
void FUN_004aa20c_Stub(int a0, void *a1)
{
}
// STUB(0x004aa290, 55 bytes) FUN_004aa290 - ref: undefined4 * FUN_004aa290(undefined4 *
// param_1, undefined4 * param_2)
void *FUN_004aa290_Stub(void *a0, void *a1)
{
    return 0;
}
// STUB(0x004aa2c8, 107 bytes) FUN_004aa2c8 - ref: undefined2 * FUN_004aa2c8(undefined2 *
// param_1, undefined2 * param_2, undefined2 * param_3)
void *FUN_004aa2c8_Stub(void *a0, void *a1, void *a2)
{
    return 0;
}
// STUB(0x004aa334, 36 bytes) FUN_004aa334 - ref: int FUN_004aa334(int param_1)
int FUN_004aa334_Stub(int a0)
{
    return 0;
}
// STUB(0x004aa358, 114 bytes) FUN_004aa358 - ref: int FUN_004aa358(undefined4 param_1,
// int param_2)
int FUN_004aa358_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x004aa3cc, 99 bytes) FUN_004aa3cc - ref: int FUN_004aa3cc(undefined2 * param_1,
// undefined2 * param_2, int param_3)
int FUN_004aa3cc_Stub(void *a0, void *a1, int a2)
{
    return 0;
}
// STUB(0x004aa454, 33 bytes) FUN_004aa454 - ref: int FUN_004aa454(int param_1, int
// param_2, undefined4 * param_3)
int FUN_004aa454_Stub(int a0, int a1, void *a2)
{
    return 0;
}
// STUB(0x004aa484, 11 bytes) FUN_004aa484 - ref: undefined4 FUN_004aa484(int param_1)
int FUN_004aa484_Stub(int a0)
{
    return 0;
}
// STUB(0x004aa490, 25 bytes) FUN_004aa490 - ref: undefined FUN_004aa490(int param_1, int
// param_2)
void FUN_004aa490_Stub(int a0, int a1)
{
}
// STUB(0x004aa4ac, 54 bytes) FUN_004aa4ac - ref: int FUN_004aa4ac(int param_1, int
// param_2)
int FUN_004aa4ac_Stub(int a0, int a1)
{
    return 0;
}
// STUB(0x004aa4e4, 79 bytes) FUN_004aa4e4 - ref: undefined FUN_004aa4e4(int param_1, int
// param_2)
void FUN_004aa4e4_Stub(int a0, int a1)
{
}
// STUB(0x004aa534, 101 bytes) FUN_004aa534 - ref: undefined2 * FUN_004aa534(int param_1,
// undefined2 * param_2)
void *FUN_004aa534_Stub(int a0, void *a1)
{
    return 0;
}
#pragma warn.8057
// END GENERATED STUBS
