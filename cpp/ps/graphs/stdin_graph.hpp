#pragma once

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

namespace ps {
namespace graphs {

class stdin_graph {
public:
    stdin_graph(size_t buffer_size = 65536)
        : m_buffer_size(buffer_size),
          m_buffer(new char[buffer_size]),
          m_begin(0),
          m_end(0)
    {}

    ~stdin_graph()
    {
        delete [] m_buffer;
    }

    bool get_next(Edge& edge)
    {
        edge.first = 0;
        edge.second = 0;

        bool is_stdin_ok = true;
        bool is_filled = false;

        int* p = &edge.first;

        while (is_stdin_ok && !is_filled)
        {
            while (m_begin < m_end)
            {
                char c = m_buffer[m_begin++];

                if (c == '\n')
                {
                    is_filled = true;
                    break;
                }

                if (c == '\t')
                {
                    p = &edge.second;
                    continue;
                }

                (*p) = (c - '0') + 10 * (*p);
            }

            if (is_filled)
                break;

            is_stdin_ok = read_next_block();
        }

        return is_filled;
    }

private:
    bool read_next_block()
    {
        m_begin = 0;
        m_end = fread(m_buffer, 1, m_buffer_size, stdin);

        return m_end > 0;
    }

    size_t m_buffer_size;
    char* m_buffer;

    int m_begin;
    int m_end;
};

}
}