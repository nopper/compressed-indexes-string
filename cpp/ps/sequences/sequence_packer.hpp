#pragma once

#include "block_codecs.hpp"

namespace ps {
namespace sequences {

class sequence_packer {
public:
    template <typename Sequence, typename DocsIterator>
    void append(options opts,
                uint64_t n,
                DocsIterator docs_begin)
    {
        std::string output;

        if (n > 0)
            Sequence::serialize(output, opts, n, docs_begin);

        append_raw(output);
    }

    void PS_ALWAYSINLINE append_raw(const std::string& output)
    {
        // We encode the length
        uint32_t val = output.size();
        uint8_t buf[5];
        size_t nvalue;
        quasi_succinct::TightVariableByte::encode(&val, 1, buf, nvalue);
        m_encoded.insert(m_encoded.end(), buf, buf + nvalue);
        m_encoded.append(output);
    }

    const std::string& encoded() const
    {
        return m_encoded;
    }

private:
    std::string m_encoded;
};

bool unpack_sequence(int idx, const char* data, uint32_t& displacement)
{
    const uint8_t* p = reinterpret_cast<const uint8_t*>(data);

    for (;;)
    {
        uint32_t length = 0;
        uint32_t length_length = quasi_succinct::TightVariableByte::decode(p, &length, 1) - p;
        displacement += length_length;

        if (idx == 0)
            return length > 0;

        p += length_length + length;
        displacement += length;
        idx--;
    }
}

}
}