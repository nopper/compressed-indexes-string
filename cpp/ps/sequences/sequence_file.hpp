#pragma once

#include <string>
#include <vector>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "ps/sequences/options.hpp"
#include "ps/coding.hpp"
#include "ps/files.hpp"

namespace ps {
namespace sequences {

template <typename Sequence>
class sequence_file
{
public:
    sequence_file(const char* filename,
                  const char* suffix = "")
        : m_filename(filename)
        , m_opts(0)
        , m_file_size(0)
    {
        m_filename.append(suffix);
        initialize();
    }

    sequence_file(options opts,
                  const char* filename,
                  const char* suffix = "")
        : m_filename(filename)
        , m_opts(opts)
        , m_file_size(0)
    {
        m_filename.append(suffix);
    }

    class builder {
    public:
        builder(sequence_file& file)
            : m_seq_file(file)
            , m_num_sequences(0)
            , m_num_elements(0)
            , m_tick(boost::posix_time::microsec_clock::universal_time())
        {
            if (!new_writable_file(m_seq_file.m_filename, &m_seq_file_writable))
                throw std::runtime_error("Unable to create output sequence file");
        }

        builder(options opts, sequence_file& file)
            : m_seq_file(file)
            , m_num_sequences(0)
            , m_num_elements(0)
            , m_tick(boost::posix_time::microsec_clock::universal_time())
        {
            m_seq_file.m_opts = opts;

            if (!new_writable_file(m_seq_file.m_filename, &m_seq_file_writable))
                throw std::runtime_error("Unable to create output sequence file");
        }

        void commit(uint64_t num_elements = 0)
        {
            uint64_t elapsed_microsec = (
                boost::posix_time::microsec_clock::universal_time() - m_tick
            ).total_microseconds();

            std::string footer;
            footer.reserve(m_seq_file.footer_size());
            footer.append(1, static_cast<char>(m_seq_file.m_opts.params.ef_log_sampling0));
            footer.append(1, static_cast<char>(m_seq_file.m_opts.params.ef_log_sampling1));
            footer.append(1, static_cast<char>(m_seq_file.m_opts.params.rb_log_rank1_sampling));
            footer.append(1, static_cast<char>(m_seq_file.m_opts.params.rb_log_sampling1));
            footer.append(1, static_cast<char>(m_seq_file.m_opts.params.log_partition_size));
            coding::put_fixed_64(footer, m_seq_file.m_opts.universe);

            coding::put_fixed_64(footer, m_num_sequences);
            coding::put_fixed_64(footer, m_num_elements + num_elements);
            coding::put_fixed_64(footer, elapsed_microsec);

            m_seq_file_writable->append(footer);
            m_seq_file_writable->close();
            m_seq_file.initialize();
        }

        uint64_t append(const std::string& encoded_sequence, uint64_t num_elements = 1)
        {
            uint64_t offset = m_seq_file_writable->get_file_size();

            m_seq_file_writable->append(encoded_sequence);
            m_num_sequences++;
            m_num_elements += num_elements;

            return offset;
        }

        template<typename DocsIterator>
        uint64_t append(uint64_t n, DocsIterator docs_begin)
        {
            std::string encoded;
            Sequence::serialize(encoded, m_seq_file.m_opts, n, docs_begin);
            return append(encoded, n);
        }

        sequence_file& m_seq_file;
        uint64_t m_num_sequences;
        uint64_t m_num_elements;
        boost::posix_time::ptime m_tick;
        std::unique_ptr<files::writable_file> m_seq_file_writable;
    };

    const char* data_at(uint64_t offset) const
    {
        return m_sequence->data() + offset;
    }

    typename Sequence::enumerator sequence_at(uint64_t offset) const
    {
        return m_sequence->deserialize_at(offset);
    }

    uint64_t num_sequences() const
    {
        return m_num_sequences;
    }

    uint64_t num_elements() const
    {
        return m_num_elements;
    }

    uint64_t construction_time_microsec() const
    {
        return m_construction_time_microsec;
    }

    options opts() const
    {
        return m_opts;
    }

    uint64_t contents_size() const
    {
        return m_file_size - footer_size();
    }

    uint64_t file_size() const
    {
        return m_file_size;
    }

    uint64_t footer_size() const
    {
        return sizeof(uint8_t) * 5 + sizeof(uint64_t) * 4;
    }

    std::string filename() const
    {
        return m_filename;
    }

private:
    void initialize()
    {
        m_mmapped_file.open(m_filename.c_str());

        if (!m_mmapped_file.is_open())
            throw std::runtime_error("Error opening sequence file");

        const char *data = m_mmapped_file.data();
        m_file_size = m_mmapped_file.size() / sizeof(data[0]);

        // We read the footer first
        const char *p = data;
        p += m_file_size - footer_size();

        m_opts.params.ef_log_sampling0 = static_cast<uint8_t>(*p++);
        m_opts.params.ef_log_sampling1 = static_cast<uint8_t>(*p++);
        m_opts.params.rb_log_rank1_sampling = static_cast<uint8_t>(*p++);
        m_opts.params.rb_log_sampling1 = static_cast<uint8_t>(*p++);
        m_opts.params.log_partition_size = static_cast<uint8_t>(*p++);
        m_opts.universe = coding::decode_fixed_64(p);

        m_num_sequences = coding::decode_fixed_64(p + 8);
        m_num_elements = coding::decode_fixed_64(p + 8 + 8);
        m_construction_time_microsec = coding::decode_fixed_64(p + 8 + 8 + 8);

        // std::cout << "sequence_file is"
        //     << " params.ef_log_sampling0=" << (int)m_opts.params.ef_log_sampling0
        //     << " params.ef_log_sampling1=" << (int)m_opts.params.ef_log_sampling1
        //     << " params.rb_log_rank1_sampling=" << (int)m_opts.params.rb_log_rank1_sampling
        //     << " params.rb_log_sampling1=" << (int)m_opts.params.rb_log_sampling1
        //     << " params.log_partition_size=" << (int)m_opts.params.log_partition_size
        //     << " universe=" << m_opts.universe
        //     << " num_sequences=" << m_num_sequences
        //     << " m_num_elements=" << m_num_elements
        //     << " m_construction_time_microsec=" << m_construction_time_microsec
        //     << std::endl;

        m_sequence.reset(new Sequence(data, m_file_size, m_opts));
    }

    std::string m_filename;
    struct options m_opts;
    uint64_t m_file_size;

    boost::iostreams::mapped_file_source m_mmapped_file;
    std::unique_ptr<Sequence> m_sequence;

    uint64_t m_num_sequences;
    uint64_t m_num_elements;
    uint64_t m_construction_time_microsec;
};

}
}
