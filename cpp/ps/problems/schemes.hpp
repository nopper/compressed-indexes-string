#pragma once

namespace ps {
namespace problems {

enum Scheme
{
    Hopping,
    AsIndex,
    Coverage,
    BaselineHopping,
    BaselineAsIndex,
    FastBaselineHopping,
    FastBaselineAsIndex,

    TopkHopping,
    TopkHoppingRMQ,
    TopkHoppingWAND,
    TopkHoppingRMQWAND,
};

struct Schemes {
    static const Scheme hopping = Scheme::Hopping;
    static const Scheme asindex = Scheme::AsIndex;
    static const Scheme coverage = Scheme::Coverage;
    static const Scheme baseline_hopping = Scheme::BaselineHopping;
    static const Scheme baseline_asindex = Scheme::BaselineAsIndex;
    static const Scheme fast_baseline_hopping = Scheme::FastBaselineHopping;
    static const Scheme fast_baseline_asindex = Scheme::FastBaselineAsIndex;

    static const Scheme topk_hopping = Scheme::TopkHopping;
    static const Scheme topk_hopping_rmq = Scheme::TopkHoppingRMQ;
    static const Scheme topk_hopping_wand = Scheme::TopkHoppingWAND;
    static const Scheme topk_hopping_rmq_wand = Scheme::TopkHoppingRMQWAND;
};

}
}
