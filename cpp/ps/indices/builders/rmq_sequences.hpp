#pragma once

#include <string>
#include "succinct/mapper.hpp"
#include "ps/indices/simple_index.hpp"
#include "ps/configuration.hpp"
#include "succinct/mapper.hpp"
#include "succinct/topk_vector.hpp"
#include "succinct/elias_fano_compressed_list.hpp"
#include "ps/indices/node.hpp"

namespace ps {
namespace indices {

// Forward declarations
template <typename Sequence> class simple_index;
template <typename Sequence> class coverage_index;
template <typename Sequence> class topk_index;

namespace topk {

class rmq_sequences : boost::noncopyable {
    friend class topk_index;
public:
    rmq_sequences(const char * filename)
        : m_filename(filename)
        , m_seq_docid(filename, "rmq.doc")
        , m_seq_degree(filename, "rmq.deg")
    {
        auto en = m_seq_docid.sequence_at(0);
        m_docid.resize(en.size());

        uint64_t prev = 0;

        for (uint64_t i = 0; i < en.size(); ++i, en.next())
        {
            m_docid[i] = en.docid() - prev - 1;
            prev = en.docid();
        }

        auto en_deg = m_seq_degree.sequence_at(0);
        m_degree.resize(en_deg.size());

        prev = 0;

        for (uint64_t i = 0; i < en_deg.size(); ++i, en_deg.next())
        {
            m_degree[i] = en_deg.docid() - prev - 1;
            prev = en_deg.docid();
        }

        m_filename.append(".rmq");
        m_cartesian_trees_file.open(m_filename.c_str());

        const char *data = m_cartesian_trees_file.data();
        const char *eof = data + m_cartesian_trees_file.size() / sizeof(data[0]);

        while (data < eof) {
            m_cartesian_trees.push_back(new succinct::cartesian_tree());
            size_t bytes_read = succinct::mapper::map(*m_cartesian_trees.back(), data);
            data += bytes_read;
        }
    }

    rmq_sequences(sequences::options opts, const char * filename)
        : m_filename(filename)
        , m_seq_docid(opts, filename, "rmq.doc")
        , m_seq_degree(opts, filename, "rmq.deg")
    {
        m_filename.append(".rmq");
    }

    class builder {
    public:
        builder(rmq_sequences& parent,
                size_t threshold = configuration::get().indices_topk_rmq_sizehint)
            : m_parent(parent)
            , m_threshold(threshold)
        {
            m_parent.m_degree.push_back(0);
        }

        template <typename InputIterator>
        void append(uint64_t docid, uint64_t cdf_degree, uint64_t size, InputIterator begin)
        {
            for (uint64_t i = 0; i < size; ++i, ++begin)
                m_scores.push_back(*begin);

            m_parent.m_docid.push_back(m_parent.m_cartesian_trees.size());

            if (m_scores.size() >= m_threshold)
            {
                m_parent.m_degree.push_back(cdf_degree);
                m_parent.m_cartesian_trees.push_back(
                    new succinct::cartesian_tree(m_scores, std::greater<uint64_t>())
                );
                m_scores.clear();
            }
        }

        void commit()
        {
            if (m_scores.size() > 0)
            {
                m_parent.m_cartesian_trees.push_back(
                    new succinct::cartesian_tree(m_scores, std::greater<uint64_t>())
                );
                m_scores.clear();
            }

            // Write m_docid and m_degree array to file
            std::vector<uint64_t> enc_docid(m_parent.m_docid);
            encode_duplicates(enc_docid);

            typename sequences::sequence_file<sequences::ef_seq>::builder docid_builder(
                sequences::options(enc_docid[enc_docid.size() - 1] + 1), m_parent.m_seq_docid);

            docid_builder.append(enc_docid.size(), enc_docid.begin());
            docid_builder.commit();

            std::vector<uint64_t> enc_degree(m_parent.m_degree);
            encode_duplicates(enc_degree);

            typename sequences::sequence_file<sequences::ef_seq>::builder degree_builder(
                sequences::options(enc_degree[enc_degree.size() - 1] + 1), m_parent.m_seq_degree);

            degree_builder.append(enc_degree.size(), enc_degree.begin());
            degree_builder.commit();

            std::ofstream ct_file;
            ct_file.open(m_parent.m_filename.c_str());

            // XX: we are actually wasting some bytes for each tree since we need to save map_flags
            for (auto& tree: m_parent.m_cartesian_trees)
                succinct::mapper::freeze(*tree, ct_file);

            ct_file.close();
        }

    protected:
        void encode_duplicates(std::vector<uint64_t>& enc)
        {
            uint64_t prev = 0;

            for (uint64_t i = 0; i < enc.size(); ++i)
            {
                enc[i] = enc[i] + prev + 1;
                prev = enc[i];
            }
        }

        rmq_sequences& m_parent;
        std::vector<uint64_t> m_scores;
        size_t m_threshold;
    };

    uint64_t get_index(uint64_t docid) const
    {
        return m_docid[docid];
    }

    uint64_t get_degree(uint64_t index) const
    {
        return m_degree[index];
    }

    const succinct::cartesian_tree* get_cartesian_tree(uint64_t index) const
    {
        return m_cartesian_trees[index];
    }

    const std::vector<uint64_t>& docid() const
    {
        return m_docid;
    }

    const std::vector<uint64_t>& degree() const
    {
        return m_degree;
    }

    const std::vector<succinct::cartesian_tree*>& cartesian_trees() const
    {
        return m_cartesian_trees;
    }

protected:
    std::string m_filename;
    std::vector<uint64_t> m_docid;
    std::vector<uint64_t> m_degree;
    sequences::sequence_file<sequences::ef_seq> m_seq_docid;
    sequences::sequence_file<sequences::ef_seq> m_seq_degree;
    std::vector<succinct::cartesian_tree*> m_cartesian_trees;

    boost::iostreams::mapped_file_source m_cartesian_trees_file;
};

}
}
}
