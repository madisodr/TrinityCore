/* HyperionCore
 * Code Name: Helios
 * Desc: Gameobject placement and building system
 */

#include "AccountMgr.h"
#include "Chat.h"
#include "DatabaseEnv.h"
#include "GameObject.h"
#include "Language.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "WorldSession.h"
#include <unordered_map>

#define JANUS_OBJECT 540120

const char* JANUS_LOAD     = "SELECT * FROM janus";
const char* JANUS_LOAD_ONE = "SELECT map, x, y, z, o FROM janus WHERE guid='%u' LIMIT 1";
const char* JANUS_LOOKUP   = "SELECT objectID FROM janus WHERE guid='%u'";
const char* JANUS_UPDATE   = "UPDATE janus SET map='%u', x='%f', y='%f', o='%f', WHERE guid='%u'";
const char* JANUS_INSERT   = "INSERT INTO janus (guid, map, x, y, z, o) VALUES (%u, %u, %f, %f, %f)";
const char* JANUS_DELETE   = "DELETE FROM janus WHERE guid='%u'";

class JanusCords {
    public:
        JanusCords(uint32 _map, float _x, float _y, float _z, float _o) {
            this->map = _map;
            this->x = _x;
            this->y = _y;
            this->z = _z;
            this->o = _o;
        }

        void UpdateCords(uint32 _map, float _x, float _y, float _z, float _o) {
            this->map = _map;
            this->x = _x;
            this->y = _y;
            this->z = _z;
            this->o = _o;
        }

        void FetchCords(uint32& _map, float& _x, float& _y, float& _z, float& _o) {
            _map = this->map;
            _x = this->x;
            _y = this->y;
            _z = this->z;
            _o = this->o;
        }

    private:
        uint32 map;
        float x;
        float y;
        float z;
        float o;
};

static std::unordered_map<uint32, JanusCords*> portal_map;

class JanusLoader : public WorldScript {
    public:
        JanusLoader() : WorldScript("janus_loader") {}
        void OnStartup() {
            LoadJanus();
        }

        void LoadJanus() {
            QueryResult r = WorldDatabase.PQuery(JANUS_LOAD);
            if (r) {
                Field* field;
                do {
                    field = r ->Fetch();
                    uint32 guid = field[0].GetUInt32();
                    uint32 map = field[1].GetUInt32();
                    float x = field[2].GetFloat();
                    float y = field[3].GetFloat();
                    float z = field[4].GetFloat();
                    float o = field[5].GetFloat();
                    JanusCords* t = new JanusCords(map, x, y, z, o);
                    std::pair<uint32, JanusCords*> newPair (guid, t);
                    portal_map.insert(newPair);
                } while (r->NextRow());
            }
        }

        void ReloadJanus() {
            for (auto& portal : portal_map) {
                delete portal.second;
            }

            portal_map.clear();
            LoadJanus();
        }
};

class JanusCommandScript : public CommandScript {
    public:
        JanusCommandScript() : CommandScript( "JanusCommandScript" ) {}
        std::vector<ChatCommand> GetCommands() const override
        {
            static std::vector<ChatCommand> JanusCommandTable =
            {
                { "link",   rbac::RBAC_PERM_COMMAND_JANUS_LINK,     false,  &HandleLinkCommand,   "" },
                { "delete", rbac::RBAC_PERM_COMMAND_JANUS_DELETE,   false,  &HandleDeleteCommand, "" },
            };

            static std::vector<ChatCommand> commandTable =
            {
                {"janus",   rbac::RBAC_PERM_COMMAND_JANUS, false,  NULL, "", JanusCommandTable},
            };

            return commandTable;
        }

        static bool HandleLinkCommand(ChatHandler* handler, const char* args) {

            char* id = handler->extractKeyFromLink((char*)args, "Hgameobject");
            if (!id)
                return false;

            ObjectGuid::LowType guidLow = strtoull(id, nullptr, 10);
            if (!guidLow)
                return false;

            Player* player = handler->GetSession()->GetPlayer();
            GameObject* object = ChatHandler(player->GetSession()).GetObjectFromPlayerMapByDbGuid(guidLow);

            if (!object) {
                handler->PSendSysMessage(LANG_COMMAND_OBJNOTFOUND, guidLow);
                handler->SetSentErrorMessage(true);
                return false;
            }

            if (object->GetEntry() != JANUS_OBJECT) {
                handler->PSendSysMessage("Not a Janus Gameobject. Object entry must be 540120", guidLow);
                handler->SetSentErrorMessage(true);
                return false;
            }

            float x = player->GetPositionX();
            float y = player->GetPositionY();
            float z = player->GetPositionZ();
            float o = player->GetOrientation();
            uint32 map = player->GetMapId();

            std::unordered_map<uint32, JanusCords*>::const_iterator it = portal_map.find(guidLow);
            if (it == portal_map.end()) {
                WorldDatabase.PExecute(JANUS_INSERT, guidLow, map, x, y, z, o);
                JanusCords* t = new JanusCords(map, x, y, z, o);
                portal_map.insert(std::make_pair<uint32, JanusCords*>((uint32)guidLow, (JanusCords*)t));
                handler->PSendSysMessage("Portal %u has been linked at your location. Map[%u] x[%f] y[%f] z[%f] o[%f]", guidLow, map, x, y, z, o);
            } else {
                WorldDatabase.PExecute(JANUS_UPDATE, map, x, y, z, o, guidLow);
                it->second->UpdateCords(map, x, y, z, 0);
                handler->PSendSysMessage("Portal %u has been updated at your location. Map[%u] x[%f] y[%f] z[%f] o[%f]", guidLow, map, x, y, z, o);
            }

            return true;
        }

        static bool HandleDeleteCommand(ChatHandler* handler, const char* args) {
            char* id = handler->extractKeyFromLink((char*)args, "Hgameobject");

            if (!id)
                return false;

            ObjectGuid::LowType guidLow = strtoull(id, nullptr, 10);

            if (!guidLow)
                return false;

            Player* player = handler->GetSession()->GetPlayer();
            GameObject* object = ChatHandler(player->GetSession()).GetObjectFromPlayerMapByDbGuid(guidLow);

            if (!object) {
                handler->PSendSysMessage(LANG_COMMAND_OBJNOTFOUND, guidLow);
                handler->SetSentErrorMessage(true);
                return false;
            }

            if (object->GetEntry() != JANUS_OBJECT) {
                handler->PSendSysMessage("This is not a Janus portal object. Please delete it as a regular gameobject.");
                handler->SetSentErrorMessage(true);
                return false;
            }

            ObjectGuid ownerGuid = object->GetOwnerGUID();
            if (!ownerGuid.IsEmpty()) {
                Unit* owner = ObjectAccessor::GetUnit(*handler->GetSession()->GetPlayer(), ownerGuid);
                if (!owner || !ownerGuid.IsPlayer()) {
                    handler->PSendSysMessage(LANG_COMMAND_DELOBJREFERCREATURE, ownerGuid.ToString().c_str(), object->GetGUID().ToString().c_str());
                    handler->SetSentErrorMessage(true);
                    return false;
                }

                owner->RemoveGameObject(object, false);
            }

            object->SetRespawnTime(0); // not save respawn time
            object->Delete();
            object->DeleteFromDB();

            std::unordered_map<uint32, JanusCords*>::const_iterator it = portal_map.find(guidLow);
            if (it == portal_map.end()) {
                handler->PSendSysMessage("Portal %u has no portal data loaded.", guidLow);
            } else {
                delete it->second;
                portal_map.erase(it);
                handler->PSendSysMessage("Portal %u has been deleted.", guidLow);
                WorldDatabase.PExecute(JANUS_DELETE, guidLow); // Delete the shit from janus' table.
            }

            handler->PSendSysMessage(LANG_COMMAND_DELOBJMESSAGE, object->GetGUID().ToString().c_str());
            return true;
        }
};

class Janus : public GameObjectScript {
    public:
        Janus() : GameObjectScript("janus"){}

        bool OnGossipHello(Player* player, GameObject* go) {
            uint64 guid = go->GetSpawnId();
            std::unordered_map<uint32, JanusCords*>::const_iterator it = portal_map.find(guid);
            if (it != portal_map.end()) {
                uint32 map;
                float x, y, z, o;
                JanusCords* t = it->second;
                t->FetchCords(map, x, y, z, o);
                player->TeleportTo(map, x, y, z, o);
            } else {
                ChatHandler(player->GetSession()).SendSysMessage("No portal information found. Please report this to a GM or admin with the following Gameobject ID: %u.", guid);
            }

            return true;
        }
};

void AddSC_Janus() {
    new Janus();
    new JanusCommandScript();
}
