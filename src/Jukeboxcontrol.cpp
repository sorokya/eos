#include <vcl.h>
#pragma hdrstop

#include "Jukeboxcontrol.h"
#include "Protocol.h"

#pragma package(smart_init)

JukeBoxController::JukeBoxController()
{
    encode_scratch = (char *)operator new(8);
    recent_plays.clear();
}

JukeBoxController::~JukeBoxController()
{
    recent_plays.clear();
}

void JukeBoxController::Add(JukeBoxController *self, int map_id)
{
    JukeBox record(map_id);
    self->recent_plays.insert(self->recent_plays.end(), record);
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
                rem = value % EO_NUM_MAX;
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
                char pad = EO_NUM_EMPTY;
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
        int secs = ms / MS_PER_SECOND + days * SECONDS_PER_DAY;
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
            int secs = ms / MS_PER_SECOND + days * SECONDS_PER_DAY;
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

void JukeBoxController_RemoveMap(JukeBoxController *self, int map_id)
{
    for (std::vector<JukeBox>::iterator it = self->recent_plays.begin();
         it != self->recent_plays.end();
         it++)
    {
        if (it->map_id == map_id)
        {
            self->recent_plays.erase(it);
            break;
        }
    }
}
