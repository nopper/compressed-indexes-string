#pragma once

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/cat.hpp>

#include "ps/dicts/permuterm.hpp"
#include "ps/dicts/strarray.hpp"

namespace ps {
namespace dicts {
    typedef permuterm permuterm_dict;
    typedef strarray strarray_dict;
}
}

#define PS_DICT_TYPES (permuterm)(strarray)
