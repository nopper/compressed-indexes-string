#ifndef QUEUES_HPP
#define QUEUES_HPP

#include <boost/thread/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/mutex.hpp>

#include <boost/signals2/signal.hpp>
#include <boost/bind.hpp>

#include <boost/lexical_cast.hpp>

#include <thread>
#include <sstream>
#include <queue>

#include "ps/utils.hpp"
#include "ps/configuration.hpp"

namespace ps {
namespace queues {

class job {
public:
    virtual void prepare(void* user_data) = 0;
    virtual void commit(void* user_data) = 0;
};

typedef std::shared_ptr<job> job_ptr_type;
typedef std::vector<job_ptr_type> job_list;

void thread_main(void* user_data, const job_list& work_queue)
{
    for (auto const& j: work_queue)
        j->prepare(user_data);
}

typedef void (*thread_func)(void*, const job_list&);

class queue : boost::noncopyable {
public:
    queue(double work_per_thread = configuration::get().work_per_thread,
          void* user_data = nullptr,
          thread_func runner = thread_main)
        : m_user_data(user_data)
        , m_runner(runner)
    {}

    virtual ~queue()
    {}

    virtual void add_job(job_ptr_type j, double expected_work) = 0;
    virtual void complete() = 0;

protected:
    void* m_user_data;
    thread_func m_runner;
};


class ordered_queue : public queue {
public:
    ordered_queue(double work_per_thread = configuration::get().work_per_thread,
                  void* user_data = nullptr,
                  thread_func runner = thread_main)
        : queue(work_per_thread, user_data, runner)
        , m_expected_work(0)
        , m_work_per_thread(work_per_thread)
        , m_active_threads(0)
        , m_committing(false)
    {
        m_max_threads = configuration::get().worker_threads;
        logger() << "ordered queue using " << m_max_threads
                 << " worker threads (" << (int)work_per_thread
                 << " work per thread)" << std::endl;
    }

    virtual void add_job(job_ptr_type j, double expected_work)
    {
        if (m_max_threads) {
            m_next_thread.first.push_back(j);
            m_expected_work += expected_work;
            if (m_expected_work >= m_work_per_thread) {
                spawn_next_thread();
            }
        } else { // all in main thread
            j->prepare(m_user_data);
            j->commit(m_user_data);
            j.reset();
        }
    }

    ~ordered_queue()
    {
        complete();
    }

    virtual void complete()
    {
        if (!m_next_thread.first.empty()) {
            spawn_next_thread();
        }

        while (m_active_threads > 0) {
            commit_thread();
        }

        if (m_committing)
            wait_commit();
    }

private:
    void spawn_next_thread()
    {
        if (m_active_threads >= m_max_threads)
            commit_thread();

        m_running_threads.emplace_back();
        std::swap(m_next_thread, m_running_threads.back());

        const job_list& cur_queue = m_running_threads.back().first;
        m_active_threads++;
        m_running_threads.back().second = boost::thread(
            boost::bind(m_runner, m_user_data, cur_queue)
        );

        m_expected_work = 0;
    }

    static void commit_thread_main(void * user_data, std::vector<job_ptr_type>& queue)
    {
        for (auto& j: queue) {
            j->commit(user_data);
            j.reset();
        }
    }

    void wait_commit()
    {
        m_commit_thread.join();

        if (m_committing)
            m_running_threads.pop_front();

        m_committing = false;
    }

    void commit_thread()
    {
        assert(m_active_threads > 0);
        wait_commit();

        m_running_threads.front().second.join();
        m_active_threads--;

        std::vector<job_ptr_type>& queue = m_running_threads.front().first;
        m_committing = true;
        m_commit_thread = boost::thread(
            ordered_queue::commit_thread_main, m_user_data, queue
        );
    }

    typedef std::pair<std::vector<job_ptr_type>, boost::thread> thread_t;
    thread_t m_next_thread;
    std::deque<thread_t> m_running_threads;

    size_t m_expected_work;
    double m_work_per_thread;
    size_t m_max_threads;
    size_t m_active_threads;
    bool m_committing;
    boost::thread m_commit_thread;
};

class unordered_multi_queue : public queue {
public:

    unordered_multi_queue(double work_per_thread = configuration::get().work_per_thread,
                          void* user_data = nullptr,
                          thread_func runner = thread_main)
        : queue(work_per_thread, user_data, runner)
        , m_num_tasks(0)
        , m_free_thread(-1)
        , m_expected_work(0)
        , m_work_per_thread(work_per_thread)
    {
        m_max_threads = configuration::get().worker_threads;
        logger() << "not ordered multi queue using " << m_max_threads
                 << " worker threads (" << (int)work_per_thread
                 << " work per thread)" << std::endl;

        m_threads.resize(m_max_threads);
        m_running.resize(m_max_threads, false);
        m_queues.resize(m_max_threads);
    }

    virtual void add_job(job_ptr_type j, double expected_work)
    {
        find_free_thread();

        m_queues[m_free_thread].push_back(j);
        m_expected_work += expected_work;

        if (m_expected_work > m_work_per_thread)
            spawn_next_thread();
    }

    ~unordered_multi_queue()
    {
        complete();
    }

    virtual void complete()
    {
        // We first check that we have job to do but that has not been spawned yet
        if (m_free_thread >= 0 && m_expected_work > 0)
            spawn_next_thread();

        boost::unique_lock<boost::mutex> lock(m_wait_mutex);

        while (m_num_tasks > 0)
            m_task_completed.wait(lock);

        for (size_t i = 0; i < m_queues.size(); ++i)
        {
            if (!m_running[i] && !m_queues[i].empty())
                commit_thread(i);
        }
    }

private:
    void spawn_next_thread()
    {
        boost::unique_lock<boost::mutex> lock(m_wait_mutex);

        m_num_tasks++;
        m_running[m_free_thread] = true;
        m_threads[m_free_thread] = new boost::thread(
            boost::bind(&unordered_multi_queue::thread_func, this,
                        m_free_thread, m_queues[m_free_thread])
        );

        m_expected_work = 0;
        m_free_thread = -1;
    }

    void thread_func(int thread_id, const job_list& work_queue)
    {
        m_runner(m_user_data, work_queue);

        boost::unique_lock<boost::mutex> lock(m_wait_mutex);

        if (--m_num_tasks <= 0)
            m_num_tasks = 0;

        m_running[thread_id] = false;
        m_task_completed.notify_all();
    }

    void commit_thread(int i)
    {
        for (auto& j: m_queues[i])
        {
            j->commit(m_user_data);
            j.reset();
        }

        m_queues[i].clear();
    }

    void find_free_thread()
    {
        if (m_free_thread >= 0)
            return;

        boost::unique_lock<boost::mutex> lock(m_wait_mutex);

        while (m_num_tasks == m_queues.size())
            m_task_completed.wait(lock);

        m_free_thread = 0;
        while (m_free_thread < (int)m_queues.size() &&
               m_running[m_free_thread])
            m_free_thread++;

        commit_thread(m_free_thread);
    }

    size_t m_num_tasks;
    std::vector<bool> m_running;
    std::vector<std::vector<job_ptr_type>> m_queues;
    std::vector<boost::thread*> m_threads;

    boost::mutex m_wait_mutex;
    boost::condition_variable m_task_completed;

    int m_free_thread;
    size_t m_expected_work;
    double m_work_per_thread;
    size_t m_max_threads;
};

class unordered_single_queue : public queue {
public:
    unordered_single_queue(double work_per_thread = configuration::get().work_per_thread,
                           void* user_data = nullptr,
                           thread_func runner = thread_main)
        : queue(work_per_thread, user_data, runner)
        , m_num_tasks(0)
        , m_shutdown(false)
    {
        size_t max_threads = configuration::get().worker_threads;
        logger() << "not ordered single queue using " << max_threads
                 << " worker threads (" << (int)work_per_thread
                 << " work per thread)" << std::endl;

        using boost::bind;
        using boost::thread;

        for (size_t i = 0; i < max_threads; ++i)
            m_threads.add_thread(new thread(bind(&unordered_single_queue::thread_func, this)));
    }

    ~unordered_single_queue()
    {
        {
            boost::lock_guard<boost::shared_mutex> write_lock(m_shutdown_mutex);
            m_shutdown = true;
        }

        m_cond.notify_all();
        m_threads.join_all();
    }

    virtual void add_job(job_ptr_type j, double expected_work)
    {
        add_job(j, expected_work, true);
    }

    virtual void add_job(job_ptr_type j, double /*expected_work*/, bool blocking)
    {
        if (blocking)
        {
            boost::unique_lock<boost::mutex> lock(m_wait_mutex);
            while (m_num_tasks == (int)m_threads.size())
                m_task_completed.wait(lock);
        }

        {
            boost::lock_guard<boost::mutex> task_queue_lock(m_mutex);
            boost::lock_guard<boost::mutex> task_count_lock(m_wait_mutex);

            m_tasks.push(j);
            ++m_num_tasks;
        }

        m_cond.notify_one();
    }

    virtual void complete()
    {
        wait();
    }

    void wait()
    {
        try
        {
            boost::unique_lock<boost::mutex> lock(m_wait_mutex);
            while (m_num_tasks > 0)
                m_wait_cond.wait(lock);
        }
        catch (boost::thread_resource_error& e)
        {
            std::ostringstream msg;
            msg << "thread_resource_error while waiting for thread pool to empty: "
                << e.what();
            handle_thread_exception(boost::this_thread::get_id(), msg.str());
        }
        catch (boost::thread_interrupted&)
        {
            handle_thread_exception(boost::this_thread::get_id(),
                "interrupted while waiting for thread pool to empty");
        }
    }

private:
    void thread_func()
    {
        job_ptr_type j;

        for(;;)
        {
            try
            {
                {
                    typedef boost::shared_lock<boost::shared_mutex> read_lock;
                    read_lock r(m_shutdown_mutex);
                    if (m_shutdown)
                        break;
                }

                boost::unique_lock<boost::mutex> lock(m_mutex);

                if (m_tasks.empty())
                {
                    m_cond.wait(lock);
                    if (m_tasks.empty())
                        continue;
                }

                j = m_tasks.front();
                m_tasks.pop();
            }
            catch (boost::thread_resource_error& e)
            {
                std::ostringstream msg;
                msg << "fatal thread_resource_error: " << e.what();
                handle_thread_exception(boost::this_thread::get_id(), msg.str());
                break;
            }
            catch (boost::thread_interrupted&)
            {
                handle_thread_exception(boost::this_thread::get_id(),
                    "non-fatal thread interruption");
                continue;
            }

            {
                std::vector<job_ptr_type> work_queue(1, j);
                m_runner(m_user_data, work_queue);

                boost::unique_lock<boost::mutex> lock(m_commit_mutex);
                j->commit(m_user_data);
                j.reset();
            }

            boost::unique_lock<boost::mutex> lock(m_wait_mutex);
            if (--m_num_tasks <= 0)
            {
                m_num_tasks = 0;
                m_wait_cond.notify_all();
            }

            m_task_completed.notify_all();
        }
    }

    std::queue<job_ptr_type> m_tasks;
    boost::thread_group m_threads;

    boost::mutex m_mutex;
    boost::condition_variable m_cond;

    int m_num_tasks;
    boost::mutex m_wait_mutex;
    boost::condition_variable m_wait_cond;

    bool m_shutdown;
    boost::shared_mutex m_shutdown_mutex;

    boost::mutex m_commit_mutex;

    boost::mutex m_wait_task_completion;
    boost::condition_variable m_task_completed;

    typedef boost::signals2::signal<void (const boost::thread::id&,
                                          const std::string&)>
           thread_exception_handler;
    thread_exception_handler handle_thread_exception;
};

queue* new_queue(void* user_data = nullptr, thread_func runner = thread_main)
{
    const std::string& impl = configuration::get().queue_impl;
    double work_per_thread = configuration::get().work_per_thread;

    if (impl == "unordered_single")
        return new unordered_single_queue(work_per_thread, user_data, runner);
    if (impl == "unordered_multi")
        return new unordered_multi_queue(work_per_thread, user_data, runner);

    return new ordered_queue(work_per_thread, user_data, runner);
}


}
}

#endif
