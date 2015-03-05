#pragma once

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/cat.hpp>

#include "ps/sequences/block_sequences.hpp"
#include "ps/sequences/elias_sequences.hpp"
#include "ps/sequences/plain_sequences.hpp"

#include "compact_elias_fano.hpp"
#include "partitioned_sequence.hpp"
#include "uniform_partitioned_sequence.hpp"
#include "compact_elias_fano.hpp"

namespace ps {
namespace sequences {

typedef elias_seq<quasi_succinct::compact_elias_fano> ef_seq;
typedef elias_seq<quasi_succinct::indexed_sequence> single_seq;
typedef elias_seq<quasi_succinct::uniform_partitioned_sequence<>> uniform_seq;
typedef elias_seq<quasi_succinct::partitioned_sequence<>> opt_seq;

typedef block_seq<quasi_succinct::optpfor_block> block_optpfor_seq;
typedef block_seq<quasi_succinct::varint_G8IU_block> block_varint_seq;
typedef block_seq<quasi_succinct::interpolative_block> block_interpolative_seq;

typedef plain_seq fixed_seq;

}
}

#define PS_SEQ_TYPES (ef)(single)(uniform)(opt)(block_optpfor)(block_varint)(block_interpolative)(fixed)