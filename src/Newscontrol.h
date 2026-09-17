#ifndef NewscontrolH
#define NewscontrolH

#include <Classes.hpp>

// Layout recovered from the reference (offsets in bytes):
//   0  TStringList *ini_file
struct Newscontrol
{
    TStringList *ini_file;

    Newscontrol();
    ~Newscontrol();

    static void LoadNews(Newscontrol *self);
    static void LoadFile(Newscontrol *self, String file_name);
    static String Get(Newscontrol *self, int index);
};

#endif
