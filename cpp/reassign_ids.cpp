#include <iostream>
#include <vector>
#include <string>
#include "ps/optargs.hpp"
#include "ps/graphs/graph_types.hpp"
#include "ps/utils.hpp"
#include "ps/mapping.hpp"
#include "succinct/util.hpp"

using namespace ps::graphs;
using namespace ps::mapping;

template<typename Translator>
void reassign_ids(const char *filename, bool quiet, std::vector<unsigned int>& columns)
{
    Translator translator(filename);

    std::string line;

    int lineno = 0;
    int num_errors = 0;

    while (std::getline(std::cin, line))
    {
        std::istringstream linestream(line);
        std::ostringstream output;
        std::string token;
        unsigned int column = 0;
        unsigned int curr_column = 0;
        bool is_ok = true;

        while (std::getline(linestream, token, '\t') && is_ok)
        {
            if (column > 0)
                output << '\t';

            if (curr_column < columns.size() && columns[curr_column] == column)
            {
                typename Translator::value_type dst{};

                try {
                    int src = std::stoi(token);
                    is_ok = translator.get(src, dst);
                } catch (...) {
                    is_ok = false;
                }

                if (!is_ok && !quiet)
                {
                    num_errors++;
                    std::cerr << "Error: unable to translate value " << token
                              << " at line " << lineno << " column " << column << std::endl;
                }

                output << dst;
                curr_column++;
            }
            else
                output << token;

            column++;
        }

        if (is_ok)
            std::cout << output.str() << std::endl;

        lineno++;
    }

    if (num_errors > 0)
        std::cerr << "Total errors: " << num_errors << std::endl;
}

int main(int argc, char *argv[])
{
    PARSE_ARGUMENTS(
        ("help", "produce help message")
        ("translation", po::value<std::string>()->required(), "UserIdToSortId | SortIdToUserId")
        ("input-ids,i", po::value<std::string>()->required(), "sorted IDs")
        ("quiet,q", po::value<bool>()->default_value(false), "Suppress error messages")
        ("columns,c", po::value<std::string>()->default_value("0,1"), "Columns to translate")
    )

    bool quiet = vm["quiet"].as<bool>();
    std::string translation = vm["translation"].as<std::string>();

    std::string colstr;
    std::vector<unsigned int> columns;
    std::istringstream cols(vm["columns"].as<std::string>());
    while (std::getline(cols, colstr, ','))
    {
        columns.push_back(std::stoi(colstr));
    }
    std::sort(columns.begin(), columns.end());

    if (false) {
#define LOOP_BODY(R, DATA, T)                          \
    } else if (translation == BOOST_PP_STRINGIZE(T)) { \
        reassign_ids<T>(                               \
            vm["input-ids"].as<std::string>().c_str(), \
            quiet,                                     \
            columns                                    \
        );

        BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_MAPPER_TYPES);
#undef LOOP_BODY
    } else {
        std::cerr << "ERROR: Unknown translation " << translation << std::endl;
        return -1;
    }

    return 0;
}
