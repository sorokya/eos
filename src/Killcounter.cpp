#include <vcl.h>
#pragma hdrstop

#include "Killcounter.h"

#pragma package(smart_init)

KillCounter::KillCounter(String name)
{
    this->name = name;
    count = 1;
}

KillCounter::KillCounter(String name, int count)
{
    this->name = name;
    this->count = count;
}

KillCounter::~KillCounter()
{
}
