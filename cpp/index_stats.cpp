#include "ps/optargs.hpp"
#include "ps/utils.hpp"
#include "ps/indices/index_types.hpp"

using namespace ps;
using namespace ps::indices;

int main(int argc, char *argv[])
{
    PARSE_ARGUMENTS(
        ("help", "produce help message")
        ("index-type,t", po::value<string>()->required(), "Type of index to use")
        ("input-index,i", po::value<std::string>()->required(), "input index")
    )

    std::string index_type = vm["index-type"].as<std::string>();
    std::string input_idx = vm["input-index"].as<std::string>();

    if (false) {
#define LOOP_BODY(R, DATA, T)                                   \
    } else if (index_type == BOOST_PP_STRINGIZE(T)) {              \
        BOOST_PP_CAT(T, _index) index(input_idx.c_str()); \
        std::cout << index << std::endl;

        BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_INDEX_TYPES);
#undef LOOP_BODY
    } else {
        cerr << "ERROR: Unknown index_type " << index_type << endl;
        return -1;
    }

    return 0;
}
