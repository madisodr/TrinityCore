/*
 * Ares
 * This script provides players access to D&D like character stats.
 *
 * TODO: Load the stats into memory versus keeping them in the database.
 * This can be done either by keeping a huge map here OR storing them with the Player object
 */

#include "Chat.h"
#include "DatabaseEnv.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

using namespace std;

typedef vector<int> AresStats;

// List of potential modifiers. Maybe generate these from a database.
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

/* SQL Queries */
const char* ARES_NO_MODIFIERS_MSG = "We were unable to find any modifiers for your character. Please report this error to a GM or on the forums.";
const char* ARES_USING_BASE_MODIFIERS_MSG = "Using default modifiers. Visit our web portal and go to your character sheet in order to populate your modifiers with proper stats.";
const char* ARES_SELECT_STAT_QUERY = "SELECT %s FROM ares_modifiers WHERE guid='%u'";
const char* ARES_SELECT_ALL_QUERY = "SELECT str, dex, con, inte, wis, cha FROM ares_modifiers WHERE guid = '%u'";
const char* ARES_SELECT_BASE_RACIAL = "SELECT * FROM ares_base_racial WHERE race = '%u'";
const char* ARES_SELECT_BASE_CLASS = "SELECT * FROM ares_base_class WHERE class = '%u'";
const char* ARES_NEW = "INSERT INTO ares_modifiers VALUES('%u', '%u', '%u', '%u', '%u', '%u', '%u')";

/* System Messages */
const char* ARES_NAME_MSG = "Player: %s";
const char* ARES_STAT_MSG = "%s: %u";


/* AresPlayerScript */
class AresPlayerScript : public PlayerScript {
    public:

        /* Default Constructor */
        AresPlayerScript() : PlayerScript( "AresPlayerScript" ) {}

        /*
         * When a new character is created, generate the base stats for that characters race + class combo and save it
         * to the database.
         */
        void OnCreate(Player* player) {
            AresStats mods = {0, 0, 0, 0, 0, 0};
            LoadBaseStats(player->getRace(), player->getClass(), mods);
            CharacterDatabase.DirectPExecute(ARES_NEW, player->GetGUID().GetCounter(), mods[STR], mods[DEX], mods[CON], mods[ITN], mods[WIS], mods[CHA]);
        }

        /*
         * Loads the base stats for a race and class combo, returning them added together in an array.
         */
        void LoadBaseStats(uint32 Race, uint32 Class, AresStats& mods) {
            Field* fields;
            QueryResult result = CharacterDatabase.PQuery(ARES_SELECT_BASE_RACIAL, Race);
            if (result) {
                fields = result->Fetch();
                for(int i = 0; i < STAT_MAX; i++) {
                    mods[i] += fields[i].GetUInt32();
                }
            }

            result = CharacterDatabase.PQuery(ARES_SELECT_BASE_CLASS, Class);
            if (result) {
                fields = result->Fetch();
                for(int i = 0; i < STAT_MAX; i++) {
                    mods[i] += fields[i].GetUInt32();
                }
            }
        }
};

/* AresCommandScript */
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

        /*
         * Displays the stats of a player to the caller
         */
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

                ChatHandler(player->GetSession()).PSendSysMessage(ARES_STAT_MSG, "Str", mods[STR]);
                ChatHandler(player->GetSession()).PSendSysMessage(ARES_STAT_MSG, "Dex", mods[DEX]);
                ChatHandler(player->GetSession()).PSendSysMessage(ARES_STAT_MSG, "Con", mods[CON]);
                ChatHandler(player->GetSession()).PSendSysMessage(ARES_STAT_MSG, "Int", mods[INT]);
                ChatHandler(player->GetSession()).PSendSysMessage(ARES_STAT_MSG, "Wis", mods[WIS]);
                ChatHandler(player->GetSession()).PSendSysMessage(ARES_STAT_MSG, "Cha", mods[CHA]);
            } else {
                ChatHandler(player->GetSession()).SendSysMessage(ARES_NO_MODIFIERS_MSG);
            }

            return true;
        }

        /*
         * Rolls a dice for a stat, with a dice type, and a mod.
         */
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
