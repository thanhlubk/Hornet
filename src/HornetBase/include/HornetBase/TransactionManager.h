#pragma once
#include "HBaseDef.h"
#include "HItemManager.h"
#include <functional>
#include <stdexcept>

// class DatabaseSession; // <<< forward declare (avoid circular include)

class HORNETBASE_EXPORT TransactionManager
{
public:
    enum class TransactionType : std::uint8_t
    {
        Unknown = 0,
        Emplace = 1,
        Erase = 2,
        Modify = 3
    };

    struct TransactionOperation
    {
        TransactionType type;
        ItemTypeVariant itemType;
        Id id;
        std::string payloadBefore;
        std::string payloadAfter;

        TransactionOperation()
        {
            type = TransactionType::Unknown;
            itemType.type = ItemType::ItemUnkown;
            itemType.variant = 0;
            id = 0;
            payloadBefore.clear();
            payloadAfter.clear();
        }
    };

    struct Transaction
    {
        std::vector<TransactionOperation> ops;
    };

    explicit TransactionManager();

    bool isOpen() const noexcept;

    // --- NEW: API called by DatabaseSession when tx is active ---
    template <HItemTemplate T>
    void noteEmplace(Id id)
    {
        require_tx();
        Key k{ItemTypeOf<T>, id};
        auto &op = m_mapCurrentTransaction[k];
        if (op.type == TransactionType{})
        {
            op.type = TransactionType::Emplace;
            op.itemType = ItemTypeVariant{ItemTypeOf<T>, VariantOf<T>};
            op.id = id;
        }
    }

    template <HItemTemplate T>
    void noteErase(Id id, const std::string &str_payload_before)
    {
        require_tx();
        Key k{ItemTypeOf<T>, id};
        auto it = m_mapCurrentTransaction.find(k);
        if (it != m_mapCurrentTransaction.end() && it->second.type == TransactionType::Emplace)
        {
            // inserted then erased within same tx -> cancel the insert immediately
            m_mapCurrentTransaction.erase(it);
            // and physically erase now to keep memory tidy
            // if (auto* store = sess_.try_store(ItemInfor::getInstance().getCategory(k.type)))
            //    store->erase_raw(k.type, id);
            return;
        }
        // First time: snapshot BEFORE bytes, leave object alive (deferred delete)
        auto &op = m_mapCurrentTransaction[k];
        if (op.type == TransactionType{})
        {
            op.type = TransactionType::Erase;
            op.itemType = ItemTypeVariant{ItemTypeOf<T>, VariantOf<T>};
            op.id = id;
            // if (auto* store = sess_.try_store(ItemInfor::getInstance().getCategory(k.type))) {
            //     if (auto const* p = store->get_raw_const(k.type, id)) {
            //         op.payloadBefore = ItemInfor::getInstance().captureTransaction(k.type, p);
            //     }
            // }
            op.payloadBefore = str_payload_before;
        }
        else if (op.type == TransactionType::Modify)
        {
            // promote Modify -> Erase (we already have payloadBefore)
            op.type = TransactionType::Erase;
            op.payloadAfter.clear();
        }
    }

    template <HItemTemplate T>
    bool isNoteErase(Id id) const
    {
        if (!m_bIsOpen)
            return false;
        Key k{ItemTypeOf<T>, id};
        auto it = m_mapCurrentTransaction.find(k);
        return (it != m_mapCurrentTransaction.end() && it->second.type == TransactionType::Erase);
    }

    template <HItemTemplate T>
    bool isNoteEmplace(Id id) const
    {
        if (!m_bIsOpen)
            return false;
        Key k{ItemTypeOf<T>, id};
        auto it = m_mapCurrentTransaction.find(k);
        return (it != m_mapCurrentTransaction.end() && it->second.type == TransactionType::Emplace);
    }

    template <HItemTemplate T>
    bool isNoteModify(Id id) const
    {
        if (!m_bIsOpen)
            return false;
        Key k{ItemTypeOf<T>, id};
        auto it = m_mapCurrentTransaction.find(k);
        return (it != m_mapCurrentTransaction.end() && it->second.type == TransactionType::Modify);
    }

    // First mutable checkout of an existing object
    template <HItemTemplate T>
    void noteCheckOut(Id id, const std::string &str_payload_before)
    {
        require_tx();
        Key k{ItemTypeOf<T>, id};
        auto &op = m_mapCurrentTransaction[k];
        if (op.type == TransactionType{})
        {
            op.type = TransactionType::Modify;
            op.itemType = ItemTypeVariant{ItemTypeOf<T>, VariantOf<T>};
            op.id = id;
            op.payloadBefore = str_payload_before;
        }
        // If Emplace, we don't need snapshot (rollback erases).
        // If Erase, user shouldn't modify; we keep Erase semantics.
    }

    void begin();
    void rollback();
    void commit();
    bool undo(const Transaction *&current_tx);
    bool redo(const Transaction *&current_tx);
    std::unordered_map<Key, TransactionOperation, KeyHash> &get_current_transaction();

private:
    void require_tx() const;

    bool m_bIsOpen = false;
    std::vector<Transaction> m_vecUndoStack;
    std::vector<Transaction> m_vecRedoStack;
    std::unordered_map<Key, TransactionOperation, KeyHash> m_mapCurrentTransaction;
};
