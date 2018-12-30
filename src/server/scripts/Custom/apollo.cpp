/*
 * Title: Apollo
 * Desc: Custom talent and profession system
 */

#include "DatabaseEnv.h"
#include "ScriptMgr.h"
#include <vector>

using namespace std;

enum PerkType {
    ITEM,
    SPELL,
    OBJECT,
    SUMMON,
};

class Talent {
    public:
        Talent(uint32 id, uint32 perk, PerkType t) {
            this->m_guid = id;
            this->m_perk = perk;
            this->m_type = t;
        }

        ~Talent() {} // empty

    private:
        uint32 m_guid;
        uint32 m_perk;
        PerkType m_type;
};

class TalentHandler {
    public:
        TalentHandler() {
            QueryResult result = WorldDatabase.PQuery("SELECT * FROM apollo_talents");
            if(!result)
                return;

            Field* field;
            uint32 id, perk;
            PerkType type;
            do {
                field = result->Fetch();
                id = field[0].GetUInt32();
                perk = field[1].GetUInt32();
                type = (PerkType)field[2].GetUInt32();

                m_talents.push_back(new Talent(id, perk, type));
            } while(result->NextRow());
        }

        ~TalentHandler() {
            for(Talent* t : m_talents)
                delete t;
        }
    private:
        vector<Talent*> m_talents;
};
