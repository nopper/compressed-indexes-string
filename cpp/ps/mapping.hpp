#pragma once

#include <unordered_map>
#include <fstream>
#include <boost/utility.hpp>
#include <boost/program_options.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/cat.hpp>

namespace ps {
namespace mapping {
namespace details {

template <bool, typename K, typename V, typename M>
struct mapper_impl {
    static void parse(M& map, std::string& line, int lineno)
    {}
};

template <>
struct mapper_impl<true, int, int, std::unordered_map<int,int>> {
    static void parse(std::unordered_map<int,int>& map, std::string& line, int lineno)
    {
        int sort_id = lineno;
        int real_id = atoi(line.c_str());
        map.insert(std::pair<int, int>(sort_id, real_id));
    }
};

template <>
struct mapper_impl<false, int, int, std::unordered_map<int,int>> {
    static void parse(std::unordered_map<int,int>& map, std::string& line, int lineno)
    {
        int sort_id = lineno;
        int real_id = atoi(line.c_str());
        map.insert(std::pair<int, int>(real_id, sort_id));
    }
};

template <>
struct mapper_impl<false, int, std::string, std::unordered_map<int,std::string>> {
    static void parse(std::unordered_map<int,std::string>& map, std::string& line, int lineno)
    {
        int real_id = atoi(line.c_str());
        size_t found = line.find('\t');
        map.insert(std::pair<int,std::string>(real_id, line.substr(found + 1)));
    }
};

}

template<bool Reverse = false, typename K = int, typename V = int, typename M = std::unordered_map<K,V>>
class mapper : boost::noncopyable {
public:
    typedef K key_type;
    typedef V value_type;

    mapper(const char* filename)
    {
        std::ifstream file(filename, std::ios_base::in | std::ios_base::binary);
        boost::iostreams::filtering_streambuf<boost::iostreams::input> inbuf;
        inbuf.push(boost::iostreams::gzip_decompressor());
        inbuf.push(file);

        std::istream instream(&inbuf);
        std::string line;

        for (int lineno = 0; std::getline(instream, line); lineno++)
            ps::mapping::details::mapper_impl<Reverse, K, V, M>::parse(m_map, line, lineno);

        file.close();
    }

    bool get(const K& key, V& value)
    {
        auto it = m_map.find(key);

        if (it != m_map.end())
        {
            value = (*it).second;
            return true;
        }

        return false;
    }
private:
    M m_map;
};

typedef mapper<false,int,int,std::unordered_map<int,int>> UserIdToSortId;
typedef mapper<true,int,int,std::unordered_map<int,int>> SortIdToUserId;
typedef mapper<false,int,std::string,std::unordered_map<int,std::string>> UserIdToString;

#define PS_INT_MAPPER_TYPES (UserIdToSortId)(SortIdToUserId)
#define PS_MAPPER_TYPES (UserIdToSortId)(SortIdToUserId)(UserIdToString)

}
}