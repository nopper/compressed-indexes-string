#pragma once

#include <memory>
#include "succinct/mapper.hpp"
#include "succinct/topk_vector.hpp"
#include "ps/indices/simple_index.hpp"
#include "ps/utils.hpp"
#include "ps/indices/builders/rmq_sequences.hpp"
#include "ps/configuration.hpp"

namespace ps {
namespace indices {

template <typename Sequence>
class topk_index
{
public:
    typedef Sequence sequence_type;

    topk_index(const char* filename)
        : m_seq_postings(filename, ".pos")
        , m_seq_offsets(filename, ".off")
        , m_seq_degrees(filename, ".deg")
        , m_seq_rankings(filename, ".rnk")
        , m_rmq_sequences(filename)
    {
        auto en_offs = m_seq_offsets.sequence_at(0);
        auto en_deg = m_seq_degrees.sequence_at(0);
        auto en_ranks = m_seq_rankings.sequence_at(0);

        uint64_t prev_rank = 0;
        m_docs.resize(en_offs.size());

        for (size_t i = 0; i < en_offs.size(); ++i, en_offs.next(), en_deg.next(), en_ranks.next())
        {
            uint64_t rank = abs((int64_t)en_ranks.docid() - (int64_t)prev_rank);
            m_docs[i] = node_info(i, en_offs.docid(), en_deg.docid() - 1 - i, rank);
            prev_rank = en_ranks.docid() + 1;
        }
    }

    topk_index(sequences::options opts, const char* filename, const std::string& ranking_file)
        : m_seq_postings(opts, filename, ".pos")
        , m_seq_offsets(opts, filename, ".off")
        , m_seq_degrees(opts, filename, ".deg")
        , m_seq_rankings(opts, filename, ".rnk")
        , m_rmq_sequences(opts, filename)
    {
        m_ranking.resize(opts.universe);
        if (!ps::util::read_ranking_from(ranking_file, m_ranking))
            throw std::runtime_error("Unable to load ranking file");
    }

    class builder {
    public:
        builder(topk_index& file)
            : m_file(file)
            , m_last_docid(0)
            , m_last_offset(0)
            , m_cdf_degree(0)
            , m_seq_postings_builder(file.m_seq_postings)
        {}

        void commit()
        {
            uint64_t universe = m_file.m_seq_postings.opts().universe;

            while (m_last_docid < universe) {
                m_last_docid++;
                m_offsets.emplace_back(m_last_offset);
                m_cdf_degrees.emplace_back(++m_cdf_degree);
            }

            m_file.m_docs.resize(universe);

            for (uint64_t i = 0; i < m_file.m_docs.size(); ++i)
                m_file.m_docs[i] = node_info(i, m_offsets[i], m_cdf_degrees[i] - 1 - i, m_file.m_ranking[i]);

            typename sequences::sequence_file<sequences::ef_seq>::builder offsets_builder(
                sequences::options(m_last_offset + 1), m_file.m_seq_offsets);

            typename sequences::sequence_file<sequences::ef_seq>::builder degrees_builder(
                sequences::options(m_cdf_degree + 1), m_file.m_seq_degrees);

            offsets_builder.append(m_offsets.size(), m_offsets.begin());
            degrees_builder.append(m_cdf_degrees.size(), m_cdf_degrees.begin());

            for (uint64_t i = 1; i < m_file.m_ranking.size(); ++i)
                m_file.m_ranking[i] += m_file.m_ranking[i - 1] + 1;

            typename sequences::sequence_file<sequences::ef_seq>::builder ranking_builder(
                sequences::options(m_file.m_ranking[m_file.m_ranking.size() - 1] + 1), m_file.m_seq_rankings);

            ranking_builder.append(m_file.m_ranking.size(), m_file.m_ranking.begin());

            m_seq_postings_builder.commit();
            offsets_builder.commit();
            degrees_builder.commit();
            ranking_builder.commit();

            build_cartesian_tree();

            m_offsets.clear();
            m_cdf_degrees.clear();
            m_file.m_ranking.clear();
        }

        void append(uint64_t docid, std::string& str, uint64_t num_elements = 1)
        {
            uint64_t offset = m_seq_postings_builder.append(str, num_elements);

            assert(offset > m_last_offset || offset == 0);
            assert(docid >= m_last_docid || docid == 0);

            while (m_last_docid < docid) {
                m_last_docid++;
                m_offsets.emplace_back(m_last_offset);
                m_cdf_degrees.emplace_back(++m_cdf_degree);
            }

            m_last_docid = docid + 1;
            m_last_offset = offset;
            m_cdf_degree += num_elements + 1;

            m_offsets.emplace_back(offset);
            m_cdf_degrees.emplace_back(m_cdf_degree);
        }

    protected:
        void build_cartesian_tree()
        {
            // XX: not properly cartesian trees since they are bucketed
            progress_logger plog("cartesian_tree");
            uint64_t prev_cdf = 0;

            topk::rmq_sequences::builder builder(m_file.m_rmq_sequences);

            for (uint64_t i = 0; i < m_file.m_docs.size(); ++i)
            {
                node_info& n = m_file.m_docs[i];
                std::vector<uint64_t> scores;

                if (n.cdf_degree - prev_cdf > 0)
                {
                    auto en = m_file.sequence_at(n.offset);

                    for (uint64_t j = 0; j < en.size(); ++j, en.next())
                    {
                        uint64_t score = m_file.m_ranking[en.docid()];

                        if (en.docid() > 0)
                            score -= m_file.m_ranking[en.docid() - 1] + 1;

                        scores.push_back(score);
                    }
                }

                builder.append(n.docid, n.cdf_degree, scores.size(), scores.begin());

                prev_cdf = n.cdf_degree;
                plog.done_item();
            }

            builder.commit();
            plog.done();
        }

        topk_index& m_file;
        std::vector<uint64_t> m_offsets;
        std::vector<uint64_t> m_cdf_degrees;

        uint64_t m_last_docid;
        uint64_t m_last_offset;
        uint64_t m_cdf_degree;

        typename sequences::sequence_file<Sequence>::builder m_seq_postings_builder;
    };

    uint64_t degree(uint64_t docid) const
    {
        return m_docs[docid].cdf_degree - ((docid == 0) ? 0 : m_docs[docid - 1].cdf_degree);
    }

    bool get_offset(uint64_t docid, uint64_t& offset) const
    {
        offset = m_docs[docid].offset;
        return degree(docid) > 0;
    }

    class rmq_sequence;

    bool get_rmq_sequence(uint64_t docid, rmq_sequence& s) const
    {
        uint64_t idx = m_rmq_sequences.get_index(docid);
        uint64_t cdf_degree = (docid == 0 ? 0 : m_docs[docid - 1].cdf_degree);

        if (idx > 0)
            s.m_topk_start_offset = cdf_degree - m_rmq_sequences.get_degree(idx);
        else
            s.m_topk_start_offset = cdf_degree;

        s.m_cartesian_tree = m_rmq_sequences.get_cartesian_tree(idx);
        s.m_docs = &m_docs;

        return true;
    }

    typename Sequence::enumerator sequence_at(uint64_t offset) const
    {
        return m_seq_postings.sequence_at(offset);
    }

    uint64_t num_docs() const
    {
        return m_docs.size();
    }

    uint64_t num_elements() const
    {
        return m_seq_postings.num_elements();
    }

    uint64_t construction_time_microsec() const
    {
        return m_seq_postings.construction_time_microsec();
    }

    sequences::options opts() const
    {
        return m_seq_postings.opts();
    }

    friend std::ostream& operator<<(std::ostream& os, const topk_index& index)
    {
        size_t num_docs = index.num_docs();
        size_t num_elements = index.num_elements();
        size_t node_bytes = index.m_seq_offsets.file_size();
        size_t postings_bytes = index.m_seq_postings.file_size();

        os << "docs=" << num_docs
           << " elements=" << num_elements
           << " docs-bytes=" << node_bytes
           << " postings-bytes=" << postings_bytes
           << " elements/node=" << ((double)num_elements / num_docs)
           << " bits/doc=" << ((double)postings_bytes * 8.0 / num_docs)
           << " bits/element=" << ((double)postings_bytes * 8.0 / num_elements)
           << " construction_time_microsec=" << index.construction_time_microsec();

        return os;
    }

    class rmq_sequence {
        friend class topk_index;

        typedef std::tuple<uint64_t, uint64_t, uint64_t> entry_type;
        typedef std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t> queue_element_type;

        struct value_index_comparator {
            template <typename Tuple>
            bool operator()(Tuple const& a, Tuple const& b) const
            {
                using std::get;
                return (get<1>(a) < get<1>(b) ||
                        (get<1>(a) == get<1>(b) && get<2>(a) > get<2>(b)));
            }
        };

    public:
        rmq_sequence(uint64_t docid,
                     typename Sequence::enumerator enumerator,
                     uint64_t threshold = configuration::get().indices_topk_threshold)
            : m_docid(docid)
            , m_enumerator(enumerator)
            , m_threshold(threshold)
            , m_sorted(false)
        {}

        void rmq(uint64_t a, uint64_t b)
        {
            m_q.clear();

            if ((b - a + 1) <= m_threshold)
                extract_all_in(a, b);
            else
                extract_minimum_in(a, b);
        }

        bool next()
        {
            if (m_q.empty())
                return false;

            uint64_t cur_docid;
            uint64_t cur_score;
            uint64_t cur_pos, cur_a, cur_b;

            if (!m_sorted)
                std::pop_heap(m_q.begin(), m_q.end(), value_index_comparator());

            std::tie(cur_docid, cur_score, cur_pos, cur_a, cur_b) = m_q.back();
            m_q.pop_back();

            m_cur = entry_type(cur_docid, cur_score, cur_pos);

            if (m_sorted)
                return true;

            if (cur_pos != cur_a) {
                uint64_t pos = m_cartesian_tree->rmq(m_topk_start_offset + cur_a, m_topk_start_offset + cur_pos - 1) - m_topk_start_offset;

                m_enumerator.move(pos);
                uint64_t docid = m_enumerator.docid();
                uint64_t score = (*m_docs)[docid].score;

                m_q.emplace_back(docid, score, pos, cur_a, cur_pos - 1);
                std::push_heap(m_q.begin(), m_q.end(), value_index_comparator());
            }

            if (cur_pos != cur_b) {
                uint64_t pos = m_cartesian_tree->rmq(m_topk_start_offset + cur_pos + 1, m_topk_start_offset + cur_b) - m_topk_start_offset;

                m_enumerator.move(pos);
                uint64_t docid = m_enumerator.docid();
                uint64_t score = (*m_docs)[docid].score;

                m_q.emplace_back(docid, score, pos, cur_pos + 1, cur_b);
                std::push_heap(m_q.begin(), m_q.end(), value_index_comparator());
            }

            return true;
        }

        uint64_t size() const
        {
            return m_enumerator.size();
        }

        uint64_t docid() const
        {
            return m_docid;
        }

        entry_type const& value() const
        {
            return m_cur;
        }

    protected:
        void PS_ALWAYSINLINE extract_all_in(uint64_t a, uint64_t b)
        {
            m_enumerator.move(a);

            for (size_t i = a; i <= b; ++i, m_enumerator.next())
            {
                uint64_t docid = m_enumerator.docid();
                uint64_t score = (*m_docs)[docid].score;

                m_q.emplace_back(docid, score, i, i, i);
            }

            std::sort(m_q.begin(), m_q.end(), [](const queue_element_type& doca, const queue_element_type& docb) {
                return std::get<1>(doca) < std::get<1>(docb);
            });

            m_sorted = true;
        }

        void PS_ALWAYSINLINE extract_minimum_in(uint64_t a, uint64_t b)
        {
            uint64_t pos = m_cartesian_tree->rmq(m_topk_start_offset + a, m_topk_start_offset + b) - m_topk_start_offset;

            m_enumerator.move(pos);
            uint64_t docid = m_enumerator.docid();
            uint64_t score = (*m_docs)[docid].score;

            m_q.emplace_back(docid, score, pos, a, b);
            std::push_heap(m_q.begin(), m_q.end(), value_index_comparator());
        }

        uint64_t m_docid;
        typename Sequence::enumerator m_enumerator;
        uint64_t m_threshold;

        const succinct::cartesian_tree* m_cartesian_tree;
        const std::vector<node_info>* m_docs;
        uint64_t m_topk_start_offset;

        std::vector<queue_element_type> m_q;
        entry_type m_cur;
        bool m_sorted;
    };

protected:
    std::vector<node_info> m_docs;
    std::vector<uint64_t> m_ranking; // Will be used just during construction
    sequences::sequence_file<Sequence> m_seq_postings;
    sequences::sequence_file<sequences::ef_seq> m_seq_offsets;
    sequences::sequence_file<sequences::ef_seq> m_seq_degrees;
    sequences::sequence_file<sequences::ef_seq> m_seq_rankings;

    std::string m_simple_index_fname;
    topk::rmq_sequences m_rmq_sequences;
};

}
}
