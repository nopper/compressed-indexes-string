#pragma once
#include "global_parameters.hpp"

namespace ps {
namespace sequences {

struct options {
    options(uint64_t _universe)
        : universe(_universe)
    {}

    options(uint64_t _universe, quasi_succinct::global_parameters _params)
        : universe(_universe)
        , params(_params)
    {}

    uint64_t universe;
    quasi_succinct::global_parameters params;
};

}
}