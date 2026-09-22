#include <vcl.h>
#pragma hdrstop

#include "Gamecontrol.h"
#include "Protocol.h"

#pragma package(smart_init)

Game::Game()
{
}

Game::~Game()
{
}

int Game::Exp_RequiredForLevel(Game *self, int level)
{
    int required_exp = 0;
    if (level > 0)
        required_exp = (int)((level * 1.1) * (level * 1.1) * (level * 1.1) * 100);
    return required_exp;
}

int Game::Combat_CalcArmorPen(Game *self, int avg_dmg, int armor, double factor)
{
    double result = 50;
    if (avg_dmg < armor)
    {
        double bound = armor;
        bound = bound - bound * factor;
        if (bound < 0)
            bound = 0;
        double damage_diff = armor - avg_dmg;
        double span = armor - bound;
        double rating = damage_diff / span * 40;
        if (avg_dmg < bound)
            result = 10;
        else
            result -= rating;
    }
    if (avg_dmg > armor)
    {
        double bound = armor;
        bound = bound * factor + bound + 1;
        double damage_diff = avg_dmg - armor;
        double span = bound - armor;
        double rating = damage_diff / span * 40;
        if (avg_dmg > bound)
            result = 90;
        else
            result += rating;
    }
    return (int)result;
}

int Game::Combat_CalcHitRate(Game *self, int accuracy, int evade, double factor)
{
    double result = 50;
    if (accuracy < evade)
    {
        double bound = evade;
        bound = bound - bound * factor;
        if (bound < 0)
            bound = 0;
        double accuracy_diff = evade - accuracy;
        double span = evade - bound;
        double rating = accuracy_diff / span * 30;
        if (accuracy < bound)
            result = 20;
        else
            result -= rating;
    }
    if (accuracy > evade)
    {
        double bound = evade;
        bound = bound * factor + bound + 1;
        double accuracy_diff = accuracy - evade;
        double span = bound - evade;
        double rating = accuracy_diff / span * 30;
        if (accuracy > bound)
            result = 80;
        else
            result += rating;
    }
    return (int)result;
}

double Game::Combat_CalcElementMult(Game *self,
                                    MapCoord coord,
                                    short atk_power,
                                    short target_value)
{
    double result = 1;
    if (coord.x > Element_None)
    {
        if (coord.x == Element_Dark)
            result = coord.y / 10 + atk_power + 8;
        else if (target_value > 0)
            result = Combat_ElementScore(self, atk_power + coord.y, target_value);
    }
    if (result > 1)
        result = 0.01L * result + 1;
    return result;
}

int Game::Combat_ElementScore(Game *self, int atk_power, int target_value)
{
    if (atk_power > 100)
        atk_power = 100;
    if (target_value > 100)
        target_value = 100;
    int score = (atk_power + atk_power + target_value) / 3;
    return score / 3 + 15;
}
