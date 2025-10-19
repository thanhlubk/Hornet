#include "HornetBase/TransactionHelper.h"
#include "HornetBase/DatabaseSession.h"
#include "HornetBase/HItem.h" // for HCursor/category/id accessors

namespace TransactionHelper
{
    // ---- NEW: HCursor* exchange as (CategoryType, Id) <-> pointer
    bool transactionDataExchange(bool capture, std::string &buf, size_t &off, HCursor *&p, DatabaseSession *pDb)
    {
        if (capture)
        {
            ItemType type = p ? p->type() : ItemType::ItemUnkown;
            Id id = p ? p->id() : Id{};
            return transactionDataExchange(true, buf, off, type, pDb) && transactionDataExchange(true, buf, off, id, pDb);
        }
        else
        {
            ItemType type{};
            Id id{};
            if (!transactionDataExchange(false, buf, off, type, pDb))
                return false;
            if (!transactionDataExchange(false, buf, off, id, pDb))
                return false;

            // null marker
            if (type == ItemType::ItemUnkown || id == Id{})
            {
                p = nullptr;
                return true;
            }

            // resolve via the session set by DatabaseSession at restore sites
            if (!pDb)
            {
                // No session in scope: cannot resolve now; fall back to nullptr.
                p = nullptr;
                return true;
            }
            p = pDb->getCursor(type, id);
            return true;
        }
    }
}