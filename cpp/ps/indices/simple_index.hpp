#pragma once

#include <string>
#include <vector>
#include <iostream>
#include "ps/coding.hpp"
#include "ps/files.hpp"
#include "ps/sequences/sequence_types.hpp"
#include "ps/sequences/sequence_file.hpp"
#include "ps/indices/node.hpp"

namespace ps {
namespace indices {

template <typename Sequence>
class simple_index
{
public:
    typedef Sequence sequence_type;

    simple_index(const char* filename)
        : m_seq_postings(filename, ".pos")
        , m_seq_offsets(filename, ".off")
        , m_seq_degrees(filename, ".deg")
    {
        auto en_offs = m_seq_offsets.sequence_at(0);
        auto en_deg = m_seq_degrees.sequence_at(0);

        m_docs.resize(en_offs.size());

        for (size_t i = 0; i < en_offs.size(); ++i, en_offs.next(), en_deg.next())
            m_docs[i] = node_info(i, en_offs.docid(), en_deg.docid() - 1 - i, 0);
    }

    simple_index(sequences::options opts, const char* filename)
        : m_seq_postings(opts, filename, ".pos")
        , m_seq_offsets(opts, filename, ".off")
        , m_seq_degrees(opts, filename, ".deg")
    {}

    class builder {
    public:
        builder(simple_index& file)
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

            typename sequences::sequence_file<sequences::ef_seq>::builder offsets_builder(
                sequences::options(m_last_offset + 1), m_file.m_seq_offsets);

            typename sequences::sequence_file<sequences::ef_seq>::builder degrees_builder(
                sequences::options(m_cdf_degree + 1), m_file.m_seq_degrees);

            offsets_builder.append(m_offsets.size(), m_offsets.begin());
            degrees_builder.append(m_cdf_degrees.size(), m_cdf_degrees.begin());

            m_seq_postings_builder.commit();
            offsets_builder.commit();
            degrees_builder.commit();

            m_file.m_docs.resize(universe);

            for (uint64_t i = 0; i < m_file.m_docs.size(); ++i)
                m_file.m_docs[i] = node_info(i, m_offsets[i], m_cdf_degrees[i] - 1, 0);

            m_offsets.clear();
            m_cdf_degrees.clear();
        }

        void append(uint64_t docid, std::string& str, uint64_t num_elements = 1)
        {
            uint64_t offset = m_seq_postings_builder.append(str, num_elements);

            assert(offset > m_last_offset || offset == 0);
            assert(docid >= m_last_docid);

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
        simple_index& m_file;
        std::vector<uint64_t> m_offsets;
        std::vector<uint64_t> m_cdf_degrees;

        uint64_t m_last_docid;
        uint64_t m_last_offset;
        uint64_t m_cdf_degree;

        typename sequences::sequence_file<Sequence>::builder m_seq_postings_builder;
    };

    bool get_offset(uint64_t docid, uint64_t& offset) const
    {
        offset = m_docs[docid].offset;
        return degree(docid) > 0;
    }

    uint64_t degree(uint64_t docid) const
    {
        return m_docs[docid].cdf_degree - ((docid == 0) ? 0 : m_docs[docid - 1].cdf_degree);
    }

    std::string get_raw_sequence(uint64_t docid)
    {
        uint64_t end = docid + 1;
        uint64_t start_offset = m_docs[docid].offset;

        while (end < m_docs.size() && m_docs[end].offset == start_offset)
            end++;

        uint64_t end_offset = (end < m_docs.size()) ? m_docs[end].offset : (m_seq_postings.file_size() - m_seq_postings.footer_size());

        return std::string(m_seq_postings.data_at(m_docs[docid].offset),
                           (size_t)(end_offset - m_docs[docid].offset));
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

    friend std::ostream& operator<<(std::ostream& os, const simple_index& index)
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

protected:
    std::vector<node_info> m_docs;
    sequences::sequence_file<Sequence> m_seq_postings;
    sequences::sequence_file<sequences::ef_seq> m_seq_offsets;
    sequences::sequence_file<sequences::ef_seq> m_seq_degrees;
};

}
}
