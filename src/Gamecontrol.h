#ifndef GamecontrolH
#define GamecontrolH

#include "Mapcontrol.h"

// The combat/game-rules helper. sizeof is 8 (pinned by the `operator new(8)`
// argument at the FormCreate call site, 0x401f96); the constructor
// (0x4b1214) initializes no fields and no other function reads or writes them,
// so the two dwords are unnamed. The reference carries an explicit (empty)
// destructor (0x4b123c) and no virtual table.
class Game
{
  public:
    int field_0x0;
    int field_0x4;

    Game();
    ~Game();

    static int Exp_RequiredForLevel(Game *self, int level);
    static int Combat_CalcArmorPen(Game *self, int avg_dmg, int armor, double factor);
    static int Combat_CalcHitRate(Game *self, int accuracy, int evade, double factor);
    static double Combat_CalcElementMult(Game *self,
                                         MapCoord coord,
                                         short atk_power,
                                         short target_value);
    static int Combat_ElementScore(Game *self, int atk_power, int target_value);
};

#endif
