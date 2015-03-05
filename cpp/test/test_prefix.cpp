#define BOOST_TEST_MODULE prefix

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include "ps/prefix.hpp"

// Stolen from http://stackoverflow.com/a/17503436/870914
#define CHECK_CLOSE_COLLECTION(aa, bb, tolerance) {                            \
    auto a = std::begin(aa), ae = std::end(aa);                                \
    auto b = std::begin(bb);                                                   \
    BOOST_REQUIRE_EQUAL(std::distance(a, ae), std::distance(b, std::end(bb))); \
    for(; a != ae; ++a, ++b) {                                                 \
        BOOST_CHECK_CLOSE(*a, *b, tolerance);                                  \
    }                                                                          \
}

BOOST_AUTO_TEST_CASE(create_prefix_cdf)
{
    std::vector<std::string> strings = {
        "ciao",
        "come",
        "stai?",
        "sto",
        "stupendamente",
        "suca!"
    };

    {
        std::vector<std::string> prefixes;
        std::vector<double> cdf;

        ps::prefix::create_prefix_cdf(1, prefixes, cdf, strings.begin(), strings.end());

        std::vector<std::string> prefixes_expected = {"c", "s"};
        std::vector<double> cdf_expected = {2 / 6.0, 1.0};

        BOOST_REQUIRE_EQUAL_COLLECTIONS(prefixes.begin(), prefixes.end(),
                                        prefixes_expected.begin(), prefixes_expected.end());
        CHECK_CLOSE_COLLECTION(cdf, cdf_expected, 0.001);

    }

    {
        std::vector<std::string> prefixes;
        std::vector<double> cdf;

        ps::prefix::create_prefix_cdf(2, prefixes, cdf, strings.begin(), strings.end());

        std::vector<std::string> prefixes_expected = {"ci", "co", "st", "su"};
        std::vector<double> cdf_expected = {1 / 6.0, (1 + 1) / 6.0, (3 + 2) / 6.0, (1 + 5) / 6.0};

        BOOST_REQUIRE_EQUAL_COLLECTIONS(prefixes.begin(), prefixes.end(),
                                        prefixes_expected.begin(), prefixes_expected.end());
        CHECK_CLOSE_COLLECTION(cdf, cdf_expected, 0.001);
    }
}

BOOST_AUTO_TEST_CASE(sample_prefixes_unique)
{
    std::default_random_engine generator;
    std::discrete_distribution<int> distribution {1,1,1,1,1,10,1,1,1,1};

    size_t num_strings = 100;
    std::vector<std::string> strings;

    for (size_t i = 0; i < num_strings; ++i)
    {
        int number = distribution(generator);
        std::string s(1, (char)('0' + number));
        strings.push_back(s);
    }

    std::sort(strings.begin(), strings.end());

    std::vector<std::string> prefixes;
    std::vector<double> cdf;
    std::vector<size_t> results;

    size_t num_queries = 10;

    ps::prefix::create_prefix_cdf(1, prefixes, cdf, strings.begin(), strings.end());

    int success = 0;

    for (int i = 0; i < 100; i++)
    {
        results.clear();
        ps::prefix::sample_prefixes(num_queries, results, prefixes, cdf, false);

        std::unordered_map<int,int> m;
        int max_value = -1;
        int max_key = -1;

        for (auto& r: results)
        {
            m[r] += 1;
            if (m[r] > max_value)
            {
                max_value = m[r];
                max_key = r;
            }
        }

        if (max_key == 5)
            success++;
    }

    std::cout << "Successess " << success << std::endl;
    BOOST_REQUIRE(success >= 80);
}