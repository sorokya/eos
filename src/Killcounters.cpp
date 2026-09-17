#include <vcl.h>
#pragma hdrstop

#include "Killcounters.h"

#pragma package(smart_init)

Killcounters::Killcounters()
{
    buckets.Length = 27;
    field_0 = new TStringList;
}

Killcounters::~Killcounters()
{
}
