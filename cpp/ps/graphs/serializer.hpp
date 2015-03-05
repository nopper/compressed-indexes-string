#pragma once

#include "ps/queues.hpp"
#include "ps/sequences/options.hpp"
#include "ps/graphs/graph_types.hpp"

namespace ps {
namespace graphs {

template<typename GraphType, typename Index>
class graph_serializer {
public:
    graph_serializer(GraphType& graph, Index& index)
        : m_builder(index)
        , m_opts(index.opts())
    {
        progress_logger plog("friends");
        edges_iterator<GraphType> iterator(graph);

        size_t written = 0;
        queues::ordered_queue q;

        Edges edges;

        while (iterator.get_next(edges))
        {
            std::shared_ptr<edges_write_job> jobptr(
                new edges_write_job(*this, edges)
            );
            q.add_job(jobptr, 1);

            plog.done_item();
            written++;
        }

        q.complete();
        m_builder.commit();

        plog.done();
    }
private:
    struct edges_write_job : queues::job {
        edges_write_job(graph_serializer& parent, Edges edges)
            : m_serializer(parent)
            , m_edges(edges)
        {}

        virtual void prepare(void* user_data)
        {
            // XX: for options() use const reference instead of copy
            Index::sequence_type::serialize(
                m_value,
                m_serializer.m_opts,
                m_edges.second.size(), m_edges.second.begin()
            );
        }

        virtual void commit(void* user_data)
        {
            m_serializer.m_builder.append(
                m_edges.first, m_value, m_edges.second.size()
            );
        }

        graph_serializer& m_serializer;
        Edges m_edges;
        std::string m_value;
    };

    typename Index::builder m_builder;
    sequences::options m_opts;
};


}
}
