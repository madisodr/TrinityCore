/* HyperionCore */

#include "DatabaseEnv.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ObjectAccessor.h"
#include "TargetedMovementGenerator.h"
#include "WorldSession.h"

const char* PEGASUS_SELECT_ENTRY = "SELECT entry, spawn_entry FROM pegasus";

enum GossipOptions {
    DISMISS,
    STAY,
    FOLLOW,
    BANK,
    EXIT
};

// container to hold mount data loaded from DB.
// static std::map<<entry, spawnid> mounts;
static std::unordered_map<uint32, uint32> mounts;

class PegasusLoader : public WorldScript {

    public:
        PegasusLoader() : WorldScript("pegasus_loader") {}

        void OnStartup() {
            LoadMounts();
        }

        void LoadMounts() {
            QueryResult r = WorldDatabase.PQuery(PEGASUS_SELECT_ENTRY);
            if (r) {
                Field* field;
                do {
                    field = r->Fetch();
                    mounts.insert(std::make_pair<uint32, uint32>(field[0].GetUInt32(), field[1].GetUInt32()));
                } while (r->NextRow());
            }
        }

        void ReloadMounts() {
            mounts.clear();
            LoadMounts();
        }
};

class PegasusHandler : public PlayerScript {

    public:
        PegasusHandler() : PlayerScript( "pegasus_handler" ) {}

        void UnsummonMountFromWorld(Player* player) {
            player->PlayerTalkClass->SendCloseGossip();

            if (player->GetPegasusMount() != NULL) {
                player->GetPegasusMount()->ToTempSummon()->UnSummon();
                player->SetPegasusMount(NULL);
            }
        }

        void OnMount(Player* player) {
            UnsummonMountFromWorld(player);
        }

        void OnDismount(Player* player, uint32 entry) {
            // no duplicate mounts
            UnsummonMountFromWorld(player);

            TempSummon* summon = player->SummonCreature(mounts[entry], player->GetPositionX() + 5, player->GetPositionY() + 5, player->GetPositionZ() + 1);
            player->SetPegasusMount(summon);
            summon->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, summon->GetFollowAngle());
        }

        void OnLogout(Player* player) {
            UnsummonMountFromWorld(player);
        }
};

class PegasusGossip : public CreatureScript {
    public:
        PegasusGossip() : CreatureScript("pegasus_gossip") { }

        bool OnGossipHello(Player* player, Creature* creature) {
            if (player == creature->ToTempSummon()->GetSummoner()) {
                ClearGossipMenuFor(player);
                AddGossipItemFor(player, 0, "Dismiss", GOSSIP_SENDER_MAIN, DISMISS);
                AddGossipItemFor(player, 0, "Stay", GOSSIP_SENDER_MAIN, STAY);
                AddGossipItemFor(player, 0, "Follow", GOSSIP_SENDER_MAIN, FOLLOW);
                AddGossipItemFor(player, 0, "Check Storage", GOSSIP_SENDER_MAIN, BANK);
                AddGossipItemFor(player, 0, "Exit", GOSSIP_SENDER_MAIN, EXIT);
                SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, player->GetGUID());
            }

            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) {
            ClearGossipMenuFor(player);
            if(player->GetPegasusMount() == NULL) {
                return false;
            }

            switch(action) {
                case DISMISS:
                        creature->ToTempSummon()->UnSummon();
                        player->SetPegasusMount(NULL);
                    break;
                case STAY: // Tell the mount to stay in one place.
                        creature->SetPosition(creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ(), creature->GetOrientation());
                        creature->GetMotionMaster()->MovementExpired(true);
                    break;
                case FOLLOW:
                        creature->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST, creature->GetFollowAngle());
                    break;
                case BANK:
                    player->GetSession()->SendShowBank(player->GetGUID());
                    break;
                case EXIT:
                    break;
            }

            CloseGossipMenuFor(player);
            return true;
        }
};

void AddSC_Pegasus() {
    new PegasusLoader();
    new PegasusHandler();
    new PegasusGossip();
}
