#include <vcl.h>
#pragma hdrstop

#include "Jukeboxcontrol.h"

#pragma package(smart_init)

Jukeboxcontrol::Jukeboxcontrol()
{
    field_0 = (char *)operator new(8);
    recent_plays.clear();
}

String Jukeboxcontrol::EncodeNumber(Jukeboxcontrol *self, int value, int width)
{
    return "";
}

String Jukeboxcontrol::BuildRecentTracksString(Jukeboxcontrol *self, int npc_id)
{
    String result = "";
    for (std::vector<JukeBox>::iterator it = self->recent_plays.begin();
         it != self->recent_plays.end(); it++)
    {
        if (it->id != npc_id)
            continue;
        result.Insert(EncodeNumber(self, npc_id, 2), result.Length() + 1);
        if (it->playing == 0)
            continue;
        TDateTime now = Now();
        TTimeStamp a = DateTimeToTimeStamp(it->timer);
        TTimeStamp b = DateTimeToTimeStamp(now);
        int days = b.Date - a.Date;
        int ms = b.Time - a.Time;
        int secs = ms / 1000 + days * 86400;
        if (secs < 90)
            result.Insert(it->name, result.Length() + 1);
        else
            it->playing = 0;
    }
    return result;
}
