#pragma once

#include "block_codecs.hpp"
#include "options.hpp"
#include "util.hpp" // PEF
#include "succinct/util.hpp" // PEF/Succinct

namespace ps {
namespace sequences {

template <typename DocsSequence, typename DocsIterator>
static void write(std::vector<uint8_t>& out, uint32_t n,
                  DocsIterator docs_begin) {
    quasi_succinct::TightVariableByte::encode_single(n, out);

    uint64_t block_size = DocsSequence::block_size;
    uint64_t blocks = succinct::util::ceil_div(n, block_size);
    size_t begin_block_maxs = out.size();
    size_t begin_block_endpoints = begin_block_maxs + 4 * blocks;
    size_t begin_blocks = begin_block_endpoints + 4 * (blocks - 1);
    out.resize(begin_blocks);

    DocsIterator docs_it(docs_begin);
    std::vector<uint32_t> docs_buf(block_size);
    uint32_t last_doc(-1);
    uint32_t block_base = 0;
    for (size_t b = 0; b < blocks; ++b) {
        uint32_t cur_block_size =
            ((b + 1) * block_size <= n)
            ? block_size : (n % block_size);

        for (size_t i = 0; i < cur_block_size; ++i) {
            uint32_t doc(*docs_it++);
            docs_buf[i] = doc - last_doc - 1;
            last_doc = doc;
        }
        *((uint32_t*)&out[begin_block_maxs + 4 * b]) = last_doc;

        DocsSequence::encode(docs_buf.data(), last_doc - block_base - (cur_block_size - 1),
                           cur_block_size, out);
        if (b != blocks - 1) {
            *((uint32_t*)&out[begin_block_endpoints + 4 * b]) = out.size() - begin_blocks;
        }
        block_base = last_doc + 1;
    }
}

template <typename DocsSequence>
class block_seq {
public:
    block_seq(const char *data, uint64_t data_size, options opts)
        : m_data(reinterpret_cast<const uint8_t *>(data))
        , m_data_size(data_size)
        , m_universe(opts.universe)
        , m_params(opts.params)
    {}

    template<typename DocsIterator>
    static void serialize(std::string& output,
                          options opts,
                          uint64_t n,
                          DocsIterator docs_begin)
    {
        std::vector<uint8_t> vec;
        write<DocsSequence>(vec, n, docs_begin);
        output.assign(reinterpret_cast<const char*>(vec.data()), vec.size());
    }

    class enumerator;

    const char* data() const
    {
        return reinterpret_cast<const char *>(m_data);
    }

    enumerator deserialize_at(size_t offset) const
    {
        return enumerator(m_data + offset, m_universe);
    }

    class enumerator {
    public:
        enumerator(uint8_t const* data, uint64_t universe)
            : m_n(0) // just to silence warnings
            , m_base(quasi_succinct::TightVariableByte::decode(data, &m_n, 1))
            , m_blocks(succinct::util::ceil_div(m_n, DocsSequence::block_size))
            , m_block_maxs(m_base)
            , m_block_endpoints(m_block_maxs + 4 * m_blocks)
            , m_blocks_data(m_block_endpoints + 4 * (m_blocks - 1))
            , m_universe(universe)
        {
            m_docs_buf.resize(DocsSequence::block_size);
            reset();
        }

        void reset()
        {
            decode_docs_block(0);
        }

        void QS_ALWAYSINLINE next()
        {
            ++m_pos_in_block;
            if (QS_UNLIKELY(m_pos_in_block == m_cur_block_size)) {
                if (m_cur_block + 1 == m_blocks) {
                    m_cur_docid = m_universe;
                    m_cur_block_size = size() % DocsSequence::block_size;
                    m_pos_in_block = m_cur_block_size;
                    m_cur_block = m_blocks - 1;
                    return;
                }
                decode_docs_block(m_cur_block + 1);
            } else {
                m_cur_docid += m_docs_buf[m_pos_in_block] + 1;
            }
        }

        void QS_ALWAYSINLINE next_geq(uint64_t lower_bound)
        {
            assert(lower_bound >= m_cur_docid);
            if (QS_UNLIKELY(lower_bound > m_cur_block_max)) {
                // binary search seems to perform worse here
                if (lower_bound > block_max(m_blocks - 1)) {
                    m_cur_docid = m_universe;
                    m_cur_block_size = size() % DocsSequence::block_size;
                    m_pos_in_block = m_cur_block_size;
                    m_cur_block = m_blocks - 1;
                    return;
                }

                uint64_t block = m_cur_block + 1;
                while (block_max(block) < lower_bound) {
                    ++block;
                }

                decode_docs_block(block);
            }

            while (docid() < lower_bound) {
                m_cur_docid += m_docs_buf[++m_pos_in_block] + 1;
                assert(m_pos_in_block < m_cur_block_size);
            }
        }

        void QS_ALWAYSINLINE move(uint64_t pos)
        {
            uint64_t block = pos / DocsSequence::block_size;
            if (QS_UNLIKELY(block != m_cur_block)) {
                decode_docs_block(block);
            }
            if (QS_UNLIKELY(position() > pos)) {
                m_pos_in_block = 0;
                m_cur_docid = m_docs_buf[0];
            }
            while (position() < pos) {
                m_cur_docid += m_docs_buf[++m_pos_in_block] + 1;
            }
        }

        uint64_t docid() const
        {
            return m_cur_docid;
        }

        uint64_t position() const
        {
            return m_cur_block * DocsSequence::block_size + m_pos_in_block;
        }

        uint64_t size() const
        {
            return m_n;
        }

    private:
        uint32_t block_max(uint32_t block) const
        {
            return ((uint32_t const*)m_block_maxs)[block];
        }

        void QS_NOINLINE decode_docs_block(uint64_t block)
        {
            static const uint64_t block_size = DocsSequence::block_size;
            m_cur_block_size =
                ((block + 1) * block_size <= size())
                ? block_size : (size() % block_size);
            uint32_t cur_base = (block ? block_max(block - 1) : uint32_t(-1)) + 1;
            m_cur_block_max = block_max(block);

            uint32_t endpoint = block
                ? ((uint32_t const*)m_block_endpoints)[block - 1]
                : 0;
            uint8_t const* block_data = m_blocks_data + endpoint;
            DocsSequence::decode(block_data, m_docs_buf.data(),
                               m_cur_block_max - cur_base - (m_cur_block_size - 1),
                               m_cur_block_size);

            m_docs_buf[0] += cur_base;

            m_cur_block = block;
            m_pos_in_block = 0;
            m_cur_docid = m_docs_buf[0];
        }

        uint32_t m_n;
        uint8_t const* m_base;
        uint32_t m_blocks;
        uint8_t const* m_block_maxs;
        uint8_t const* m_block_endpoints;
        uint8_t const* m_blocks_data;
        uint64_t m_universe;

        uint32_t m_cur_block;
        uint32_t m_pos_in_block;
        uint32_t m_cur_block_max;
        uint32_t m_cur_block_size;
        uint32_t m_cur_docid;

        std::vector<uint32_t> m_docs_buf;
    };

private:
    const uint8_t* m_data;
    uint64_t m_data_size;
    uint64_t m_universe;
    quasi_succinct::global_parameters m_params;
};

}
}
