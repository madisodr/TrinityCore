/* HyperionCore
 */

#include "AccountMgr.h"
#include "Chat.h"
#include "Config.h"
#include "ChannelMgr.h"
#include "Channel.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "World.h"
#include "WorldSession.h"

using namespace std;

typedef string Color;
Color BLUE = "|cff3399FF";
Color RED = "|cffCC0000";
Color WHITE = "|cffFFFFFF";
Color GREEN = "|cff00cc00";
Color GRAY = "|cff808080";

const string CHANNEL_NAME = "OOC";
const uint32 CHANNEL_ID = 1;
const uint64 CHANNEL_DELAY = 1;

struct OOCChannelChatElements {
    uint64 time;
    string last_msg;
};

unordered_map<uint32, OOCChannelChatElements> OOCChannelChat;

string RANK[4] = {
    "Player",
    "Dev",
    "GM",
    "Admin",
};

class idyia_commandscript : public CommandScript {
    public:
        idyia_commandscript() : CommandScript("idyia_commandscript") {}

        std::vector<ChatCommand> GetCommands() const override {
            static std::vector<ChatCommand> commandTable = {
                { "togglechat", rbac::RBAC_PERM_COMMAND_IDYIA, false, &HandleToggleCommand, ""},
            };

            return commandTable;
        }

        static bool HandleToggleCommand(ChatHandler* handler, const char* /*args*/) {
            Player* player = handler->GetSession()->GetPlayer();
            string MSG = "";
            player->toggleOOCChat();
            if (player->IsSeeingOOCChat)
                MSG += "[" + RED + "Enableing OOC Chat|r]";
            else
                MSG += "[" + RED + "Disableing OOC Chat|r]";

            handler->PSendSysMessage( MSG.c_str() );
            return true;
        }
};

void SendWorldChatChannelMessage(const std::string msg) {
    SessionMap sessions = sWorld->GetAllSessions();

    for(SessionMap::iterator itr = sessions.begin(); itr != sessions.end(); ++itr) {
        if(!itr->second)
            continue;

        Player* player = itr->second->GetPlayer();
        if (player->IsSeeingOOCChat)
            ChatHandler(player->GetSession()).PSendSysMessage(msg.c_str());
    }
}


class ooc_channel : public PlayerScript {
    public:
        ooc_channel() : PlayerScript( "ooc_channel" ) {}

        void OnLogin(Player* player, bool /*firstLogin*/) override {
            ChatHandler(player->GetSession()).PSendSysMessage("Please refrain from being overtly offensive or using discriminatory langauge.");
            ChatHandler(player->GetSession()).PSendSysMessage("Joined OOC Chat. Use `/%u` to access the channel.", CHANNEL_ID);
            ChatHandler(player->GetSession()).PSendSysMessage("Use `.togglechat` to disable/enable ooc chat.");
            player->toggleOOCChat(true, true); // Turn chat on at start up.
        }

        virtual void OnChat(Player* player, uint32 /*type*/, uint32 lang, std::string& msg, Channel* channel) override {
            if (lang != LANG_ADDON) {
                uint32 raw_id = channel->GetChannelId();
                uint32 id;

                // channel id's seem to differ than what you see in game so we will convert them here.
                if (raw_id == 1) { id = 1; }
                if (raw_id == 2) { id = 2; }
                if (raw_id == 22) { id = 3; }

                // TC_LOG_INFO("server.loading", "RAW CHANNEL ID:%u", channel->GetChannelId()); // use to identify any channels not allready listed above.

                if (id == CHANNEL_ID) {
                    if ((msg != "") && (msg != "Away") && (player->CanSpeak() == true)) {
                        uint64 current_time = time(NULL);
                        uint32 guid = player->GetGUID().GetCounter();

                        if (!OOCChannelChat[guid].time) {
                            OOCChannelChat[guid].time = (current_time - CHANNEL_ID);
                            OOCChannelChat[guid].last_msg = "";
                        }
                    
                        // here we will set the gm's stored values so they clear the checks.
                        if (player->IsGameMaster()) {
                            OOCChannelChat[guid].time = current_time - CHANNEL_DELAY;
                            OOCChannelChat[guid].last_msg = "";
                        }

                        if ((current_time - OOCChannelChat[guid].time) < CHANNEL_DELAY) {
                            ChatHandler(player->GetSession()).PSendSysMessage("%sSpam timer triggered.", RED);
                        } else {
                            if (OOCChannelChat[guid].last_msg == msg) {
                                ChatHandler(player->GetSession()).PSendSysMessage("%sSpam message detected.", RED);
                            } else {
                                auto gm_rank = player->GetSession()->GetSecurity();
                                std::string pName = player->GetName();
                                //std::string name = "|Hplayer:" + pName + "|h" + pName;

                                std::string MSG = "";

                                MSG += "[" + GRAY + CHANNEL_NAME + "|r]";

                                if (player->IsGameMaster()) {
                                    if (gm_rank == SEC_ADMINISTRATOR)
                                        MSG += "[" + BLUE + "Admin" + "|r]";
                                    else if (gm_rank == SEC_GAMEMASTER || gm_rank == SEC_MODERATOR)
                                        MSG += "[" + BLUE + "Mod" + "|r]";
                                }

                                MSG += GREEN + "[" + pName + "|r]";
                                MSG += ":" + GREEN + msg;

                                OOCChannelChat[guid].time = current_time;
                                OOCChannelChat[guid].last_msg = msg;

                                SendWorldChatChannelMessage(MSG);
                            }
                        }
                    }
                    msg = -1;
                }
            }
        }
};

void AddSC_OOCChat() {
    new idyia_commandscript();
    new ooc_channel();
}
