#ifndef IntDecoderH
#define IntDecoderH

#include <Classes.hpp>

// Base for the *Values classes. `Decode` is a base-253 integer decoder recovered
// from the reference (ClassValues unit, 0x537330): positions 1..4 contribute
// (c - 1) * {1, 253, 253^2, 253^3}, terminating on byte 0xFE or 0. The same
// body is emitted once per *Values unit, so it is defined here inline rather
// than per class; `this` is not referenced by the body.
class IntDecoder
{
public:
    int Decode(String str)
    {
        String t = str;
        int result = 0;
        for (int i = 1; i <= t.Length(); i++)
        {
            unsigned char ch = t[i];
            if (ch == 0xFE || ch == 0)
                break;
            int c = ch - 1;
            if (i == 1)
                result += c;
            if (i == 2)
                result += c * 0xfd;
            if (i == 3)
                result += c * 0xfa09;
            if (i == 4)
                result += c * 0xf71ae5;
        }
        return result;
    }
};

#endif
