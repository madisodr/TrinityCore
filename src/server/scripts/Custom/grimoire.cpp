/******************
 * Grimoire
 *
 * Author: Halcyon (Daniel Madison)
 *
 * Desc -
 */

#include "Chat.h"
#include "DatabaseEnv.h"
#include "Language.h"
#include "Item.h"
#include "Mail.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Spell.h"
#include "SpellMgr.h"

#define LEVEL_UP 47292
#define COIN 999116
#define UNCOMMON_REQUEST_COUNT 20
const char* GRIMOIRE_LOAD = "SELECT spell_id FROM grimoire WHERE item='%u'";
const char* ERROR_INVALID_ITEM = "Invalid item";
const char* ERROR_QUALITY_NOW_ALLOWED = "You can't request items of this quality.";
const char* ERROR_NOT_ENOUGH_COINS = "You do not have enough coins.";
const char* ERROR_ITEM_TYPE = "You can't request this type of item.";
const char* GRIMOIRE_UPDATE_CHARACTER = "UPDATE character_requests SET amount = amount - 1 WHERE guid='%u'";
const char* REQUESTS_LEFT = "You have '%u' requests left on this character.";

class Grimoire : public ItemScript {
    public:
        Grimoire() : ItemScript("Grimoire"){}
        bool OnUse(Player* p, Item* item, SpellCastTargets const& /*targets*/) {
            QueryResult result = WorldDatabase.PQuery(GRIMOIRE_LOAD, item->GetEntry());

            if (result) {
                Field* field = result->Fetch();
                uint32 spell = field[0].GetUInt32();

                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spell);
                if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo, p)) {
                    ChatHandler(p->GetSession()).PSendSysMessage(LANG_COMMAND_SPELL_BROKEN, spell);
                    ChatHandler(p->GetSession()).SetSentErrorMessage(true);
                    return false;
                }

                if (p->HasSpell(spell)) {
                    ChatHandler(p->GetSession()).SendSysMessage(LANG_YOU_KNOWN_SPELL);
                    return false;
                }

                p->LearnSpell(spell, false);
                p->DestroyItemCount(item->GetEntry(), 1, true, true);
            }

            return true;
        }
};

enum GossipOptions {
    REQUEST_ITEM = 5,
    DISMISS = 10,
    EXIT = 25
};

class GrimoireNpc : public CreatureScript {
    public:
        GrimoireNpc() : CreatureScript("grimoire_gossip") {}

        bool OnGossipHello(Player* player, Creature* creature) {
            if (!creature || !player)
                return false;

            ClearGossipMenuFor(player);
            AddGossipItemFor(player, 0, "Request an Item", GOSSIP_SENDER_MAIN, REQUEST_ITEM);
            AddGossipItemFor(player, 0, "Nevermind", GOSSIP_SENDER_MAIN, DISMISS);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, player->GetGUID());

            return true;
        }

        bool OnGossipSelectCode(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action, const char* code) {
            player->PlayerTalkClass->ClearMenus();
            player->PlayerTalkClass->SendCloseGossip();

            if (action == DISMISS)
                return true;

            uint32 itemId = 0;
            uint32 cost = 0;

            ChatHandler handler = ChatHandler(player->GetSession());

            if (atoi((char*) code) > 0)
                itemId = atoi((char*)code);

            if (itemId == COIN)
                return false;

            ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
            if (!itemTemplate) {
                handler.PSendSysMessage(ERROR_INVALID_ITEM);
                return false;
            }

            uint32 quality = itemTemplate->GetQuality();

            if (quality > ITEM_QUALITY_UNCOMMON) {
                ChatHandler(player->GetSession()).PSendSysMessage(ERROR_QUALITY_NOW_ALLOWED);
                return false;
            }

            uint8 itemClass = itemTemplate->GetClass();
            if (itemClass == 2 || itemClass == 4) {
                if (quality == ITEM_QUALITY_UNCOMMON)
                    cost = 30;

                if (GetItemRequests(player) > 0)
                    cost = 0;

                if( !player->HasItemCount(COIN, cost) && quality == ITEM_QUALITY_UNCOMMON) {
                    ChatHandler(player->GetSession()).PSendSysMessage(ERROR_NOT_ENOUGH_COINS);
                    return false;
                }

                if (CreateItem(player, itemId) == false)
                    return false;

                if (cost > 0) {
                    player->DestroyItemCount(COIN, cost, true);
                }

                if (quality == ITEM_QUALITY_UNCOMMON)
                    CharacterDatabase.PExecute(GRIMOIRE_UPDATE_CHARACTER, player->GetGUID().GetCounter());

                ChatHandler(player->GetSession()).PSendSysMessage(REQUESTS_LEFT, GetItemRequests(player));
                return true;
            } else {
                ChatHandler(player->GetSession()).PSendSysMessage(ERROR_ITEM_TYPE);
                return false;
            }
        }

        uint32 GetItemRequests(Player* player) {
            QueryResult result = CharacterDatabase.PQuery("SELECT amount FROM character_requests WHERE guid='%u'", player->GetGUID().GetCounter());
            if (result) {
                Field* fields = result->Fetch();
                return fields[0].GetUInt32();
            } else {
                CharacterDatabase.PExecute("INSERT INTO character_requests (guid, amount) VALUES ('%u', '%u')", player->GetGUID().GetCounter(), UNCOMMON_REQUEST_COUNT);
		        return UNCOMMON_REQUEST_COUNT;
            }

            return 0;
        }

        bool CreateItem(Player* player, uint32 itemId) {
            ChatHandler handler = ChatHandler(player->GetSession());
            if (Item* item = Item::CreateItem(itemId, 1, player)) {
                MailSender sender(MAIL_NORMAL, player->GetGUID().GetCounter(), MAIL_STATIONERY_GM);
                MailDraft draft("Item Request", "Your items, as requested.");

                SQLTransaction trans = CharacterDatabase.BeginTransaction();
                item->SaveToDB(trans);
                draft.AddItem(item);

                draft.SendMailTo(trans, MailReceiver(player, player->GetGUID().GetCounter()), sender);
                CharacterDatabase.CommitTransaction(trans);

                return true;
            }

            return false;
        }
};

void AddSC_Grimoire() {
    new Grimoire();
    new GrimoireNpc();
}
