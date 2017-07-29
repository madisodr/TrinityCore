/* HyperionCore */

#include "Chat.h"
#include "DatabaseEnv.h"
#include "GameObject.h"
#include "Item.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "G3D/Quat.h"
#include "ObjectMgr.h"
#include "Spell.h"
#include "SpellScript.h"

#define HELIOS_SPELL 29173
#define START_ZONE 725

struct HeliosObjectTemplate {
    uint32 item;
    uint32 object;
};

const char* HELIOS_LOAD = "SELECT item, object FROM helios";
const char* HELIOS_LOOKUP = "SELECT object FROM helios_spawned WHERE item = '%u'";
static std::vector<HeliosObjectTemplate*> HeliosObjectTemplateList;

class HeliosHandler : public WorldScript {
    public:
        HeliosHandler() : WorldScript("HeliosHandler") {}
        void OnStartup() {
            loadHelios();
        }

        // Cleanup the helios objects
        void OnShutdown() {
            for (HeliosObjectTemplate* i : HeliosObjectTemplateList) {
                delete i;
            }
        }

        bool loadHelios() {
            Field* f;
            QueryResult r = WorldDatabase.PQuery(HELIOS_LOAD);
            if (!r)
                return false;

            do {
                f = r->Fetch();
                HeliosObjectTemplate* i = new HeliosObjectTemplate;
                i->item = f[0].GetUInt32();
                i->object = f[1].GetUInt32();
                HeliosObjectTemplateList.push_back( i );
            } while (r->NextRow());

            return true;
        }
};

class HeliosItem : public ItemScript {
    public:
        static uint32 itemEntry;
        static ObjectGuid itemGUID;
        static const WorldLocation* pos;

        HeliosItem() : ItemScript( "HeliosItem" ) {}

        bool OnUse(Player* player, Item* item, SpellCastTargets const& targets) {
            if (isGoodMap( player->GetMapId() )) {
                itemEntry = item->GetEntry();
                itemGUID = item->GetGUID();
                pos = targets.GetDstPos();
                player->CastSpell(player, HELIOS_SPELL, true);
                return true;
            } else {
                ChatHandler( player->GetSession() ).SendSysMessage( "You are not able to spawn gameobjects in this zone." );
                return false;
            }
        }

        bool isGoodMap(uint32 map) {
            if (map == START_ZONE)
                return false;

            return true;
        }
};

uint32 HeliosItem::itemEntry;
ObjectGuid HeliosItem::itemGUID;
const WorldLocation* HeliosItem::pos;

class HeliosSpell : public SpellScriptLoader {
    public:
        HeliosSpell() : SpellScriptLoader("HeliosSpell") {}
        class HeliosSpell_SpellScript : public SpellScript {
            PrepareSpellScript(HeliosSpell_SpellScript);

            void HandleAfterCast() {
                if (!GetCaster() || GetCaster()->GetTypeId() != TYPEID_PLAYER)
                    return;

                Player* player = GetCaster()->ToPlayer();
                const WorldLocation* pos = HeliosItem::pos;
                bool found = false;
                HeliosObjectTemplate* data;
                for (HeliosObjectTemplate* i : HeliosObjectTemplateList) {
                    if (i->item == HeliosItem::itemEntry) {
                        data = i;
                        found = true;
                        break;
                    }
                }

                if (found) {
                    uint32 objEntry = data->object;

                    QueryResult result = WorldDatabase.PQuery(HELIOS_LOOKUP, HeliosItem::itemGUID.GetCounter());
                    if (result) {
                        Field* f = result->Fetch();
                        ObjectGuid::LowType guidLow = f[0].GetUInt32();
                        if (!guidLow)
                            return;

                        GameObject* object = ChatHandler(player->GetSession()).GetObjectFromPlayerMapByDbGuid(guidLow);
                        if (!object) {
                            if (!PlaceHeliosObject(player, objEntry, pos)) {
                                ChatHandler(player->GetSession()).SendSysMessage("This object could not be placed. Please file a support ticket if it continues.");
                                return;
                            }
                        } else {
                            RemoveHeliosObject(player, object);
                        }
                    }
                } else { // Didn't find the object in the list. Broken APT.
                    ChatHandler(player->GetSession()).SendSysMessage( "The gameobject this is suppose to spawn appears to be missing. Please report this error if it continues." );
                    return;
                }
            }

            bool RemoveHeliosObject(Player* player, GameObject* object) {
                if (!object || !player)
                    return false;

                object->SetRespawnTime(0);  // not save respawn time
                object->Delete();
                object->DeleteFromDB();

                return true;
            }

            bool PlaceHeliosObject(Player* player, uint32 entry, const WorldLocation* pos) {
                Map* map = player->GetMap();
                GameObject* object = new GameObject;

                if (!object->Create(entry, map, 0, *player, QuaternionData::fromEulerAnglesZYX(player->GetOrientation(), 0.0f, 0.0f), 255, GO_STATE_READY)) {
                    delete object;
                    return false;
                }

                object->CopyPhaseFrom(player);

                object->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()), player->GetPhaseMask());
                ObjectGuid::LowType spawnId = object->GetSpawnId();
                delete object;

                object = new GameObject();
                if (!object->LoadGameObjectFromDB(spawnId, map)) {
                    delete object;
                    return false;
                }

                sObjectMgr->AddGameobjectToGrid(spawnId, ASSERT_NOTNULL(sObjectMgr->GetGOData(spawnId)));
                return true;
            }

            void Register() override {
                AfterCast += SpellCastFn(HeliosSpell_SpellScript::HandleAfterCast);
            }
        };
};


void AddSC_Helios() {
    new HeliosHandler();
    new HeliosItem();
    new HeliosSpell();

}
