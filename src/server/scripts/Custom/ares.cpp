/* HyperionCore */
#include "Chat.h"
#include "DatabaseEnv.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

using namespace std;

static const vector<string> modifierList = {"str", "dex", "con", "int", "wis", "cha"};

enum MODS {
    STR,
    DEX,
    CON,
    ITN,
    WIS,
    CHA,
    STAT_MAX,
};

const char* ARES_NO_MODIFIERS_MSG = "We were unable to find any modifiers for your character. Please report this error to a GM or on the forums.";
const char* ARES_USING_BASE_MODIFIERS_MSG = "Using default modifiers. Visit our web portal and go to your character sheet in order to populate your modifiers with proper stats.";
const char* ARES_SELECT_STAT_QUERY = "SELECT %s FROM ares_modifiers WHERE guid='%u'";
const char* ARES_SELECT_ALL_QUERY = "SELECT str, dex, con, inte, wis, cha FROM ares_modifiers WHERE guid = '%u'";
const char* ARES_SELECT_BASE_RACIAL = "SELECT * FROM ares_base_racial WHERE race = '%u'";
const char* ARES_SELECT_BASE_CLASS = "SELECT * FROM ares_base_class WHERE class = '%u'";
const char* ARES_NEW = "INSERT INTO ares_modifiers VALUES('%u', '%u', '%u', '%u', '%u', '%u', '%u')";

const char* ARES_NAME_MSG = "Player: %s";
const char* ARES_STR_MSG = "Str: %u";
const char* ARES_DEX_MSG = "Dex: %u";
const char* ARES_CON_MSG = "Con: %u";
const char* ARES_ITN_MSG = "Int: %u";
const char* ARES_WIS_MSG = "Wis: %u";
const char* ARES_CHA_MSG = "Cha: %u";
typedef vector<int> AresStats;

class AresPlayerScript : public PlayerScript {
    public:
        AresPlayerScript() : PlayerScript( "AresPlayerScript" ) {}
        void OnCreate(Player* player) {
            AresStats mods = {0, 0, 0, 0, 0, 0};
            LoadBaseStats(player->getRace(), player->getClass(), mods);
            CharacterDatabase.DirectPExecute(ARES_NEW, player->GetGUID().GetCounter(), mods[STR], mods[DEX], mods[CON], mods[ITN], mods[WIS], mods[CHA]);
        }

        void LoadBaseStats(uint32 Race, uint32 Class, AresStats& mods) {
            Field* fields;
            QueryResult result = CharacterDatabase.PQuery(ARES_SELECT_BASE_RACIAL, Race);
            if (result) {
                fields = result->Fetch();
                for(int i = 0; i < STAT_MAX; i++)
                    mods[i] += fields[i].GetUInt32();
            }

            result = CharacterDatabase.PQuery(ARES_SELECT_BASE_CLASS, Class);
            if (result) {
                fields = result->Fetch();
                for(int i = 0; i < STAT_MAX; i++)
                    mods[i] += fields[i].GetUInt32();
            }
        }
};


class AresCommandScript : public CommandScript {
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

        static bool HandleViewCommand(ChatHandler* handler, const char* args) {
            Player* target;
            ObjectGuid targetGuid;
            std::string targetName;

            if(!handler->extractPlayerTarget((char*) args, &target, &targetGuid, &targetName))
                return false;

            Player* player = handler->GetSession()->GetPlayer();

            QueryResult result = CharacterDatabase.PQuery(ARES_SELECT_ALL_QUERY, targetGuid.GetCounter());
            if (result) {
                AresStats mods = {0, 0, 0, 0, 0, 0};
                Field* field = result->Fetch();

                ChatHandler(player->GetSession()).PSendSysMessage(ARES_NAME_MSG, targetName.c_str());
                for(int i = 0; i < STAT_MAX; i++)
                    mods[i] = field[i].GetUInt32();

                ChatHandler(player->GetSession()).PSendSysMessage(ARES_STR_MSG, mods[STR]);
                ChatHandler(player->GetSession()).PSendSysMessage(ARES_DEX_MSG, mods[DEX]);
                ChatHandler(player->GetSession()).PSendSysMessage(ARES_CON_MSG, mods[CON]);
                ChatHandler(player->GetSession()).PSendSysMessage(ARES_ITN_MSG, mods[ITN]);
                ChatHandler(player->GetSession()).PSendSysMessage(ARES_WIS_MSG, mods[WIS]);
                ChatHandler(player->GetSession()).PSendSysMessage(ARES_CHA_MSG, mods[CHA]);
            } else {
                ChatHandler(player->GetSession()).SendSysMessage(ARES_NO_MODIFIERS_MSG);
            }

            return true;
        }

        static bool HandleRollCommand(ChatHandler* handler, const char* args) {
            if (!*args)
                return false;

            char* c_rollStat = strtok((char*)args, " " );
            char* c_dice = strtok(NULL, " ");
            char* c_mod = strtok(NULL, " ");

            if (!c_rollStat || !c_dice)
                return false;

            // defaults to 20
            uint32 dice = (c_dice) ? atoi(c_dice) : 20;
            // if unspecificed, additional modifiers = 0
            uint8 mod = (c_mod) ? atoi(c_mod) : 0;

            int8 stat = -1;
            for (uint32 i = 0; i < modifierList.size(); i++) {
                if (c_rollStat == modifierList[i]) {
                    stat = i;
                    break;
                }
            }

            // the roll type isn't a valid modifier.
            if (stat == -1)
                return false;

            uint32 playerStat = 0;
            Player* player = handler->GetSession()->GetPlayer();

            QueryResult result = CharacterDatabase.PQuery(ARES_SELECT_STAT_QUERY, c_rollStat, player->GetGUID().GetCounter());

            if (!result)
                ChatHandler(player->GetSession()).SendSysMessage(ARES_NO_MODIFIERS_MSG);
            else
                playerStat = result->Fetch()[0].GetUInt32();

            uint32 roll = urand(1, dice);
            uint32 total = roll + playerStat + mod;
            std::string msg = "has rolled a 1d" + to_string(dice) + ": r" + to_string(roll) + " + b" + to_string(playerStat) + " + m" + to_string(mod) + " = " + to_string(total);
            player->TextEmote(msg);
            return true;
        }
};

void AddSC_Ares() {
    new AresPlayerScript();
    new AresCommandScript();
}
