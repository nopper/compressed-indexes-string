#pragma once

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <sys/mman.h>

namespace ps {
namespace graphs {

class gz_graph {
public:
    gz_graph(const char * filename)
        : m_filename(filename)
    {
        initialize();
    }

    bool get_next(Edge& edge)
    {
        std::string line_str;

        if (!std::getline(m_in, line_str))
            return false;

        ps::util::split_tab(line_str, edge.first, edge.second);
        return true;
    }

private:
    void initialize()
    {
        m_file.open(m_filename);

        if (!m_file.is_open()) {
            throw std::runtime_error("Error opening file");
        }

        char const* m_data = m_file.data();
        size_t m_data_size = m_file.size() / sizeof(m_data[0]);

        posix_madvise((void*)m_data, m_data_size, POSIX_MADV_SEQUENTIAL);

        boost::iostreams::array_source source(m_data, m_data_size);
        m_in.push(boost::iostreams::gzip_decompressor());
        m_in.push(source);
    }

    const char * m_filename;
    boost::iostreams::mapped_file_source m_file;
    boost::iostreams::filtering_istream m_in;
};

}
}