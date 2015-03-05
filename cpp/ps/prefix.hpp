#pragma once
#include <random>
#include <unordered_set>

namespace ps {
namespace prefix {

template<class ForwardIterator>
void create_prefix_cdf(size_t prefix_length,
                       std::vector<std::string>& prefixes, std::vector<double>& cdf,
                       ForwardIterator first, ForwardIterator last)
{
    std::string prev_prefix;

    double prev_count = 0.0;
    double normalizer = 1.0 * std::distance(first, last);

    for (; first != last; ++first)
    {
        std::string prefix = (*first).substr(0, prefix_length);

        if (prefix != prev_prefix && prev_count > 0)
        {
            prefixes.push_back(prev_prefix);
            cdf.push_back(prev_count);
            prev_count = 0.0;
        }

        prev_prefix = prefix;
        prev_count += 1.0;
    }

    prefixes.push_back(prev_prefix);
    cdf.push_back(prev_count);

    // Transform to PDF than CDF
    prev_count = 0.0;
    for (auto& prob: cdf)
    {
        prob = prob / normalizer + prev_count;
        prev_count = prob;
    }
}

void sample_prefixes(size_t k,
                     std::vector<size_t>& results,
                     const std::vector<std::string>& prefixes,
                     const std::vector<double>& cdf,
                     bool unique = true,
                     unsigned seed = 42)
{
    std::mt19937 gen(seed);
    std::uniform_real_distribution<> dis(0, 1);

    if (k > prefixes.size())
        k = prefixes.size();

    results.clear();
    std::unordered_set<size_t> sampled;

    while (results.size() < k)
    {
        double r = dis(gen);

        auto iterator = std::lower_bound(cdf.begin(), cdf.end(), r);
        size_t index = std::distance(cdf.begin(), iterator);

        while (sampled.find(index) != sampled.end())
            index = (index + 1) % cdf.size();

        if (unique)
            sampled.insert(index);

        results.push_back(index);
    }

    std::sort(results.begin(), results.end());
}

}
}
