#include <vcl.h>
#pragma hdrstop

#include "Learnvalue.h"

#pragma package(smart_init)

LearnValue::LearnValue(int id)
{
    this->id = id;
    skills.clear();
}

LearnValue::~LearnValue()
{
}

void LearnValue::AddSkill(int skill_id, int level_requirement, int class_requirement,
                          int price, int skill_requirement_1, int skill_requirement_2,
                          int skill_requirement_3, int skill_requirement_4,
                          int str_requirement, int int_requirement,
                          int wis_requirement, int agi_requirement,
                          int con_requirement, int cha_requirement)
{
    bool found = false;
    std::vector<LearnItemVal>::iterator it = skills.begin();
    while (it != skills.end())
    {
        if (skill_id == it->id)
        {
            found = true;
            it->level_requirement = level_requirement;
            it->class_requirement = class_requirement;
            it->price = price;
            it->skill_requirement_1 = skill_requirement_1;
            it->skill_requirement_2 = skill_requirement_2;
            it->skill_requirement_3 = skill_requirement_3;
            it->skill_requirement_4 = skill_requirement_4;
            it->str_requirement = str_requirement;
            it->int_requirement = int_requirement;
            it->wis_requirement = wis_requirement;
            it->agi_requirement = agi_requirement;
            it->con_requirement = con_requirement;
            it->cha_requirement = cha_requirement;
        }
        it++;
    }
    if (!found)
    {
        LearnItemVal value(skill_id);
        value.level_requirement = level_requirement;
        value.class_requirement = class_requirement;
        value.price = price;
        value.skill_requirement_1 = skill_requirement_1;
        value.skill_requirement_2 = skill_requirement_2;
        value.skill_requirement_3 = skill_requirement_3;
        value.skill_requirement_4 = skill_requirement_4;
        value.str_requirement = str_requirement;
        value.int_requirement = int_requirement;
        value.wis_requirement = wis_requirement;
        value.agi_requirement = agi_requirement;
        value.con_requirement = con_requirement;
        value.cha_requirement = cha_requirement;
        skills.insert(skills.end(), value);
    }
}
