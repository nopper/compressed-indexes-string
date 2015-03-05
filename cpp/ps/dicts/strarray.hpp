#ifndef STRARRAY_HPP
#define STRARRAY_HPP

#include <vector>
#include <fstream>
#include <boost/algorithm/string/predicate.hpp>
#include "ps/utils.hpp"

namespace ps {
namespace dicts {

class strarray {
public:
    strarray(const char* filename)
    {
        std::fstream input(filename);

        if (input.is_open())
        {
            std::string line;
            while (getline(input, line))
                m_strings.push_back(line);
            input.close();
        }
    }

    std::pair<int,int> prefix_search(const std::string& prefix) const
    {
        std::string prefix_end = prefix;
        ps::util::successor_of(prefix_end);

        auto beg = m_strings.begin(); auto end = m_strings.end();
        auto left = std::lower_bound(beg, end, prefix);

        if (left == m_strings.end())
            return std::pair<int, int>(-1, -1);

        auto right = std::lower_bound(beg, end, prefix_end);

        if (right >= left &&
            boost::algorithm::starts_with(*left, prefix))
        {
            return std::pair<int,int>(std::distance(beg, left), std::distance(beg, right));
        }

        return std::pair<int,int>(-1, -1);
    }

    int64_t rank(const std::string& str)
    {
        auto it = std::lower_bound(m_strings.begin(), m_strings.end(), str);

        if (it != m_strings.end() && !(str < *it))
            return it - m_strings.begin();

        return -1;
    }

    void select(const int64_t i, std::string& ret)
    {
        ret = m_strings[i];
    }

    static void build(std::vector<std::string>& keys, const char *filename)
    {
        std::ofstream output(filename);

        for (std::string &s: keys)
            output << s << std::endl;

        output.close();

        std::vector<std::string>().swap(keys);
    }

protected:
    std::vector<std::string> m_strings;
};

}
}

#endif
