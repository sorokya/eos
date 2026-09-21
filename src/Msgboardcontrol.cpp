#include <vcl.h>
#include <fstream.h>
#pragma hdrstop

#include "Msgboardcontrol.h"
#include "Protocol.h"

#pragma package(smart_init)

MsgBoardController::MsgBoardController()
{
    field_0x118 = (char *)operator new(8);
    field_0x0 = 0x18;
    field_0x4 = 0;
    for (int i = 0; i < 8; i++)
    {
        boards[i].clear();
        aBoard_enabled[i] = 1;
    }
    LoadBoards(this);
}

MsgBoardController::~MsgBoardController()
{
    SaveBoards(this);
    for (int i = 0; i < 8; i++)
    {
        boards[i].clear();
        aBoard_enabled[i] = 1;
    }
}

String
MsgBoardController::EncodeNumber(MsgBoardController *self, unsigned int value, int width)
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
                self->field_0x118[i] = c;
                value = quotient;
                if (quotient < 1)
                    flag = false;
                else if (i + 1 == width)
                    width++;
            }
            else
            {
                char pad = EO_NUM_EMPTY;
                self->field_0x118[i] = pad;
            }
        }
    }
    catch (...)
    {
        width = 0;
    }
    String result(self->field_0x118, width);
    return result;
}

void MsgBoardController::ClearBoard(MsgBoardController *self, int board)
{
    if (board >= 1 && board <= 8)
    {
        board--;
        self->boards[board].clear();
        self->aBoard_enabled[board] = 1;
    }
}

void MsgBoardController::DeletePost(MsgBoardController *self, int board, int post_id)
{
    if (board >= 1 && board <= 8)
    {
        board--;
        for (vector<MsgBoard>::iterator it = self->boards[board].begin();
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

int MsgBoardController::CountPosts(MsgBoardController *self, int board, String author)
{
    int count = 0;
    if (board >= 1 && board <= 8)
    {
        board--;
        for (vector<MsgBoard>::iterator it = self->boards[board].begin();
             it != self->boards[board].end();
             it++)
        {
            if (author == it->poster)
                count++;
        }
    }
    return count;
}

void MsgBoardController::AddPost(MsgBoardController *self,
                                 int board,
                                 String poster,
                                 String subject,
                                 String message,
                                 char flag)
{
    if (board >= 1 && board <= 8)
    {
        self->field_0x4++;
        if (self->field_0x4 > 40000)
            self->field_0x4 = 0;
        MsgBoard post(self->field_0x4, poster, subject, message);
        board--;
        if (flag == 0)
            self->boards[board].insert(self->boards[board].begin(), post);
        else
            self->boards[board].insert(self->boards[board].end(), post);
        self->aBoard_enabled[board] = 1;
        if (self->boards[board].size() > (unsigned)(self->field_0x0 + 4))
            SetPostLimit(&self->boards[board], self->field_0x0 + 4);
    }
}

void MsgBoardController::SetPostLimit(vector<MsgBoard> *posts, unsigned int count)
{
    MsgBoard post;
    if (posts->size() < count)
        posts->insert(posts->end(), count - posts->size(), post);
    else if (count < posts->size())
        posts->erase(posts->begin() + count, posts->end());
}

String MsgBoardController::GetBoard(MsgBoardController *self, int board)
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

String MsgBoardController::GetPost(MsgBoardController *self, int board, int post_id)
{
    String result;
    if (board >= 1 && board <= 8)
    {
        board--;
        vector<MsgBoard>::iterator it;
        int count = 0;
        for (it = self->boards[board].begin();
             it != self->boards[board].end() && count < self->field_0x0;
             it++)
        {
            if (it->id == post_id)
            {
                result = EncodeNumber(self, it->id, 2);
                result.Insert(it->message, result.Length() + 1);
                break;
            }
        }
    }
    return result;
}

void MsgBoardController::BuildBoardName(MsgBoardController *self, int board)
{
    if (board >= 1 && board <= 8)
    {
        int count = self->boards[board - 1].size();
        if (count > self->field_0x0)
            count = self->field_0x0;
        String result = EncodeNumber(self, board, 1);
        result.Insert(EncodeNumber(self, count, 1), result.Length() + 1);
        board--;
        vector<MsgBoard>::iterator it;
        int i = 0;
        for (it = self->boards[board].begin();
             it != self->boards[board].end() && i < self->field_0x0;
             i++, it++)
        {
            result.Insert(EncodeNumber(self, it->id, 2), result.Length() + 1);
            result.Insert((char)EO_BREAK_BYTE, result.Length() + 1);
            result.Insert(it->poster, result.Length() + 1);
            result.Insert((char)EO_BREAK_BYTE, result.Length() + 1);
            result.Insert(it->subject, result.Length() + 1);
            result.Insert((char)EO_BREAK_BYTE, result.Length() + 1);
        }
        self->aBoard_names[board] = result;
    }
}

String MsgBoardController::BuildBoardData(MsgBoardController *self, int board)
{
    String text = "";
    String result = "";
    if (board >= 1 && board <= 8)
    {
        board--;
        result = EncodeNumber(self, self->boards[board].size(), 2);
        result.Insert((char)EO_BREAK_BYTE, result.Length() + 1);
        vector<MsgBoard>::iterator it;
        for (it = self->boards[board].begin(); it != self->boards[board].end(); it++)
        {
            String poster = it->poster;
            String subject = it->subject;
            String message = it->message;
            text.Insert(poster, text.Length() + 1);
            text.Insert(subject, text.Length() + 1);
            text.Insert(message, text.Length() + 1);
            result.Insert(EncodeNumber(self, poster.Length(), 2), result.Length() + 1);
            result.Insert((char)EO_BREAK_BYTE, result.Length() + 1);
            result.Insert(EncodeNumber(self, subject.Length(), 2), result.Length() + 1);
            result.Insert((char)EO_BREAK_BYTE, result.Length() + 1);
            result.Insert(EncodeNumber(self, message.Length(), 2), result.Length() + 1);
            result.Insert((char)EO_BREAK_BYTE, result.Length() + 1);
        }
        result.Insert(text, result.Length() + 1);
    }
    return result;
}

void MsgBoardController::LoadBoard(MsgBoardController *self, int board, String data)
{
    if (board >= 1 && board <= 8)
    {
        if (data.Length() > 0)
        {
            ClearBoard(self, board);
            board--;
            self->aBoard_enabled[board] = 1;
            SetDecodeSource(self, data, (char)EO_BREAK_BYTE);
            int count = DecodeNumber(self, ReadToken(self));
            int total = 0;
            for (int i = 0; i < count && i < 0x20; i++)
            {
                int a = DecodeNumber(self, ReadToken(self));
                int b = DecodeNumber(self, ReadToken(self));
                int c = DecodeNumber(self, ReadToken(self));
                self->field_0x144[i] = a;
                self->field_0x1c4[i] = b;
                self->field_0x244[i] = c;
                total = total + a + b + c;
            }
            String text = ReadRest(self);
            if (text.Length() >= total && total > 0)
            {
                for (int j = 0; j < count && j < 0x20; j++)
                {
                    String poster = text.SubString(1, self->field_0x144[j]);
                    text.Delete(1, self->field_0x144[j]);
                    String subject = text.SubString(1, self->field_0x1c4[j]);
                    text.Delete(1, self->field_0x1c4[j]);
                    String message = text.SubString(1, self->field_0x244[j]);
                    text.Delete(1, self->field_0x244[j]);
                    AddPost(self, board + 1, poster, subject, message, 1);
                }
            }
        }
    }
}

void MsgBoardController::SetDecodeSource(MsgBoardController *self,
                                         String data,
                                         char delimiter)
{
    self->field_0x10c = 1;
    self->misc_text = data;
    self->field_0x110 = data.Length();
    self->field_0x114 = delimiter;
}

String MsgBoardController::ReadToken(MsgBoardController *self)
{
    String result = "";
    try
    {
        if (self->field_0x110 >= 1)
        {
            while (self->field_0x10c <= self->field_0x110)
            {
                if (self->misc_text[self->field_0x10c] != self->field_0x114)
                {
                    result.Insert(self->misc_text[self->field_0x10c],
                                  result.Length() + 1);
                }
                else
                {
                    self->field_0x10c++;
                    break;
                }
                self->field_0x10c++;
            }
        }
    }
    catch (...)
    {
        result = "";
    }
    return result;
}

String MsgBoardController::ReadRest(MsgBoardController *self)
{
    String result = "";
    try
    {
        if (self->field_0x110 >= 1)
        {
            while (self->field_0x10c <= self->field_0x110)
            {
                result.Insert(self->misc_text[self->field_0x10c], result.Length() + 1);
                self->field_0x10c++;
            }
        }
    }
    catch (...)
    {
        result = "";
    }
    return result;
}

int MsgBoardController::DecodeNumber(MsgBoardController *self, String value)
{
    int result = 0;
    try
    {
        int byte_index = 1;
        while (value.Length() >= byte_index)
        {
            char c = value[byte_index];
            unsigned char ch = c;
            if (ch == EO_NUM_EMPTY || ch == 0)
                break;
            int n = ch;
            n = n - 1;
            if (byte_index == 1)
                result = result + n;
            if (byte_index == 2)
                result = result + n * EO_NUM_MAX;
            if (byte_index == 3)
                result = result + n * EO_NUM_MAX_2;
            if (byte_index == 4)
                result = result + n * EO_NUM_MAX_3;
            byte_index = byte_index + 1;
        }
    }
    catch (...)
    {
        result = 0;
    }
    return result;
}

bool MsgBoardController::LoadBoards(MsgBoardController *self)
{
    String path;
    String unused;
    int h;
    int size;
    char *buf;
    try
    {
        path = "./pub/dbb001.ebf";
        h = FileOpen(path.c_str(), 0);
        if (h < 0)
            return 0;
        size = FileSeek(h, 0, 2);
        FileSeek(h, 0, 0);
        buf = new char[size + 1];
        FileRead(h, buf, size);
        FileClose(h);
        path = buf;
        path.SetLength(size);
        delete[] buf;
        for (int i = 0; i < 8; i++)
        {
            self->boards[i].clear();
            self->aBoard_enabled[i] = 1;
        }
        for (int i = 0; i < 8; i++)
        {
            self->field_0x2c4[i] = DecodeNumber(self, path.SubString(1, 4));
            path.Delete(1, 4);
        }
        for (int i = 0; i < 8; i++)
        {
            self->aExtra_strings[i] = path.SubString(1, self->field_0x2c4[i]);
            path.Delete(1, self->field_0x2c4[i]);
        }
        for (int i = 0; i < 8; i++)
        {
            LoadBoard(self, i + 1, self->aExtra_strings[i]);
        }
    }
    catch (...)
    {
        FileClose(h);
        return 0;
    }
    return 1;
}

void MsgBoardController::SaveBoards(MsgBoardController *self)
{
    String lengths = "";
    String contents = "";
    String board_data = "";
    for (int i = 1; i <= 8; i++)
    {
        board_data = BuildBoardData(self, i);
        lengths.Insert(EncodeNumber(self, board_data.Length(), 4), lengths.Length() + 1);
        contents.Insert(board_data, contents.Length() + 1);
    }
    lengths.Insert(contents, lengths.Length() + 1);
    String path = "./pub/dbb001.ebf";
    ofstream file;
    file.open(path.c_str(), ios::binary);
    file << lengths.c_str();
    file.close();
}

