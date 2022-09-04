#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"

int mag_manacost(struct char_data * ch, int spellnum);
int mana_gain(struct char_data * ch);

MemoryOfSpells::MemoryOfSpells() : m_time(0), m_mana_per_second(0), m_mana_stored(0)
{
}

int MemoryOfSpells::size() const
{
    return mem_spells.size();
}

int MemoryOfSpells::getid(int index)
{
    return mem_spells[index];
}

int MemoryOfSpells::count(int id) const 
{
    int result = 0;
    for (int i=0,e=size(); i<e; ++i)
    {
        if (mem_spells[i] == id)
            result++;
    }
    return result;
} 

int MemoryOfSpells::get_next_spell(struct char_data *ch)
{
    if (size() == 0)
        return -1;
    int spell_id = mem_spells[0];
    int mana_cost = get_mana_cost(ch, spell_id);
    if (GET_MANA_STORED(ch) >= mana_cost)
    {
        mem_spells.erase(mem_spells.begin());
        m_mana_stored -= mana_cost;
        return spell_id;
    }
    return -1;
}

int MemoryOfSpells::get_mana_cost(struct char_data *ch, int id)
{
    return mag_manacost(ch, id);
}

void MemoryOfSpells::add(int id)
{
    mem_spells.push_back(id);
}

void MemoryOfSpells::dec(int id)
{
    int i=size()-1;
    for (; i>=0; --i)
    {
        if (mem_spells[i] == id)
        {
            mem_spells.erase(mem_spells.begin() + i);
            break;
        }
    }
}

void MemoryOfSpells::clear()
{
    mem_spells.clear();
}

int MemoryOfSpells::calc_time(struct char_data *ch, int index)
{
    int mana_need = GET_MANA_NEED(ch);
    if (index != -1)
    {
        mana_need = 0;
        for (int i=0; i<= index; i++)
        {
            int id = mem_spells[i];
            mana_need += get_mana_cost(ch, id);
        }        
    }

    float mana_req = mana_need;
    mana_req -= m_mana_stored;
    if (mana_req <= 0)
        return 0;

    int mg = mana_gain(ch);
    float mana_per_second = (float)mg / 60; 
    if (mana_per_second == 0)
        return 0;

    float ftime_req = mana_req / mana_per_second;
    int time_req = static_cast<int>(ftime_req);      // time in seconds
    return time_req;
}

int MemoryOfSpells::calc_mana_stored(char_data *ch)
{
    // calc dt
    time_t ctime = time(NULL);
    if (m_time == 0)
       m_time = ctime;
    int dt = ctime - m_time;
    if (dt < 0 || dt > 3)
        dt = 0;
    m_time = ctime;

    int mg = mana_gain(ch);
    if (mg <= 0)
    {
        m_mana_per_second = 0;
        m_mana_stored = GET_MANA_STORED(ch);
        return GET_MANA_STORED(ch);
    }

    int ms = static_cast<int>(m_mana_stored);
    if (ms != GET_MANA_STORED(ch))
        m_mana_stored = GET_MANA_STORED(ch);

    m_mana_per_second = (float)mg / 60;    
    float dmana = m_mana_per_second * dt;
    m_mana_stored = m_mana_stored + dmana;
    return static_cast<int>(m_mana_stored);
}
