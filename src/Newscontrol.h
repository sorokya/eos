#ifndef NewscontrolH
#define NewscontrolH

#include <Classes.hpp>

// Layout recovered from the reference (offsets in bytes):
//   0  TStringList *ini_file
struct NewsTopics
{
    TStringList *ini_file;

    NewsTopics();
    ~NewsTopics();

    static void LoadNews(NewsTopics *self);
    static void LoadFile(NewsTopics *self, String file_name);
    static String Get(NewsTopics *self, int index);
};

#endif
