#pragma once

#include "ps/graphs/edges.hpp"

namespace ps {
namespace indices {

template <class Enumerator, typename T>
void fill_vector(Enumerator en, std::vector<T>& vec)
{
    size_t offset = vec.size();
    vec.resize(vec.size() + en.size() - en.position());

    for (size_t i = en.position(); i < en.size(); ++i, en.next())
        vec[offset++] = en.docid();
}

template <class Index>
bool neighbors(const Index& index, int source, graphs::Edges& edges)
{
    uint64_t offset;
    edges.first = source;
    edges.second.clear();

    if (!index.get_offset(source, offset))
        return false;

    auto en = index.sequence_at(offset);
    fill_vector(en, edges.second);

    return true;
}

template <class Index>
bool neighbors(const Index& index, int source, graphs::Edges& edges, int k)
{
    uint64_t offset;
    edges.first = source;
    edges.second.clear();

    if (!index.get_offset(source, offset))
        return false;

    auto en = index.sequence_at(offset);

    if (k == 1)
    {
        fill_vector(en, edges.second);
        return true;
    }

    if (k == 2)
    {
        fill_vector(en, edges.second);
        en.reset();

        for (size_t i = en.position(); i < en.size(); ++i, ++offset, en.next())
        {
            uint64_t friend_offset;
            if (index.get_offset(en.docid(), friend_offset))
            {
                auto friend_en = index.sequence_at(friend_offset);
                fill_vector(friend_en, edges.second);
            }
        }

        std::sort(edges.second.begin(), edges.second.end());
        edges.second.erase(std::unique(edges.second.begin(), edges.second.end()), edges.second.end());
        edges.second.erase(std::remove(edges.second.begin(), edges.second.end(), (uint64_t)source), edges.second.end());
        return true;
    }

    std::unordered_set<int> visited;
    std::unordered_set<int> union_lst;
    std::queue<int> curr_queue;
    std::queue<int> next_queue;

    curr_queue.push(source);

    while (--k >= 0)
    {
        while (!curr_queue.empty())
        {
            int node = curr_queue.front();

            visited.insert(node);
            curr_queue.pop();

            if (index.get_offset(node, offset))
            {
                auto node_index_en = index.sequence_at(offset);

                for (size_t i = 0; i < node_index_en.size(); ++i, node_index_en.next())
                {
                    int next_node = node_index_en.docid();
                    union_lst.insert(next_node);

                    if (visited.find(next_node) == visited.end() && k >= 0)
                        next_queue.push(next_node);
                }
            }
        }

        std::swap(curr_queue, next_queue);
    }

    // Remove ourself from the list (we don't like to have imaginary friends)
    auto to_delete = union_lst.find(source);

    if (to_delete != union_lst.end())
        union_lst.erase(to_delete);

    if (union_lst.empty())
        return false;

    edges.first = source;
    edges.second.resize(union_lst.size());
    std::copy(union_lst.begin(), union_lst.end(), edges.second.begin());
    std::sort(edges.second.begin(), edges.second.end());

    return true;
}

}
}