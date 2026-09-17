#ifndef QuesttypeH
#define QuesttypeH

#include <Classes.hpp>

// Layout recovered from the reference constructor (0x5375d4): int value (+0),
// AnsiString name (+4).
class QuestType
{
  public:
    int value;
    String name;

    QuestType(int value, String name);
    ~QuestType();
};

#endif
