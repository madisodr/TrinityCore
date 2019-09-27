/* Zombie Spawning */
#include "Chat.h"
#include "DatabaseEnv.h"
#include "ScriptMgr.h"
#include "Creature.h"
#include "Player.h"
#include <vector>

using namespace std;


namespace Hyperion {

    const char* GENERATOR_LOAD = "SELECT difficulty FROM hyperion_generator WHERE guid = '%u'";
    const char* GENERATOR_DEFAULT = "INSERT INTO hyperion_generator VALUES('%u', '%u')";
    const char* GENERATOR_UPDATE = "UPDATE hyperion_generator SET difficulty='%u' WHERE guid='%u'";

    enum Difficulty {
        EASY,
        NORMAL,
        HEROIC,
        LEGENDARY,
        MAX_LEVEL
    };

    enum Creatures {
        ZOMBIE = 37538,
    };

    enum MovementType {
        STOP,
        ROAM,
        WALK,
        RUN,
        FEIGN_DEATH,
        DIE,
        MAX
    };

    const int MAX_DIST_BEFORE_DESPAWN = 60;
    const int GRACE_PERIOD = 300; // 5 minutes / 300 seconds
    const int WAVE_TIMER = 6;
    const float PI_DIV = M_PI / 180;
    class Infected {
        public:
            Infected(Creature* c) {
                this->me = c;
            }

            ~Infected() {
                delete me;
            }

            bool InfectedMovement(MovementType m) {
                if(m < 0 || m >= MAX)
                    return false;

                this->m_status = m;

                switch(m) {
                    case STOP:
                        me->CombatStop();
                        me->StopMoving();
                        break;
                    case ROAM:
                        break;
                    case WALK:
                        me->SetWalk(true);
                        break;
                    case RUN:
                        me->SetWalk(false);
                        break;
                    case FEIGN_DEATH:
                        me->setDeathState(JUST_DIED);
                        break;
                    case DIE:
                        me->DealDamage(me, me->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                        break;
                    case MAX:
                    default:
                        break;
                }

                return true;
            }

            Creature* GetCreature() {
                return me;
            }

        private:
            Creature* me;
            MovementType m_status;
    };

    class Generator {
        public:
            Generator(Player* p) {
                this->me = p;
                this->m_guid = p->GetGUID().GetCounter();
                LoadFromDB();
            }

            ~Generator() {
                for (Infected* i : m_infected)
                    delete i;
            }

            void CleanUp() {
                for (Infected* i : m_infected)
                    delete i;

                ChatHandler(me->GetSession()).SendSysMessage("Clenaing Up");
                m_hordeSpawned = false;
            }

            void LoadFromDB() {
                Field* fields;
                ChatHandler(me->GetSession() ).SendSysMessage("Load from DB");
                QueryResult result = CharacterDatabase.PQuery(GENERATOR_LOAD, this->m_guid);
                if (result) {
                    fields = result->Fetch();
                    this->m_difficulty = (Difficulty)fields[0].GetUInt32();
                } else {
                    CharacterDatabase.PExecute(GENERATOR_DEFAULT, this->m_guid, Difficulty::EASY);
                    this->m_difficulty = Difficulty::EASY;
                }
            }

            void SaveToDB() {
                CharacterDatabase.PExecute(GENERATOR_LOAD, this->m_difficulty, this->m_guid);
            }

            void SetDifficulty(Difficulty _d) {
                this->m_difficulty = _d;
            }

            void SpawnHorde(uint8 _size, bool force = false) {
                ChatHandler(me->GetSession() ).SendSysMessage("Spawning Horde");
                if (_size <= 0)
                    return;

                if (m_hordeSpawned && !force)
                    return;

                for (uint8 i = 0; i < _size; i++) {
                    ChatHandler(me->GetSession() ).SendSysMessage("Adding creature");
                    int angle = (rand() % 720) - 360;
                    int dist = 20;

                    float pointX = me->GetPositionX() + (dist * cos(angle * PI_DIV));
                    float pointY = me->GetPositionY() + (dist * sin(angle * PI_DIV));
                    float pointZ = me->GetMap()->GetHeight(me->GetPhaseShift(), pointX, pointY, me->GetPositionZ());

                    Creature* t = me->SummonCreature(ZOMBIE, pointX, pointY, pointZ, angle, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, WAVE_TIMER * 10000);
                    Infected* infected = new Infected(t);
                    m_infected.push_back(infected);
                }

                ChatHandler(me->GetSession() ).SendSysMessage("Horde Spawned");
                m_hordeSpawned = true;
            }


            void CheckDistance() {
                for (Infected* i : m_infected) {
                    if (me->GetDistance(i->GetCreature()) > MAX_DIST_BEFORE_DESPAWN)
                        delete i;
                }
            }

        private:
            std::vector<Infected*> m_infected;
            bool m_hordeSpawned;
            uint32 m_guid;
            Difficulty m_difficulty;
            Player* me;
    };
}

class PlayerGenerator : public PlayerScript {
    public:
        PlayerGenerator() : PlayerScript("PlayerGenerator") {}

        void OnLogin(Player* player, bool /*firstLogin*/) {
            ChatHandler( player->GetSession() ).SendSysMessage("Creating Generator");
            m_generator = new Hyperion::Generator(player);
            waveTimer = Hyperion::GRACE_PERIOD;

        }

        void OnLogout(Player* p) {
            if (p && m_generator)
                delete m_generator;
        }

        void OnUpdate(Player* /*player*/, time_t now) {
            if (now <= waveTimer) {
                waveTimer = now + 300;
                m_generator->SpawnHorde(20, true);
            }
        }

    private:
        uint32 waveTimer;
        Hyperion::Generator* m_generator;
};

void AddSC_Generator() {
    new PlayerGenerator();
}
