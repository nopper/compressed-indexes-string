#pragma once

namespace ps {

class configuration {
public:
    static configuration const& get() {
        static configuration instance;
        return instance;
    }

    static configuration& get_nonconst() {
        static configuration instance;
        return instance;
    }

    size_t worker_threads;
    std::string queue_impl;
    double work_per_thread;

    int indices_topk_threshold;
    int indices_topk_rmq_wand_threshold;
    size_t indices_topk_rmq_sizehint;

private:
    configuration()
    {
        // Queues specific configurations
        fillvar("PS_THREADS", worker_threads, std::thread::hardware_concurrency());
        fillvar("PS_QUEUE", queue_impl, "ordered");
        fillvar("PS_WORK_PER_THREAD", work_per_thread, 1 << 24);

        // Indices specific configurations
        fillvar("PS_INDICES_TOPK_THRESHOLD", indices_topk_threshold, 10);
        fillvar("PS_INDICES_TOPK_RMQ_WAND_THRESHOLD", indices_topk_rmq_wand_threshold, 1000);
        fillvar("PS_INDICES_TOPK_RMQ_SIZEHINT", indices_topk_rmq_sizehint, 1 << 20);
    }

    template <typename T, typename T2>
    void fillvar(const char* envvar, T& var, T2 def)
    {
        const char* val = std::getenv(envvar);
        if (!val || !strlen(val)) {
            var = def;
        } else {
            var = boost::lexical_cast<T>(val);
        }
    }
};

}
