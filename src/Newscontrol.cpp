#include <vcl.h>
#pragma hdrstop

#include "Newscontrol.h"

#pragma package(smart_init)

NewsTopics::NewsTopics()
{
    ini_file = new TStringList;
    LoadNews(this);
}

NewsTopics::~NewsTopics()
{
}

void NewsTopics::LoadNews(NewsTopics *self)
{
    LoadFile(self, "./config/news.ini");
}

String NewsTopics::Get(NewsTopics *self, int index)
{
    String result = "";
    if (index < self->ini_file->Count)
        result = self->ini_file->Strings[index];
    return result;
}

void NewsTopics::LoadFile(NewsTopics *self, String file_name)
{
    self->ini_file->Clear();

    try
    {
        String exe = Application->ExeName;
        int last_slash_pos = 0;
        if (exe.Length() >= 1)
        {
            for (int i = exe.Length(); i >= 1; i--)
            {
                if (exe[i] == '\\')
                {
                    last_slash_pos = i;
                    break;
                }
            }
        }

        String dir = "";
        if (exe.Length() >= 1 && last_slash_pos >= 1)
        {
            for (int char_index = 1; char_index < last_slash_pos; char_index++)
                dir = dir + exe[char_index];
        }

        exe = dir;

        String tail;
        if (exe[exe.Length()] != '\\')
            tail = "\\" + file_name;
        else
            tail = file_name;

        self->ini_file->LoadFromFile(exe + tail);

        for (int i = self->ini_file->Count - 1; i >= 0; i--)
        {
            if (self->ini_file->Strings[i].Length() > 1)
            {
                if (self->ini_file->Strings[i][1] == '#')
                    self->ini_file->Delete(i);
            }
            else
                self->ini_file->Delete(i);
        }
    }
    catch (...)
    {
    }
}
