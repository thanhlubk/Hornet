#pragma once
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <array>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <cstdint>
#include <cassert>
#include <cstring> // for std::memcpy used in writeRaw/readRaw
#include "HBaseDef.h"
// #include "DatabaseSession.h"

class DatabaseSession;
class HCursor;

namespace MeasureHelper
{
    // ---------------- size measurement ----------------
    template <class T>
    inline size_t measureTriviallyCopyableObject(const T &)
    {
        static_assert(std::is_trivially_copyable_v<T>, "Requires trivially copyable type");
        return sizeof(T);
    }

    // Create a template first but not implement it so that any type that is not define yet will fail to compile
    // template<class T>
    // inline size_t measure(const T& v);

    // arithmetic
    template <class T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
    inline size_t measure(const T &obj)
    {
        (void)obj;
        return measureTriviallyCopyableObject(obj);
    }

    // ---------------- std::string ----------------
    inline size_t measure(const std::string &obj)
    {
        return sizeof(uint64_t) + obj.size(); // length + bytes
    }

    // --- NEW: measure for enum (serialize underlying type size)
    template <class E>
    inline size_t measure(const E &e) requires std::is_enum_v<E>
    {
        using U = std::underlying_type_t<E>;
        U u = static_cast<U>(e);
        return measure(u); // reuse arithmetic path
    }

    // --- measure for HCursor* as (ItemType + Id)
    inline size_t measure(HCursor *const &)
    {
        return sizeof(ItemType) + sizeof(Id);
    }

    // array
    template <class T, size_t N>
    inline size_t measure(const std::array<T, N> &obj)
    {
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            return sizeof(T) * N;
        }
        else
        {
            size_t size = 0;
            for (auto &member : obj)
                size += measure(member);
            return size;
        }
    }

    // vector
    template <class T>
    inline size_t measure(const std::vector<T> &obj)
    {
        size_t size = sizeof(uint64_t); // length prefix
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            size += obj.size() * sizeof(T);
        }
        else
        {
            for (auto &member : obj)
                size += measure(member);
        }
        return size;
    }

    // list
    template <class T>
    inline size_t measure(const std::list<T> &obj)
    {
        size_t size = sizeof(uint64_t);
        for (auto &member : obj)
            size += measure(member);
        return size;
    }

    // set
    template <class T>
    inline size_t measure(const std::set<T> &obj)
    {
        size_t size = sizeof(uint64_t);
        for (auto &member : obj)
            size += measure(member);
        return size;
    }

    // map
    template <class K, class V>
    inline size_t measure(const std::map<K, V> &obj)
    {
        size_t size = sizeof(uint64_t);
        for (auto &kv : obj)
            size += measure(kv.first) + measure(kv.second);
        return size;
    }

    // ---------- measure for unordered containers ----------
    template <class T, class Hash, class Eq, class Alloc>
    inline size_t measure(const std::unordered_set<T, Hash, Eq, Alloc> &obj)
    {
        size_t size = sizeof(uint64_t);
        for (auto const &e : obj)
            size += measure(e);
        return size;
    }

    template <class K, class V, class Hash, class Eq, class Alloc>
    inline size_t measure(const std::unordered_map<K, V, Hash, Eq, Alloc> &obj)
    {
        size_t size = sizeof(uint64_t);
        for (auto const &kv : obj)
            size += measure(kv.first) + measure(kv.second);
        return size;
    }
}

namespace TransactionHelper
{
    // ---------------- raw writes/reads ----------------
    inline void writeRaw(std::string &out, size_t &offset, const void *src, size_t size)
    {
        assert(offset + size <= out.size());
        if (size)
            std::memcpy(out.data() + offset, src, size);
        offset += size;
    }
    inline bool readRaw(const std::string &in, size_t &offset, void *dst, size_t size)
    {
        if (offset + size > in.size())
            return false;
        if (size)
            std::memcpy(dst, in.data() + offset, size);
        offset += size;
        return true;
    }

    // ==== arithmetic ====
    template <class T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
    inline bool transactionDataExchange(bool capture, std::string &buf, size_t &off, T &v, DatabaseSession *pDb)
    {
        if (capture)
        {
            writeRaw(buf, off, &v, sizeof(T));
            return true;
        }
        else
        {
            return readRaw(buf, off, &v, sizeof(T));
        }
    }

    // ---- NEW: enum exchange via underlying integer ----
    template <class E, std::enable_if_t<std::is_enum_v<E>, int> = 0>
    inline bool transactionDataExchange(bool capture, std::string &buf, size_t &off, E &e, DatabaseSession *pDb)
    {
        using U = std::underlying_type_t<E>;
        if (capture)
        {
            U u = static_cast<U>(e);
            return transactionDataExchange(true, buf, off, u, pDb);
        }
        else
        {
            U u{};
            if (!transactionDataExchange(false, buf, off, u, pDb))
                return false;
            e = static_cast<E>(u);
            return true;
        }
    }

    // ---- HCursor* exchange as (ItemType, Id) <-> pointer
    bool transactionDataExchange(bool capture, std::string &buf, size_t &off, HCursor *&p, DatabaseSession *pDb);

    // ==== std::array ====
    template <class T, size_t N>
    inline bool transactionDataExchange(bool capture, std::string &buf, size_t &off, std::array<T, N> &a, DatabaseSession *pDb)
    {
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            const size_t bytes = sizeof(T) * N;
            if (capture)
            {
                writeRaw(buf, off, a.data(), bytes);
                return true;
            }
            return readRaw(buf, off, a.data(), bytes);
        }
        else
        {
            for (auto &e : a)
                if (!transactionDataExchange(capture, buf, off, e, pDb))
                    return false;
            return true;
        }
    }

    // ==== std::vector ====
    template <class T>
    inline bool transactionDataExchange(bool capture, std::string &buf, size_t &off, std::vector<T> &v, DatabaseSession *pDb)
    {
        uint64_t n = capture ? static_cast<uint64_t>(v.size()) : 0;
        if (!transactionDataExchange(capture, buf, off, n, pDb))
            return false;
        if (!capture)
            v.resize(static_cast<size_t>(n));
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            const size_t bytes = sizeof(T) * static_cast<size_t>(n);
            if (capture)
            {
                if (n)
                    writeRaw(buf, off, v.data(), bytes);
                return true;
            }
            return n ? readRaw(buf, off, v.data(), bytes) : true;
        }
        else
        {
            for (size_t i = 0; i < static_cast<size_t>(n); ++i)
                if (!transactionDataExchange(capture, buf, off, v[i], pDb))
                    return false;
            return true;
        }
    }

    // ==== std::list ====
    template <class T>
    inline bool transactionDataExchange(bool capture, std::string &buf, size_t &off, std::list<T> &lst, DatabaseSession *pDb)
    {
        uint64_t n = capture ? static_cast<uint64_t>(lst.size()) : 0;
        if (!transactionDataExchange(capture, buf, off, n, pDb))
            return false;
        if (capture)
        {
            for (auto &e : lst)
                if (!transactionDataExchange(true, buf, off, e, pDb))
                    return false;
            return true;
        }
        else
        {
            lst.clear();
            for (size_t i = 0; i < static_cast<size_t>(n); ++i)
            {
                T tmp{};
                if (!transactionDataExchange(false, buf, off, tmp, pDb))
                    return false;
                lst.push_back(std::move(tmp));
            }
            return true;
        }
    }

    // ==== std::set (ordered) ====
    template <class T, class Cmp, class Alloc>
    inline bool transactionDataExchange(bool capture, std::string &buf, size_t &off, std::set<T, Cmp, Alloc> &s, DatabaseSession *pDb)
    {
        uint64_t n = capture ? static_cast<uint64_t>(s.size()) : 0;
        if (!transactionDataExchange(capture, buf, off, n, pDb))
            return false;
        if (capture)
        {
            // const_cast is OK: capture branch won’t mutate the element
            for (auto &e : s)
                if (!transactionDataExchange(true, buf, off, const_cast<T &>(e), pDb))
                    return false;
            return true;
        }
        else
        {
            s.clear();
            for (size_t i = 0; i < static_cast<size_t>(n); ++i)
            {
                T tmp{};
                if (!transactionDataExchange(false, buf, off, tmp, pDb))
                    return false;
                s.insert(std::move(tmp));
            }
            return true;
        }
    }

    // ==== std::map (ordered) ====
    template <class K, class V, class Cmp, class Alloc>
    inline bool transactionDataExchange(bool capture, std::string &buf, size_t &off, std::map<K, V, Cmp, Alloc> &m, DatabaseSession *pDb)
    {
        uint64_t n = capture ? static_cast<uint64_t>(m.size()) : 0;
        if (!transactionDataExchange(capture, buf, off, n, pDb))
            return false;
        if (capture)
        {
            for (auto &kv : m)
            {
                if (!transactionDataExchange(true, buf, off, const_cast<K &>(kv.first), pDb))
                    return false;
                if (!transactionDataExchange(true, buf, off, const_cast<V &>(kv.second), pDb))
                    return false;
            }
            return true;
        }
        else
        {
            m.clear();
            for (size_t i = 0; i < static_cast<size_t>(n); ++i)
            {
                K k{};
                V v{};
                if (!transactionDataExchange(false, buf, off, k, pDb))
                    return false;
                if (!transactionDataExchange(false, buf, off, v, pDb))
                    return false;
                m.emplace(std::move(k), std::move(v));
            }
            return true;
        }
    }

    // ==== std::set (unordered) ====
    template <class T, class Hash, class Eq, class Alloc>
    inline bool transactionDataExchange(bool capture, std::string &buf, size_t &off,
                                        std::unordered_set<T, Hash, Eq, Alloc> &s, DatabaseSession *pDb)
    {
        uint64_t n = capture ? static_cast<uint64_t>(s.size()) : 0;
        if (!transactionDataExchange(capture, buf, off, n, pDb))
            return false;

        if (capture)
        {
            for (auto const &e : s)
            {
                if (!transactionDataExchange(true, buf, off, const_cast<T &>(e), pDb))
                    return false;
            }
            return true;
        }
        else
        {
            s.clear();
            for (size_t i = 0; i < static_cast<size_t>(n); ++i)
            {
                T tmp{};
                if (!transactionDataExchange(false, buf, off, tmp, pDb))
                    return false;
                s.insert(std::move(tmp));
            }
            return true;
        }
    }

    // ==== std::map (unordered) ====
    template <class K, class V, class Hash, class Eq, class Alloc>
    inline bool transactionDataExchange(bool capture, std::string &buf, size_t &off,
                                        std::unordered_map<K, V, Hash, Eq, Alloc> &m, DatabaseSession *pDb)
    {
        uint64_t n = capture ? static_cast<uint64_t>(m.size()) : 0;
        if (!transactionDataExchange(capture, buf, off, n, pDb))
            return false;

        if (capture)
        {
            for (auto const &kv : m)
            {
                if (!transactionDataExchange(true, buf, off, const_cast<K &>(kv.first), pDb))
                    return false;
                if (!transactionDataExchange(true, buf, off, const_cast<V &>(kv.second), pDb))
                    return false;
            }
            return true;
        }
        else
        {
            m.clear();
            for (size_t i = 0; i < static_cast<size_t>(n); ++i)
            {
                K k{};
                V v{};
                if (!transactionDataExchange(false, buf, off, k, pDb))
                    return false;
                if (!transactionDataExchange(false, buf, off, v, pDb))
                    return false;
                m.emplace(std::move(k), std::move(v));
            }
            return true;
        }
    }

    // ==== std::string ====
    inline bool transactionDataExchange(bool capture, std::string &buf, size_t &off, std::string &s, DatabaseSession *pDb)
    {
        uint64_t n = capture ? static_cast<uint64_t>(s.size()) : 0;
        if (!transactionDataExchange(capture, buf, off, n, pDb))
            return false;
        if (capture)
        {
            if (n)
                writeRaw(buf, off, s.data(), static_cast<size_t>(n));
            return true;
        }
        else
        {
            const size_t need = static_cast<size_t>(n);
            if (off + need > buf.size())
                return false;
            s.assign(buf.data() + off, buf.data() + off + need);
            off += need;
            return true;
        }
    }

    // Turn a const member reference into a mutable one (safe for capture path)
    template<class M>
    static auto& asMutable(const M& ref)
    {
        using U = std::remove_cv_t<std::remove_reference_t<M>>;
        return const_cast<U&>(ref);
    }

    // Capture a sequence of members into a single buffer (pre-sized, single pass)
    template <class T, class... MPtrs>
    std::string captureMembers(const T &self, DatabaseSession *pDb, MPtrs... mptrs)
    {
        static_assert((std::is_member_object_pointer_v<MPtrs> && ...), "members only");
        // compute total bytes once
        size_t total = (size_t{0} + ... + MeasureHelper::measure(self.*mptrs));
        std::string out;
        out.resize(total);
        size_t off = 0;
        (transactionDataExchange(true, out, off, asMutable(self.*mptrs), pDb), ...);
        // (optional) assert all written
        // assert(off == out.size());
        return out;
    }

    // Restore a sequence of members from a single buffer (single pass)
    template <class T, class... MPtrs>
    bool restoreMembers(T &self, std::string &bytes, DatabaseSession *pDb, MPtrs... mptrs)
    {
        static_assert((std::is_member_object_pointer_v<MPtrs> && ...), "members only");
        size_t off = 0;
        bool ok = (true && ... && transactionDataExchange(false, bytes, off, self.*mptrs, pDb));
        // (optional) ok = ok && (off == bytes.size());
        return ok;
    }

} // namespace TransactionHelper

#define DEFINE_TRANSACTION_EXCHANGE(T, /* pointers-to-members... */...)                         \
    static std::string captureTransactionItem(const T &self, DatabaseSession *pDb)              \
    {                                                                                           \
        return TransactionHelper::captureMembers(self, pDb, __VA_ARGS__);                       \
    }                                                                                           \
    static void restoreTransactionItem(T &self, const std::string &bytes, DatabaseSession *pDb) \
    {                                                                                           \
        auto &nonConstBytes = const_cast<std::string &>(bytes);                                 \
        (void)TransactionHelper::restoreMembers(self, nonConstBytes, pDb, __VA_ARGS__);           \
    }