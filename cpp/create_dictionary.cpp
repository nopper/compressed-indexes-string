#include "ps/optargs.hpp"
#include "ps/dicts/dict_types.hpp"
#include "ps/utils.hpp"
#include "succinct/util.hpp"

using namespace ps;
using namespace ps::dicts;

template <typename DictType>
void build_dictionary(std::string output)
{
    std::string line;
    std::vector<std::string> strings;

    while (succinct::util::fast_getline(line, stdin, true))
        strings.push_back(line);

    DictType::build(strings, output.c_str());
}

int main(int argc, char *argv[])
{
    PARSE_ARGUMENTS(
        ("help", "produce help message")
        ("type", po::value<std::string>()->required(), "Type of dict to create")
        ("output,o", po::value<std::string>()->required(), "Output file where to save the dictionary")
    )

    std::string type = vm["type"].as<std::string>();
    std::string output = vm["output"].as<std::string>();

    if (false) {
#define LOOP_BODY(R, DATA, T)                               \
    } else if (type == BOOST_PP_STRINGIZE(T)) {             \
        build_dictionary<BOOST_PP_CAT(T, _dict)>(output);

        BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_DICT_TYPES);
#undef LOOP_BODY
    } else {
        logger() << "ERROR: Unknown type " << type << std::endl;
    }

    return 0;
}
