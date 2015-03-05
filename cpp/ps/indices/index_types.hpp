#pragma once

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/cat.hpp>

#include "ps/indices/simple_index.hpp"
#include "ps/indices/topk_index.hpp"
#include "ps/sequences/sequence_types.hpp"

// I know this is crazy but please don't cry

namespace ps {
namespace indices {

#define LOOP_BODY(R, DATA, T)                                                                  \
    typedef simple_index<BOOST_PP_CAT(sequences::T, _seq)> BOOST_PP_CAT(T, _simple_index);     \
    typedef topk_index<BOOST_PP_CAT(sequences::T, _seq)> BOOST_PP_CAT(T, _topk_index);

BOOST_PP_SEQ_FOR_EACH(LOOP_BODY, _, PS_SEQ_TYPES);
#undef LOOP_BODY

}
}

#define OP_SIMPLE(S, _, ELEM) BOOST_PP_CAT(ELEM, _simple)
#define PS_SIMPLE_INDEX_TYPES BOOST_PP_SEQ_TRANSFORM(OP_SIMPLE, 0, PS_SEQ_TYPES)

#define OP_TOPK(S, _, ELEM) BOOST_PP_CAT(ELEM, _topk)
#define PS_TOPK_INDEX_TYPES BOOST_PP_SEQ_TRANSFORM(OP_TOPK, 0, PS_SEQ_TYPES)

#define PS_INDEX_TYPES PS_SIMPLE_INDEX_TYPES PS_TOPK_INDEX_TYPES
