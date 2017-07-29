/* HyperionCore */

#include "Chat.h"
#include "DatabaseEnv.h"
#include "MoveSpline.h"
#include "MoveSplineInit.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "WaypointMovementGenerator.h"
#include "WorldSession.h"

class AthenaCommandScript : public CommandScript
{
    public:
        AthenaCommandScript() : CommandScript("AthenaCommandScript") {}
        std::vector<ChatCommand> GetCommands() const override
        {
            static std::vector<ChatCommand> AthenaCommandTable = {
                {"select",   rbac::RBAC_PERM_COMMAND_ATHENA_SELECT,   false,  &HandleAthenaSelectCommand,   ""},
                {"link",     rbac::RBAC_PERM_COMMAND_ATHENA_LINK,   false,  &HandleAthenaLinkCommand,     ""},
                {"unlink",   rbac::RBAC_PERM_COMMAND_ATHENA_UNLINK,   false,  &HandleAthenaUnlinkCommand,   ""},
            };

            static std::vector<ChatCommand> commandTable = {
                {"athena",   rbac::RBAC_PERM_COMMAND_ATHENA, false,  NULL, "", AthenaCommandTable},
            };

            return commandTable;
        }

        static bool HandleAthenaSelectCommand(ChatHandler* handler, const char* args) {
            if (!*args)
                return false;

            char* charID = handler->extractKeyFromLink((char*) args, "Hcreature_entry");

            if (!charID)
                return false;

            uint32 id = atoi(charID);

            if (!sObjectMgr->GetCreatureTemplate(id))
                return false;

            Player* p = handler->GetSession()->GetPlayer();
            p->SetAthenaSpawnEntry(id);
            handler->PSendSysMessage("Creature with id %u has been saved to your clipboard.", id);

            return true;
        }

        static bool HandleAthenaLinkCommand(ChatHandler* handler, const char* args) {
            if (!args)
                return false;

            handler->PSendSysMessage("Not yet implemented.");
            return true;
        }

        static bool HandleAthenaUnlinkCommand(ChatHandler* handler, const char* args) {
            if (!args)
                return false;

            handler->PSendSysMessage("Not yet implemented.");
            return true;
        }
};

class AthenaMovement : public SpellScriptLoader {
    public:
        AthenaMovement() : SpellScriptLoader( "AthenaMovement" ) {}

        class AthenaMovementSpellScript : public SpellScript {
        PrepareSpellScript( AthenaMovementSpellScript );

        void HandleAfterCast() {
            if (GetCaster()->GetTypeId() != TYPEID_PLAYER || !GetCaster())
                return;

            Player* player = GetCaster()->ToPlayer();
            Creature* creature = (Creature*) player->GetSelectedUnit();

            if (creature && creature->GetTypeId() == TYPEID_UNIT) {
                WorldLocation pos = *GetExplTargetDest();
                float x = pos.GetPositionX();
                float y = pos.GetPositionY();
                float z = pos.GetPositionZ();

                Movement::Location dest( x, y, z, 0.0f );
                Movement::MoveSplineInit init( creature );

                init.MoveTo( x, y, z );
                init.Launch();
            }
        }

        void Register() {
            AfterCast += SpellCastFn( AthenaMovementSpellScript::HandleAfterCast );
        }
    };

        SpellScript* GetSpellScript() const {
            return new AthenaMovementSpellScript();
        }
};

class AthenaPlacement : public SpellScriptLoader {
    public:
        AthenaPlacement() : SpellScriptLoader( "AthenaPlacement" ) {}

        class AthenaPlacementSpellScript : public SpellScript {
            PrepareSpellScript( AthenaPlacementSpellScript );

            void HandleAfterCast() {
                if (GetCaster()->GetTypeId() != TYPEID_PLAYER || !GetCaster())
                    return;

                Player* player = GetCaster()->ToPlayer();
                uint32 entry = player->AthenaSpawnEntry;
                Map* map = player->GetMap();
                const WorldLocation pos = *GetExplTargetDest();
                float x = pos.GetPositionX();
                float y = pos.GetPositionY();
                float z = pos.GetPositionZ();
                float o = pos.GetOrientation();

                if (!sObjectMgr->GetCreatureTemplate(entry))
                    return;

                Creature* creature = new Creature();
                if (!creature->Create(map->GenerateLowGuid<HighGuid::Creature>(), map, player->GetPhaseMask(), entry, x, y, z, o)) {
                    delete creature;
                    return;
                }

                creature->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()), player->GetPhaseMask());

                ObjectGuid::LowType db_guid = creature->GetSpawnId();
                creature->CleanupsBeforeDelete();
                delete creature;
                creature = new Creature();
                if (!creature->LoadCreatureFromDB(db_guid, map)) {
                    delete creature;
                    return;
                }

                sObjectMgr->AddCreatureToGrid(db_guid, sObjectMgr->GetCreatureData(db_guid));
            }

            void Register() {
                AfterCast += SpellCastFn(AthenaPlacementSpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const {
            return new AthenaPlacementSpellScript();
        }
};

void AddSC_Athena() {
    new AthenaPlacement();
    new AthenaMovement();
    new AthenaCommandScript();
}
