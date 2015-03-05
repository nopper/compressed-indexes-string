#pragma once

struct node_info {
    uint64_t docid;
    uint64_t offset;
    int cdf_degree;
    int score;

    node_info()
        : cdf_degree(0)
    {}

    node_info(uint64_t doc, uint64_t off, uint64_t cdf, uint64_t scr)
        : docid(doc)
        , offset(off)
        , cdf_degree(cdf)
        , score(scr)
    {}

    friend std::ostream& operator<<(std::ostream& os, const node_info& n)
    {
        os << "docid=" << n.docid << " offset=" << n.offset << " cdf_degree=" << n.cdf_degree << " score=" << n.score;
        return os;
    }
};