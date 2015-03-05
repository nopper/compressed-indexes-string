#pragma once

#include "ps/problems/schemes.hpp"
#include "ps/indices/index_types.hpp"

namespace ps {
namespace problems {
namespace intersection {
namespace detail {

template<typename IndexEnumerator>
void do_enumerator_intersection(IndexEnumerator& en,
                                const int l,
                                const int r,
                                std::vector<uint64_t>& result)
{
    if (en.docid() < (uint64_t)l)
        en.next_geq((uint64_t)l);

    if (PS_UNLIKELY(en.docid() < (uint64_t)l ||
                    en.docid() >= (uint64_t)r ||
                    en.position() == en.size()))
        return;

    for (size_t i = en.position(); i < en.size(); ++i, en.next())
    {
        uint64_t docid = en.docid();

        if (docid < (uint64_t)r)
            result.push_back(docid);
        else
            return;
    }
}

template<typename IndexEnumerator>
void do_enumerator_intersection(IndexEnumerator& en,
                                std::vector<uint64_t>& remapping,
                                std::vector<uint64_t>& result)
{
    for (size_t i = 0; i < remapping.size(); ++i)
    {
        uint64_t l = remapping[i];
        if (remapping[i] == UINT_MAX || en.docid() > l)
            continue;

        en.next_geq(l);

        if (PS_UNLIKELY(en.docid() < l ||
                        en.position() == en.size()))
            return;

        if (en.docid() == l)
        {
            remapping[i] = UINT_MAX;
            result.push_back(l);
        }
    }
}

template<typename IndexEnumerator>
void do_enumerator_fast_intersection(IndexEnumerator& en,
                                     const int l,
                                     const int r,
                                     const std::vector<uint64_t>& remapping,
                                     std::vector<uint64_t>& result)
{
    for (size_t i = en.position(); i < en.size(); ++i, en.next())
    {
        uint64_t docid = en.docid();
        uint64_t position = remapping[docid];
        if (PS_UNLIKELY((uint64_t)l <= position && position < (uint64_t)r))
            result.push_back(docid);
    }
}

}

template<typename Index, Scheme Method>
class solver {
public:
    solver(const Index& index)
        : m_index(index)
    {}

    void solve(uint64_t docid, int l, int r, std::vector<uint64_t>& res)
    {
        throw std::runtime_error("Not supported");
    }

    void solve_baseline_hopping(uint64_t docid, std::vector<uint64_t>& remapping, std::vector<uint64_t>& result)
    {
        uint64_t offset;
        if (!m_index.get_offset(docid, offset))
            return;

        solve_asindex_inline(docid, remapping, result);

        auto en = m_index.sequence_at(offset);

        for (size_t i = 0; i < en.size(); ++i, en.next())
            solve_asindex_inline(en.docid(), remapping, result);

        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());
        result.erase(std::remove(result.begin(), result.end(), docid), result.end());
    }

    void solve_baseline_asindex(uint64_t docid, std::vector<uint64_t>& remapping, std::vector<uint64_t>& result)
    {
        solve_asindex_inline(docid, remapping, result);
    }

    void solve_fast_baseline_hopping(uint64_t docid, int l, int r,
                                     const std::vector<uint64_t>& remapping, std::vector<uint64_t>& result)
    {
        uint64_t offset;
        if (!m_index.get_offset(docid, offset))
            return;

        solve_fast_asindex_inline(docid, l, r, remapping, result);

        auto en = m_index.sequence_at(offset);

        for (size_t i = 0; i < en.size(); ++i, en.next())
            solve_fast_asindex_inline(en.docid(), l, r, remapping, result);

        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());
        result.erase(std::remove(result.begin(), result.end(), docid), result.end());
    }

    void solve_fast_baseline_asindex(uint64_t docid, int l, int r,
                                     const std::vector<uint64_t>& remapping, std::vector<uint64_t>& result)
    {
        solve_fast_asindex_inline(docid, l, r, remapping, result);
    }

protected:
    void solve_hopping(uint64_t docid, int l, int r, std::vector<uint64_t>& result)
    {
        uint64_t offset;
        if (!m_index.get_offset(docid, offset))
            return;

        solve_asindex_inline(docid, l, r, result);

        auto en = m_index.sequence_at(offset);

        for (size_t i = 0; i < en.size(); ++i, en.next())
            solve_asindex_inline(en.docid(), l, r, result);

        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());
        result.erase(std::remove(result.begin(), result.end(), docid), result.end());
    }

    void solve_asindex(uint64_t docid, int l, int r, std::vector<uint64_t>& result)
    {
        solve_asindex_inline(docid, l, r, result);
    }

    void PS_ALWAYSINLINE solve_asindex_inline(uint64_t docid, int l, int r, std::vector<uint64_t>& result)
    {
        uint64_t offset;
        if (!m_index.get_offset(docid, offset))
            return;

        auto en = m_index.sequence_at(offset);
        return detail::do_enumerator_intersection(en, l, r, result);
    }

    void PS_ALWAYSINLINE solve_asindex_inline(uint64_t docid,
                                              std::vector<uint64_t>& remapping,
                                              std::vector<uint64_t>& result)
    {
        uint64_t offset;
        if (!m_index.get_offset(docid, offset))
            return;

        auto en = m_index.sequence_at(offset);
        return detail::do_enumerator_intersection(en, remapping, result);
    }

    void PS_ALWAYSINLINE solve_fast_asindex_inline(uint64_t docid,
                                                   int l, int r,
                                                   const std::vector<uint64_t>& remapping,
                                                   std::vector<uint64_t>& result)
    {
        uint64_t offset;
        if (!m_index.get_offset(docid, offset))
            return;

        auto en = m_index.sequence_at(offset);
        return detail::do_enumerator_fast_intersection(en, l, r, remapping, result);
    }

    const Index& m_index;
};

// Hopping is only defined for simple indices
#define LOOP_BODY(R, DATA, T)                                           \
template<>                                                              \
void solver<BOOST_PP_CAT(indices::T, _index), Schemes::hopping>::solve( \
    uint64_t docid, int l, int r, std::vector<uint64_t>& res)           \
{                                                                       \
    solve_hopping(docid, l, r, res);                                    \
}

BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_SIMPLE_INDEX_TYPES);
#undef LOOP_BODY

// AsIndex is only defined for simple indices
#define LOOP_BODY(R, DATA, T)                                           \
template<>                                                              \
void solver<BOOST_PP_CAT(indices::T, _index), Schemes::asindex>::solve( \
    uint64_t docid, int l, int r, std::vector<uint64_t>& res)           \
{                                                                       \
    solve_asindex(docid, l, r, res);                                    \
}

BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_SIMPLE_INDEX_TYPES);
#undef LOOP_BODY

}
}
}
