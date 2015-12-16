#ifndef WORKER_H
#define WORKER_H

#include <memory>
#include <vector>
#include <mutex>
#include <thread>

#include <boost/asio.hpp>
#include <boost/fiber/all.hpp>
#include <boost/core/noncopyable.hpp>

#include "session.h"

namespace petrel {

namespace ba = boost::asio;
namespace bf = boost::fibers;

class server;

/// Each io_service object runs in its own worker thread.
class worker : boost::noncopyable {
  public:
    set_log_tag_default_priority("worker");

    /// Ctor.
    worker() {}

    /// Dtor.
    ~worker();

    /// Create an acceptor for a tcp endpoint.
    void add_endpoint(ba::ip::tcp::endpoint& ep);

    /// Add a session to this worker
    void add_session(session::pointer new_session);

    /// Start a worker thread and return.
    void start(server& srv);

    /// Stop a worker
    void stop();

    /// Entry point.
    void run(server& srv);

    /// Block until the thread is finished.
    void join();

    /// Return the io_service object of the worker.
    inline ba::io_service& io_service() { return m_iosvc; }

  private:
    ba::io_service m_iosvc;
    std::thread m_thread;
    std::vector<session::pointer> m_new_sessions;
    std::mutex m_new_session_mtx;

    std::vector<ba::ip::tcp::acceptor> m_acceptors;

    static void do_accept(ba::ip::tcp::acceptor& acceptor, server& srv);

    /// Called from the timer_handler. Start fibers for new sessions.
    inline void start_new_sessions() {
        std::lock_guard<std::mutex> lock(m_new_session_mtx);
        for (auto session : m_new_sessions) {
            bf::fiber(&session::start, session).detach();
        }
        m_new_sessions.clear();
    }
};

}  // petrel

#endif  // WORKER_H
