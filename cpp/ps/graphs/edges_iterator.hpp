#pragma once

#include "ps/utils.hpp"

namespace ps {
namespace graphs {

template <typename GraphType>
class edges_iterator {
public:
    edges_iterator(GraphType& graph)
        : m_graph(graph)
    {
        bool valid = m_graph.get_next(m_current);
        PS_ASSERT(valid);
    }

    bool get_next(Edges& edges)
    {
        edges.first = m_current.first;
        edges.second.clear();

        if (m_current.first == -1)
            return false;

        edges.second.push_back(m_current.second);

        Edge next;
        std::vector<int> neighbors;
        bool stream_over = true;

        while (m_graph.get_next(next))
        {
            if (next.first == m_current.first)
                edges.second.push_back(next.second);
            else
            {
                m_current = next;
                stream_over = false;
                break;
            }
        }

        if (stream_over)
        {
            m_current = Edge(-1, -1);
            return true;
        }

        return !stream_over;
    }

private:
    GraphType& m_graph;
    Edge m_current;
};

}
}