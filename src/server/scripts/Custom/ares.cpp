/* HyperionCore */
#include "Chat.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Hyperion.h"

using namespace std;

static const vector<string> modifierList = {"str", "agi", "sta", "int", "spt", "cha"};

enum MODS {
    STR,
    AGI,
    STA,
    ITN,
    SPT,
    CHA,
    STAT_MAX,
};

const char* ARES_NO_MODIFIERS_MSG = "We were unable to find any modifiers for your character. Please report this error to a GM or on the forums.";
const char* ARES_USING_BASE_MODIFIERS_MSG = "Using default modifiers. Visit our web portal and go to your character sheet in order to populate your modifiers with proper stats.";
const char* ARES_SELECT_STAT_QUERY = "SELECT modified, %s FROM ares_modifiers WHERE guid='%u'";
const char* ARES_SELECT_ALL_QUERY = "SELECT * FROM ares_modifiers WHERE guid='%u'";
const char* ARES_SELECT_BASE_RACIAL = "SELECT * FROM ares_base_racial WHERE race = '%u'";
const char* ARES_SELECT_BASE_CLASS = "SELECT * FROM ares_base_class WHERE class = '%u'";
const char* ARES_NEW = "INSERT INTO ares_modifiers VALUES('%u', '%u', '%u, '%u', '%u', '%u', '%u')";

const char* ARES_STR_MSG = "Strength: %u";
const char* ARES_AGI_MSG = "Agility: %u";
const char* ARES_STA_MSG = "Stamina: %u";
const char* ARES_ITN_MSG = "Intellect: %u";
const char* ARES_SPT_MSG = "Wisdom: %u";
const char* ARES_CHA_MSG = "Charisma: %u";
typedef vector<int> AresStats;

class AresPlayerScript : public PlayerScript
{
    public:
        AresPlayerScript() : PlayerScript( "AresPlayerScript" ) {}
        void OnCreate(Player* player)
        {
            AresStats mods = {0, 0, 0, 0, 0, 0};
            LoadBaseStats(player->getRace(), player->getClass(), mods);
            WorldDatabase.PExecute(ARES_NEW, player->GetGUID().GetCounter(), mods[STR], mods[AGI], mods[STA], mods[ITN], mods[SPT], mods[CHA]);
        }

        void LoadBaseStats(uint32 Race, uint32 Class, AresStats& mods)
        {
            Field* fields;
            QueryResult result = CharacterDatabase.PQuery(ARES_SELECT_BASE_RACIAL, Race);
            if(result)
            {
                fields = result->Fetch();
                for(int i = 0; i < STAT_MAX; i++)
                    mods[i] += fields[i].GetUInt32();
            }

            result = CharacterDatabase.PQuery(ARES_SELECT_BASE_CLASS, Class);
            if(result)
            {
                fields = result->Fetch();
                for(int i = 0; i < STAT_MAX; i++)
                    mods[i] += fields[i].GetUInt32();
            }
        }
};


class AresCommandScript : public CommandScript
{
    public:
        AresCommandScript() : CommandScript( "AresCommandScript" ) {}
        std::vector<ChatCommand> GetCommands() const override
        {
            static std::vector<ChatCommand> commandTable = {
                {"roll",        rbac::RBAC_PERM_COMMAND_ARES_ROLL,   false,  &HandleRollCommand,   ""},
                {"viewstats",   rbac::RBAC_PERM_COMMAND_ARES_VIEW,   false,  &HandleViewCommand,   ""},
            };

            return commandTable;
        }

        static bool HandleViewCommand(ChatHandler* handler, const char* args)
        {
            Player* player;
            ObjectGuid targetGuid;
            QueryResult result;
            std::string targetName;

            if(!handler->extractPlayerTarget((char*) args, &player, &targetGuid, &targetName))
                player = handler->GetSession()->GetPlayer();

            result = CharacterDatabase.PQuery(ARES_SELECT_ALL_QUERY, player->GetGUID().GetCounter());
            if(result)
            {
                AresStats mods = {0, 0, 0, 0, 0, 0};
                Field* field = result->Fetch();
                bool setup = field[1].GetBool();


                for(int i = 1; i < STAT_MAX; i++)
                    mods[i] = field[i+1].GetInt32();

                if(setup == false)
                    ChatHandler( player->GetSession() ).SendSysMessage( ARES_USING_BASE_MODIFIERS_MSG );

                ChatHandler(player->GetSession()).SendSysMessage(ARES_STR_MSG, mods[STR]);
                ChatHandler(player->GetSession()).SendSysMessage(ARES_AGI_MSG, mods[STR]);
                ChatHandler(player->GetSession()).SendSysMessage(ARES_STA_MSG, mods[STR]);
                ChatHandler(player->GetSession()).SendSysMessage(ARES_ITN_MSG, mods[STR]);
                ChatHandler(player->GetSession()).SendSysMessage(ARES_SPT_MSG, mods[STR]);
                ChatHandler(player->GetSession()).SendSysMessage(ARES_CHA_MSG, mods[STR]);
            } else
                ChatHandler(player->GetSession()).SendSysMessage(ARES_NO_MODIFIERS_MSG);

            return true;
        }

        static bool HandleRollCommand(ChatHandler* handler, const char* args)
        {
            if(!*args)
                return false;

            char* c_rollStat = strtok((char*)args, " " );
            char* c_dice = strtok(NULL, " ");
            char* c_mod = strtok(NULL, " ");

            if(!c_rollStat || !c_dice)
                return false;

            // defaults to 20
            uint32 dice = (c_dice) ? atoi(c_dice) : 20;
            // if unspecificed, additional modifiers = 0
            uint8 mod = (c_mod) ? atoi(c_mod) : 0;

            int8 stat = -1;
            for(uint32 i = 0; i < modifierList.size(); i++) {
                if(c_rollStat == modifierList[i]) {
                    stat = i;
                    break;
                }
            }

            // the roll type isn't a valid modifier.
            if(stat == -1)
                return false;

            uint32 playerStat = 0;
            Player* player = handler->GetSession()->GetPlayer();

            QueryResult result = CharacterDatabase.PQuery(ARES_SELECT_STAT_QUERY, c_rollStat, player->GetGUID().GetCounter());
            if(!result)
                ChatHandler(player->GetSession()).SendSysMessage(ARES_NO_MODIFIERS_MSG);
            else {
                playerStat = result->Fetch()[0].GetUInt32();
            }

            uint32 roll = urand(1, dice) + playerStat + mod;
            std::string msg = "has rolled a " + std::to_string(roll) + "/" + std::to_string(dice);
            player->TextEmote(msg);
            return true;
        }
};

void AddSC_Ares()
{
    new AresPlayerScript();
    new AresCommandScript();
}
