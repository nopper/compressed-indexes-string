#ifndef UTILS_HPP
#define UTILS_HPP

#include <iostream>
#include <fstream>
#include <locale>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unordered_map>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#define PS_LIKELY(x) __builtin_expect(!!(x), 1)
#define PS_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define PS_NOINLINE __attribute__((noinline))
#define PS_ALWAYSINLINE __attribute__((always_inline))

#ifndef NDEBUG
#define PS_ASSERT(x) assert(x)
#else
#define PS_ASSERT(x) ((void)(x))
#endif

namespace ps {
    inline std::ostream& logger()
    {
        time_t t = std::time(nullptr);
        std::locale loc;
        const std::time_put<char>& tp =
            std::use_facet<std::time_put<char>>(loc);
        const char *fmt = "%F %T";
        tp.put(std::cerr, std::cerr, ' ',
               std::localtime(&t), fmt, fmt + strlen(fmt));
        return std::cerr << ": ";
    }

    struct progress_logger {
        progress_logger(const char * message = "items")
            : items(0)
            , m_message(message)
            , m_tick(boost::posix_time::microsec_clock::universal_time())
        {}

        void log()
        {
            logger() << "Processed " << items << " " << m_message << std::endl;
        }

        void done_item()
        {
            items += 1;
            if (items % 1000000 == 0)
                log();
        }

        void done()
        {
            log();
            m_time_taken = boost::posix_time::microsec_clock::universal_time() - m_tick;
            logger() << "Finished in " << boost::posix_time::to_simple_string(m_time_taken) << std::endl;
        }

        boost::posix_time::time_duration elapsed_time()
        {
            return m_time_taken;
        }

        size_t items;
        std::string m_message;
        boost::posix_time::ptime m_tick;
        boost::posix_time::time_duration m_time_taken;
    };

namespace util {

void successor_of(std::string& key)
{
    int n = key.size();
    while (--n >= 0)
    {
        const uint8_t byte = key[n];
        if (byte != static_cast<uint8_t>(0xff))
        {
            key[n] = byte + 1;
            key.resize(n + 1);
            return;
        }
    }
}

static inline void PS_ALWAYSINLINE split_tab(const std::string& line, int& src, int& dst)
{
    auto begin = line.begin();
    auto end = line.end();

    using boost::spirit::qi::int_;
    using boost::spirit::qi::eol;
    using boost::spirit::qi::phrase_parse;
    using boost::spirit::qi::_1;
    using boost::phoenix::ref;

    bool matched = phrase_parse(
        begin,
        end,
        int_[ref(src) = _1] >> '\t' >> int_[ref(dst) = _1] ,
        eol
    );

    PS_ASSERT(matched);
    PS_ASSERT(begin == end);
}

bool read_vector_from(const std::string& filename, std::vector<uint64_t>& vec)
{
    std::ifstream file(filename.c_str(), std::ios_base::in | std::ios_base::binary);

    if (!file)
        return false;

    boost::iostreams::filtering_istream in;
    in.push(boost::iostreams::gzip_decompressor());
    in.push(file);

    for(std::string str; std::getline(in, str); )
        vec.push_back(::atoi(str.c_str()));

    file.close();
    return true;
}

template <class Sequence>
bool read_ranking_from(const std::string& filename, Sequence& vec)
{
    std::ifstream file(filename.c_str(), std::ios_base::in | std::ios_base::binary);

    if (!file)
        return false;

    boost::iostreams::filtering_istream in;
    in.push(boost::iostreams::gzip_decompressor());
    in.push(file);

    for(std::string str; std::getline(in, str); )
    {
        int src, dst;
        split_tab(str, src, dst);
        vec[src] = dst;
    }

    file.close();
    return true;
}

}

}

#endif /* end of include guard */
