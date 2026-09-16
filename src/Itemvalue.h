#ifndef ItemvalueH
#define ItemvalueH

#include <Classes.hpp>

// Recovered from the reference (Itemvalue unit, 0x4781b8..0x478248) and the EIF
// parser (Itemvalues::AddItem, 0x479958). The constructor stores the item id at
// offset 0; every remaining EifRecord field is widened to a 16-bit slot (the
// one-byte on-disk fields are decoded to `short`) except `spec1`, which is a
// 32-bit word decoded from three bytes. `weight`/`element`/`element_damage`
// offsets are confirmed by the Eif_Get* accessors (0x47a2b8, 0x47a634); the
// field-to-offset assignment order is confirmed by the AddItem stores. sizeof
// is 0x58 (operator new(0x58) in AddItem) due to 4-byte alignment.
struct ItemValue
{
    int id;                              // +0x00
    short type;                          // +0x04 (Eif_GetType)
    short subtype;                       // +0x06 (Eif_GetSubtype)
    short special;                       // +0x08 (Eif_GetSpecial)
    short hp;                            // +0x0a
    short tp;                            // +0x0c
    short min_damage;                    // +0x0e
    short max_damage;                    // +0x10
    short accuracy;                      // +0x12
    short evade;                         // +0x14
    short armor;                         // +0x16
    short return_damage;                 // +0x18
    short strength;                      // +0x1a
    short intelligence;                  // +0x1c
    short wisdom;                        // +0x1e
    short agility;                       // +0x20
    short constitution;                  // +0x22
    short charisma;                      // +0x24
    short light_resistance;              // +0x26
    short dark_resistance;               // +0x28
    short earth_resistance;              // +0x2a
    short air_resistance;                // +0x2c
    short water_resistance;              // +0x2e
    short fire_resistance;               // +0x30
    int spec1;                           // +0x34 (Eif_GetSpec1)
    short spec2;                         // +0x38
    short spec3;                         // +0x3a
    short level_requirement;             // +0x3c
    short class_requirement;             // +0x3e
    short strength_requirement;          // +0x40
    short intelligence_requirement;      // +0x42
    short wisdom_requirement;            // +0x44
    short agility_requirement;           // +0x46
    short constitution_requirement;      // +0x48
    short charisma_requirement;          // +0x4a
    short weight;                        // +0x4c (Eif_GetWeight)
    short size;                          // +0x4e
    short unused;                        // +0x50
    short element;                       // +0x52 (Eif_GetElement)
    short element_damage;                // +0x54 (Eif_GetElement)

    ItemValue(int id);
    ~ItemValue();
};

#endif
