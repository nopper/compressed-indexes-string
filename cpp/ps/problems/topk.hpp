#pragma once

#include <boost/chrono.hpp>
#include "ps/problems/schemes.hpp"
#include "ps/containers.hpp"
#include "ps/indices/index_types.hpp"

namespace ps {
namespace problems {
namespace topk {

typedef std::pair<uint64_t,uint64_t> ranked_docid;

class compare_rank_docids {
public:
    compare_rank_docids(const bool& reverse = false)
        : m_reverse(reverse)
    {}

    bool operator()(const ranked_docid& a, const ranked_docid& b) const
    {
        int doc_diff = a.first - b.first;
        int rank_diff = a.second - b.second;

        if (m_reverse)
            return rank_diff == 0 ? doc_diff > 0 : rank_diff > 0;
        else
            return rank_diff == 0 ? doc_diff < 0 : rank_diff < 0;
    }
protected:
    bool m_reverse;
};

struct hash_pair {
public:
    template <typename T, typename U>
    std::size_t operator()(const std::pair<T, U> &x) const
    {
        return std::hash<T>()(x.first);
    }
};

typedef containers::unique_fixed_priority_queue<ranked_docid,
                                                std::vector<ranked_docid>,
                                                compare_rank_docids,
                                                std::unordered_set<ranked_docid, hash_pair>> topk_heap;

template <typename Sequence>
struct mw_merge {

    typedef typename ps::indices::topk_index<Sequence>::rmq_sequence rmq_sequence;

    class compare_rmq_sequences {
    public:
        compare_rmq_sequences(const bool& reverse = false)
            : m_reverse(reverse)
        {}

        bool operator()(const rmq_sequence& a, const rmq_sequence& b) const
        {
            int doc_diff = std::get<0>(a.value()) - std::get<0>(b.value());
            int rank_diff = std::get<1>(a.value()) - std::get<1>(b.value());

            if (m_reverse)
                return rank_diff == 0 ? doc_diff > 0 : rank_diff > 0;
            else
                return rank_diff == 0 ? doc_diff < 0 : rank_diff < 0;
        }
    protected:
        bool m_reverse;
    };

    typedef containers::reinsertable_priority_queue<rmq_sequence, std::vector<rmq_sequence>, compare_rmq_sequences> topk_rmq_heap;
};

namespace detail {

template<typename IndexEnumerator>
void do_enumerator_intersection(IndexEnumerator& en,
                                const int l,
                                const int r,
                                topk_heap& heap,
                                const std::vector<uint64_t>& ranking)
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
        uint64_t rank = ranking[docid];

        if (docid < (uint64_t)r)
            heap.push(std::make_pair(docid, rank));
        else
            break;
    }
}

}

template<typename Index, Scheme Method>
class solver {
    typedef typename mw_merge<typename Index::sequence_type>::topk_rmq_heap topk_rmq_heap;
    typedef typename mw_merge<typename Index::sequence_type>::rmq_sequence rmq_sequence;
public:
    solver(const Index& index,
           const std::vector<uint64_t>& ranking,
           const std::vector<uint64_t>& wand,
           int k)
        : m_index(index)
        , m_ranking(ranking)
        , m_wand(wand)
        , m_k(k)
    {}

    void solve(uint64_t docid, int l, int r, std::vector<uint64_t>& res)
    {
        throw std::runtime_error("Not supported");
    }

protected:
    void solve_hopping(uint64_t docid, int l, int r, std::vector<uint64_t>& result)
    {
        uint64_t offset;
        if (!m_index.get_offset(docid, offset))
            return;

        topk_heap heap(m_k, std::make_pair(docid, m_ranking[docid]));

        auto en = m_index.sequence_at(offset);

        solve_asindex_inline(docid, l, r, heap);

        for (size_t i = en.position(); i < en.size(); ++i, en.next())
            solve_asindex_inline(en.docid(), l, r, heap);

        int extracted = 0;

        while (!heap.empty())
        {
            result[extracted++] = heap.top().first;
            heap.pop();
        }

        result.resize(extracted);
    }

    void solve_hopping_wand(uint64_t docid, int l, int r, std::vector<uint64_t>& result)
    {
        uint64_t offset;
        if (!m_index.get_offset(docid, offset))
            return;

        auto en = m_index.sequence_at(offset);

        topk_heap heap(m_k, std::make_pair(docid, m_ranking[docid]));
        std::vector<std::pair<uint64_t, uint64_t>> docids;

        docids.emplace_back(std::make_pair(docid, m_wand.at(docid)));

        for (size_t i = en.position(); i < en.size(); ++i, en.next())
        {
            uint64_t docid = en.docid();
            docids.emplace_back(docid, m_wand[docid]);
        }

        std::sort(docids.begin(), docids.end(), [](const std::pair<uint64_t,uint64_t>& a,
                                                   const std::pair<uint64_t,uint64_t>& b) {
            return a.second > b.second;
        });

        for (auto& p: docids)
        {
            if (heap.full() && p.second < heap.minimum().second)
                break;

            solve_asindex_inline(p.first, l, r, heap);
        }

        int extracted = 0;

        while (!heap.empty())
        {
            result[extracted++] = heap.top().first;
            heap.pop();
        }

        result.resize(extracted);
    }

    void solve_hopping_rmq_wand(uint64_t docid, int l, int r, std::vector<uint64_t>& result)
    {
        uint64_t offset;
        if (!m_index.get_offset(docid, offset))
            return;

        result.resize(result.size() * 2);
        auto en = m_index.sequence_at(offset);

        topk_rmq_heap rmq_heap;

        topk_heap wand_heap(m_k, std::make_pair(docid, m_ranking[docid]));
        std::vector<std::pair<uint64_t, uint64_t>> docids;

        if (!solve_rmq_wand_inline(docid, l, r, rmq_heap))
            docids.emplace_back(docid, m_wand[docid]);

        for (size_t i = 0; i < en.size(); ++i, en.next())
        {
            if (!solve_rmq_wand_inline(en.docid(), l, r, rmq_heap))
                docids.emplace_back(en.docid(), m_wand[en.docid()]);
        }

        std::sort(docids.begin(), docids.end(), [](const std::pair<uint64_t,uint64_t>& a,
                                                   const std::pair<uint64_t,uint64_t>& b) {
            return a.second > b.second;
        });

        uint64_t previous = UINT_MAX;
        int extracted = 0;

        while (!rmq_heap.empty() && extracted < m_k)
        {
            rmq_sequence& s = rmq_heap.top();

            uint64_t target = std::get<0>(s.value());

            if (target != docid && target != previous)
            {
                result[extracted++] = target;
                previous = target;
            }

            if (s.next())
                rmq_heap.reinsert();
            else
                rmq_heap.pop();
        }

        uint64_t min_rank = (previous != UINT_MAX && extracted >= m_k) ? m_ranking[previous] : 0;

        for (auto& p: docids)
        {
            if (p.second < min_rank || (wand_heap.full() && p.second < wand_heap.minimum().second))
                break;

            solve_asindex_inline(p.first, l, r, wand_heap);
        }

        while (!wand_heap.empty())
        {
            result[extracted++] = wand_heap.top().first;
            wand_heap.pop();
        }

        result.resize(extracted);

        std::sort(result.begin(), result.end(), [=](const uint64_t a, const uint64_t b) {
            return m_ranking[a] > m_ranking[b];
        });

        result.erase(std::unique(result.begin(), result.end()), result.end());
        result.resize(std::min((size_t)m_k, result.size()));
    }

    void solve_hopping_rmq(uint64_t docid, int l, int r, std::vector<uint64_t>& result)
    {
        topk_rmq_heap heap;

        uint64_t offset;
        if (!m_index.get_offset(docid, offset))
            return;

        solve_rmq_inline(docid, l, r, heap);

        auto en = m_index.sequence_at(offset);
        for (size_t i = 0; i < en.size(); ++i, en.next())
            solve_rmq_inline(en.docid(), l, r, heap);

        uint64_t previous = UINT_MAX;
        int extracted = 0;

        while (!heap.empty() && extracted < m_k)
        {
            rmq_sequence& s = heap.top();

            uint64_t target = std::get<0>(s.value());

            if (target != docid && target != previous)
            {
                result[extracted++] = target;
                previous = target;
            }

            if (s.next())
                heap.reinsert();
            else
                heap.pop();
        }

        result.resize(extracted);
    }

    bool PS_ALWAYSINLINE solve_rmq_wand_inline(uint64_t docid, int l, int r,
                                               topk_rmq_heap& heap)
    {
        uint64_t offset;
        if (!m_index.get_offset(docid, offset))
            return true;

        auto en = m_index.sequence_at(offset);
        uint64_t threshold = configuration::get().indices_topk_rmq_wand_threshold;

        if (en.size() < threshold)
            return false;

        if (en.docid() < (uint64_t)l)
            en.next_geq((uint64_t)l);

        if (PS_UNLIKELY(en.docid() < (uint64_t)l ||
                        en.docid() >= (uint64_t)r ||
                        en.position() == en.size()))
            return true;

        uint64_t a = en.position();
        en.next_geq((uint64_t)r);
        uint64_t b = en.position();

        if (PS_LIKELY(b > a))
            b--;

        rmq_sequence seq(docid, en);
        m_index.get_rmq_sequence(docid, seq);
        seq.rmq(a, b);

        if (seq.next())
            heap.push(seq);

        return true;
    }

    void PS_ALWAYSINLINE solve_rmq_inline(uint64_t docid, int l, int r,
                                          topk_rmq_heap& heap)
    {
        uint64_t offset;

        if (!m_index.get_offset(docid, offset))
            return;

        auto en = m_index.sequence_at(offset);

        if (en.docid() < (uint64_t)l)
            en.next_geq((uint64_t)l);

        if (PS_UNLIKELY(en.docid() < (uint64_t)l ||
                        en.docid() >= (uint64_t)r ||
                        en.position() == en.size()))
            return;

        uint64_t a = en.position();
        en.next_geq((uint64_t)r);
        uint64_t b = en.position();

        if (PS_LIKELY(b > a))
            b--;

        rmq_sequence seq(docid, en);
        m_index.get_rmq_sequence(docid, seq);
        seq.rmq(a, b);

        if (seq.next())
            heap.push(seq);
    }

    void PS_ALWAYSINLINE solve_asindex_inline(uint64_t docid, int l, int r, topk_heap& heap)
    {
        uint64_t offset;
        if (!m_index.get_offset(docid, offset))
            return;

        auto en = m_index.sequence_at(offset);
        return detail::do_enumerator_intersection(en, l, r, heap, m_ranking);
    }

    const Index& m_index;
    const std::vector<uint64_t>& m_ranking;
    const std::vector<uint64_t>& m_wand;
    int m_k;
};

#define LOOP_BODY(R, DATA, T)                                                \
template<>                                                                   \
void solver<BOOST_PP_CAT(indices::T, _index), Schemes::topk_hopping>::solve( \
    uint64_t docid, int l, int r, std::vector<uint64_t>& res)                \
{                                                                            \
    solve_hopping(docid, l, r, res);                                         \
}

BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_SIMPLE_INDEX_TYPES);
#undef LOOP_BODY

#define LOOP_BODY(R, DATA, T)                                                     \
template<>                                                                        \
void solver<BOOST_PP_CAT(indices::T, _index), Schemes::topk_hopping_wand>::solve( \
    uint64_t docid, int l, int r, std::vector<uint64_t>& res)                     \
{                                                                                 \
    solve_hopping_wand(docid, l, r, res);                                         \
}

BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_SIMPLE_INDEX_TYPES);
#undef LOOP_BODY

// RMQ is only defined for topk indices
#define LOOP_BODY(R, DATA, T)                                                    \
template<>                                                                       \
void solver<BOOST_PP_CAT(indices::T, _index), Schemes::topk_hopping_rmq>::solve( \
    uint64_t docid, int l, int r, std::vector<uint64_t>& res)                    \
{                                                                                \
    solve_hopping_rmq(docid, l, r, res);                                         \
}

BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_TOPK_INDEX_TYPES);
#undef LOOP_BODY

// RMQWAND is only defined for topk indices
#define LOOP_BODY(R, DATA, T)                                                         \
template<>                                                                            \
void solver<BOOST_PP_CAT(indices::T, _index), Schemes::topk_hopping_rmq_wand>::solve( \
    uint64_t docid, int l, int r, std::vector<uint64_t>& res)                         \
{                                                                                     \
    solve_hopping_rmq_wand(docid, l, r, res);                                         \
}

BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_TOPK_INDEX_TYPES);
#undef LOOP_BODY

}
}
}
