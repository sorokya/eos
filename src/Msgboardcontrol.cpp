#include <vcl.h>
#pragma hdrstop

#include "Msgboardcontrol.h"

#pragma package(smart_init)

Msgboardcontrol::Msgboardcontrol()
{
    field_118 = (char *)operator new(8);
    field_0 = 0x18;
    field_4 = 0;
    for (int i = 0; i < 8; i++)
    {
        boards[i].clear();
        aBoard_enabled[i] = 1;
    }
    LoadBoards(this);
}

Msgboardcontrol::~Msgboardcontrol()
{
    SaveBoards(this);
    for (int i = 0; i < 8; i++)
    {
        boards[i].clear();
        aBoard_enabled[i] = 1;
    }
}

String
Msgboardcontrol::AppendEncoded(Msgboardcontrol *self, unsigned int value, int width)
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
                self->field_118[i] = c;
                value = quotient;
                if (quotient < 1)
                    flag = false;
                else if (i + 1 == width)
                    width++;
            }
            else
            {
                char pad = 0xfe;
                self->field_118[i] = pad;
            }
        }
    }
    catch (...)
    {
        width = 0;
    }
    String result(self->field_118, width);
    return result;
}

void Msgboardcontrol::ClearBoard(Msgboardcontrol *self, int board)
{
    if (board >= 1 && board <= 8)
    {
        board--;
        self->boards[board].clear();
        self->aBoard_enabled[board] = 1;
    }
}

void Msgboardcontrol::DeletePost(Msgboardcontrol *self, int board, int post_id)
{
    if (board >= 1 && board <= 8)
    {
        board--;
        for (std::vector<Msgboard>::iterator it = self->boards[board].begin();
             it != self->boards[board].end();
             it++)
        {
            if (it->id == post_id)
            {
                self->boards[board].erase(it);
                self->aBoard_enabled[board] = 1;
                return;
            }
        }
    }
}

int Msgboardcontrol::CountPosts(Msgboardcontrol *self, int board, String author)
{
    int count = 0;
    if (board >= 1 && board <= 8)
    {
        board--;
        for (std::vector<Msgboard>::iterator it = self->boards[board].begin();
             it != self->boards[board].end();
             it++)
        {
            if (author == it->poster)
                count++;
        }
    }
    return count;
}

void Msgboardcontrol::AddPost(Msgboardcontrol *self,
                              int board,
                              String poster,
                              String subject,
                              String message,
                              char flag)
{
    if (board >= 1 && board <= 8)
    {
        self->field_4++;
        if (self->field_4 > 40000)
            self->field_4 = 0;
        Msgboard post(self->field_4, poster, subject, message);
        board--;
        if (flag == 0)
            self->boards[board].insert(self->boards[board].begin(), post);
        else
            self->boards[board].insert(self->boards[board].end(), post);
        self->aBoard_enabled[board] = 1;
        if (self->boards[board].size() > (unsigned)(self->field_0 + 4))
            SetPostLimit(&self->boards[board], self->field_0 + 4);
    }
}

void Msgboardcontrol::SetPostLimit(std::vector<Msgboard> *posts, unsigned int count)
{
    Msgboard post;
    if (posts->size() < count)
        posts->insert(posts->end(), count - posts->size(), post);
    else if (count < posts->size())
        posts->erase(posts->begin() + count, posts->end());
}

String Msgboardcontrol::GetBoard(Msgboardcontrol *self, int board)
{
    String result;
    if (board >= 1 && board <= 8)
    {
        if (self->aBoard_enabled[board - 1])
        {
            BuildBoardName(self, board);
            self->aBoard_enabled[board - 1] = 0;
        }
        result = self->aBoard_names[board - 1];
    }
    return result;
}

String Msgboardcontrol::GetPost(Msgboardcontrol *self, int board, int post_id)
{
    String result;
    if (board >= 1 && board <= 8)
    {
        board--;
        int count = 0;
        for (std::vector<Msgboard>::iterator it = self->boards[board].begin();
             it != self->boards[board].end() && count < self->field_0;
             it++, count++)
        {
            if (it->id == post_id)
            {
                result = AppendEncoded(self, it->id, 2);
                result.Insert(it->message, result.Length() + 1);
                break;
            }
        }
    }
    return result;
}

void Msgboardcontrol::BuildBoardName(Msgboardcontrol *self, int board)
{
    if (board >= 1 && board <= 8)
    {
        int count = self->boards[board - 1].size();
        if (count > self->field_0)
            count = self->field_0;
        String result = AppendEncoded(self, board, 1);
        result.Insert(AppendEncoded(self, count, 1), result.Length() + 1);
        board--;
        std::vector<Msgboard>::iterator it;
        int i = 0;
        for (it = self->boards[board].begin();
             it != self->boards[board].end() && i < self->field_0;
             it++, i++)
        {
            result.Insert(AppendEncoded(self, it->id, 2), result.Length() + 1);
            String sep1 = (char)0xff;
            result.Insert(sep1, result.Length() + 1);
            result.Insert(it->poster, result.Length() + 1);
            String sep2 = (char)0xff;
            result.Insert(sep2, result.Length() + 1);
            result.Insert(it->subject, result.Length() + 1);
            String sep3 = (char)0xff;
            result.Insert(sep3, result.Length() + 1);
        }
        self->aBoard_names[board] = result;
    }
}

String Msgboardcontrol::BuildBoardData(Msgboardcontrol *self, int board)
{
    String text = "";
    String result = "";
    if (board >= 1 && board <= 8)
    {
        board--;
        result = AppendEncoded(self, self->boards[board].size(), 2);
        result.Insert(String((char)0xff), result.Length() + 1);
        for (std::vector<Msgboard>::iterator it = self->boards[board].begin();
             it != self->boards[board].end();
             it++)
        {
            String poster = it->poster;
            String subject = it->subject;
            String message = it->message;
            text.Insert(poster, text.Length() + 1);
            text.Insert(subject, text.Length() + 1);
            text.Insert(message, text.Length() + 1);
            result.Insert(AppendEncoded(self, poster.Length(), 2), result.Length() + 1);
            result.Insert(String((char)0xff), result.Length() + 1);
            result.Insert(AppendEncoded(self, subject.Length(), 2), result.Length() + 1);
            result.Insert(String((char)0xff), result.Length() + 1);
            result.Insert(AppendEncoded(self, message.Length(), 2), result.Length() + 1);
            result.Insert(String((char)0xff), result.Length() + 1);
        }
        result.Insert(text, result.Length() + 1);
    }
    return result;
}

void Msgboardcontrol::LoadBoard(Msgboardcontrol *self, int board, String data)
{
    if (board >= 1 && board <= 8)
    {
        if (data.Length() > 0)
        {
            ClearBoard(self, board);
            board--;
            self->aBoard_enabled[board] = 1;
            SetDecodeSource(self, data, (char)0xff);
            int count = DecodeNumber(self, ReadToken(self));
            int total = 0;
            int i = 0;
            while (i < count && i < 0x20)
            {
                self->field_144[i] = DecodeNumber(self, ReadToken(self));
                self->field_1c4[i] = DecodeNumber(self, ReadToken(self));
                self->field_244[i] = DecodeNumber(self, ReadToken(self));
                total =
                    total + self->field_144[i] + self->field_1c4[i] + self->field_244[i];
                i++;
            }
            String text = ReadRest(self);
            if (text.Length() >= total && total > 0)
            {
                int j = 0;
                while (j < count && j < 0x20)
                {
                    String poster = text.SubString(1, self->field_144[j]);
                    text.Delete(1, self->field_144[j]);
                    String subject = text.SubString(1, self->field_1c4[j]);
                    text.Delete(1, self->field_1c4[j]);
                    String message = text.SubString(1, self->field_244[j]);
                    text.Delete(1, self->field_244[j]);
                    AddPost(self, board + 1, poster, subject, message, 1);
                    j++;
                }
            }
        }
    }
}

void Msgboardcontrol::SetDecodeSource(Msgboardcontrol *self, String data, char delimiter)
{
    self->field_10c = 1;
    self->misc_text = data;
    self->field_110 = data.Length();
    self->field_114 = delimiter;
}

String Msgboardcontrol::ReadToken(Msgboardcontrol *self)
{
    String result = "";
    try
    {
        if (self->field_110 >= 1)
        {
            while (self->field_10c <= self->field_110)
            {
                if (self->misc_text[self->field_10c] == self->field_114)
                {
                    self->field_10c++;
                    break;
                }
                String ch(self->misc_text[self->field_10c]);
                result.Insert(ch, result.Length() + 1);
                self->field_10c++;
            }
        }
    }
    catch (...)
    {
        result = "";
    }
    return result;
}

String Msgboardcontrol::ReadRest(Msgboardcontrol *self)
{
    String result = "";
    try
    {
        if (self->field_110 >= 1)
        {
            while (self->field_10c <= self->field_110)
            {
                String ch(self->misc_text[self->field_10c]);
                result.Insert(ch, result.Length() + 1);
                self->field_10c++;
            }
        }
    }
    catch (...)
    {
        result = "";
    }
    return result;
}

int Msgboardcontrol::DecodeNumber(Msgboardcontrol *self, String value)
{
    int result = 0;
    try
    {
        int byte_index = 1;
        while (value.Length() >= byte_index)
        {
            char c = value[byte_index];
            unsigned char ch = c;
            if (ch == 0xFE || ch == 0)
                break;
            int n = ch;
            n = n - 1;
            if (byte_index == 1)
                result = result + n;
            if (byte_index == 2)
                result = result + n * 0xfd;
            if (byte_index == 3)
                result = result + n * 0xfa09;
            if (byte_index == 4)
                result = result + n * 0xf71ae5;
            byte_index = byte_index + 1;
        }
    }
    catch (...)
    {
        result = 0;
    }
    return result;
}

int Msgboardcontrol::LoadBoards(Msgboardcontrol *self)
{
    String path;
    String unused;
    String data = "./pub/dbb001.ebf";
    String tail;
    String literal;
    String token;
    String content;
    int h;
    int size;
    char *buf;
    try
    {
        path = data;
        tail = path.c_str();
        h = FileOpen(tail.c_str(), 0);
        if (h < 0)
            return 0;
        size = FileSeek(h, 0, 2);
        FileSeek(h, 0, 0);
        buf = new char[size + 1];
        FileRead(h, buf, size);
        FileClose(h);
        content = buf;
        content.SetLength(size);
        delete[] buf;
        for (int i = 0; i < 8; i++)
        {
            self->boards[i].clear();
            self->aBoard_enabled[i] = 1;
        }
        for (int i = 0; i < 8; i++)
        {
            token = content.SubString(1, 4);
            self->field_2c4[i] = DecodeNumber(self, token);
            content.Delete(1, 4);
        }
        for (int i = 0; i < 8; i++)
        {
            literal = content.SubString(1, self->field_2c4[i]);
            self->aExtra_strings[i] = literal;
            content.Delete(1, self->field_2c4[i]);
        }
        for (int i = 0; i < 8; i++)
        {
            LoadBoard(self, i + 1, self->aExtra_strings[i]);
        }
    }
    catch (...)
    {
        FileClose(h);
    }
    return 1;
}

void Msgboardcontrol::SaveBoards(Msgboardcontrol *self)
{
    String lengths = "";
    String contents = "";
    String board_data = "";
    for (int i = 1; i < 9; i++)
    {
        board_data = BuildBoardData(self, i);
        lengths.Insert(AppendEncoded(self, board_data.Length(), 4), lengths.Length() + 1);
        contents.Insert(board_data, contents.Length() + 1);
    }
    lengths.Insert(contents, lengths.Length() + 1);
    String path = "./pub/dbb001.ebf";
    std::ofstream file;
    file.open(path.c_str());
    file << lengths.c_str();
    file.close();
}
