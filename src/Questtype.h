#ifndef QuesttypeH
#define QuesttypeH

#include <Classes.hpp>

// Layout recovered from the reference constructor (0x5375d4): int value (+0),
// AnsiString name (+4).
class Questtype
{
  public:
    int value;
    String name;

    Questtype(int value, String name);
    ~Questtype();
};

#endif
