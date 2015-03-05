#pragma once

#include "ps/coding.hpp"

namespace ps {
namespace sequences {

class plain_seq {
public:
    plain_seq(const char *data, uint64_t data_size, options opts)
        : m_data(data)
    {}

    template<typename DocsIterator>
    static void serialize(std::string& output,
                          options opts,
                          uint64_t n,
                          DocsIterator docs_begin)
    {
        output.resize((n + 1) * 8);
        char *p = const_cast<char*>(output.data()); // XX: OMFG WHAT ARE YOU DOING TO ME? YY: JUST DIE!

        coding::encode_fixed_64(p, n);
        p += 8;

        for (uint32_t i = 0; i < n; ++i)
        {
            coding::encode_fixed_64(p, *docs_begin++);
            p += 8;
        }
    }

    class enumerator;

    const char* data() const
    {
        return m_data;
    }

    enumerator deserialize_at(size_t offset) const
    {
        return enumerator(m_data + offset);
    }

    class enumerator {
    public:
        enumerator(char const* data)
            : m_n(coding::decode_fixed_64(data))
            , m_position(0)
            , m_data(data + 8)
        {
            reset();
        }

        void reset()
        {
            move(0);
        }

        void QS_ALWAYSINLINE next()
        {
            ++m_position;
            m_cur_docid = coding::decode_fixed_64(m_data + m_position * 8);
        }

        void QS_ALWAYSINLINE next_geq(uint64_t lower_bound)
        {
            assert(lower_bound >= m_cur_docid);

            while (docid() < lower_bound)
                next();
        }

        void QS_ALWAYSINLINE move(uint64_t pos)
        {
            m_position = pos;
            m_cur_docid = coding::decode_fixed_64(m_data + m_position * 8);
        }

        uint64_t docid() const
        {
            return m_cur_docid;
        }

        uint64_t position() const
        {
            return m_position;
        }

        uint64_t size() const
        {
            return m_n;
        }

    private:
        uint32_t m_n;
        uint32_t m_position;
        char const* m_data;

        uint32_t m_cur_docid;
    };

private:
    char const* m_data;
};

}
}
