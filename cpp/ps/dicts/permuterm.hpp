#ifndef PERMUTERM_HPP
#define PERMUTERM_HPP

#include <vector>
#include <fstream>
#include "CompPermIdx.hpp"

namespace ps {
namespace dicts {

class permuterm {
public:
    permuterm(const char* filename)
    {
        std::fstream ifs(filename);
        m_cpi.Read(ifs);
    }

    std::pair<int,int> prefix_search(const std::string& prefix) const
    {
        std::vector<uint8_t> query;
        query.push_back(cpi00::kDelimiter);
        String2Uint8_Ts(prefix, query);
        uint64_t l, r;

        if (m_cpi.BackPermSearch(query, l, r))
            return std::pair<int,int>(l, r + 1);

        return std::pair<int,int>(-1, -1);
    }

    int64_t rank(const std::string& str)
    {
        uint64_t r = m_cpi.Rank(str);
        return (r == UINT64_MAX) ? -1 : r - 1;
    }

    void select(const int64_t i, std::string& ret)
    {
        m_cpi.Select((uint64_t)(i + 1), ret);
    }

    static void build(std::vector<std::string>& keys, const char *filename)
    {
        cpi00::CompPermIdx cpi;
        cpi.Build(keys);
        std::ofstream ofs(filename);
        cpi.Write(ofs);
    }

private:
    static void String2Uint8_Ts(const std::string& str, std::vector<uint8_t>& ret) {
        for (auto itr = str.begin(); itr != str.end(); ++itr)
            ret.push_back(static_cast<uint8_t>(*itr));
    }

protected:
    cpi00::CompPermIdx m_cpi;
};

}
}

#endif
