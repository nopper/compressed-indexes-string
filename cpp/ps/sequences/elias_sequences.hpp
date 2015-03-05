#pragma once

#include "bitvector_collection.hpp"
#include "integer_codes.hpp"

namespace ps {
namespace sequences {

template <typename DocsSequence>
class elias_seq {
public:
    elias_seq(const char *data, uint64_t data_size, options opts)
        : m_data(data)
        , m_bitvectors(reinterpret_cast<const uint64_t *>(data), data_size * 8)
        , m_universe(opts.universe)
        , m_params(opts.params)
    {}

    template<typename DocsIterator>
    static void serialize(std::string& output,
                          options opts,
                          uint64_t n,
                          DocsIterator docs_begin)
    {
        succinct::bit_vector_builder bvb;
        quasi_succinct::write_gamma_nonzero(bvb, n);
        DocsSequence::write(bvb, docs_begin, opts.universe, n, opts.params);

        succinct::bit_vector bv(&bvb);
        succinct::mapper::mappable_vector<uint64_t> const& vec = bv.data();
        size_t n_bytes = static_cast<size_t>(vec.size() * sizeof(uint64_t));
        output.assign(reinterpret_cast<const char*>(vec.data()), n_bytes);
    }

    class enumerator;

    const char* data() const
    {
        return m_data;
    }

    enumerator deserialize_at(size_t offset) const
    {
        auto docs_it = succinct::bit_vector::enumerator(m_bitvectors, offset * 8);
        uint64_t n = quasi_succinct::read_gamma_nonzero(docs_it);
        typename DocsSequence::enumerator docs_enum(m_bitvectors,
                                                    docs_it.position(),
                                                    m_universe, n,
                                                    m_params);

        return enumerator(docs_enum);
    }

    class enumerator {
    public:
        void reset()
        {
            m_cur_pos = 0;
            m_cur_docid = m_docs_enum.move(0).second;
        }

        void QS_FLATTEN_FUNC next()
        {
            auto val = m_docs_enum.next();
            m_cur_pos = val.first;
            m_cur_docid = val.second;
        }

        void QS_FLATTEN_FUNC next_geq(uint64_t lower_bound)
        {
            auto val = m_docs_enum.next_geq(lower_bound);
            m_cur_pos = val.first;
            m_cur_docid = val.second;
        }

        void QS_FLATTEN_FUNC move(uint64_t position)
        {
            auto val = m_docs_enum.move(position);
            m_cur_pos = val.first;
            m_cur_docid = val.second;
        }

        uint64_t docid() const
        {
            return m_cur_docid;
        }

        uint64_t position() const
        {
            return m_cur_pos;
        }

        uint64_t size() const
        {
            return m_docs_enum.size();
        }

    private:
        friend class elias_seq;

        enumerator(typename DocsSequence::enumerator docs_enum)
            : m_docs_enum(docs_enum)
        {
            reset();
        }

        uint64_t m_cur_pos;
        uint64_t m_cur_docid;
        typename DocsSequence::enumerator m_docs_enum;
    };

private:
    const char* m_data;
    succinct::bit_vector m_bitvectors;
    uint64_t m_universe;
    quasi_succinct::global_parameters m_params;
};

}
}