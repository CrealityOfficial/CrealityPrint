#include "cxcloud/tool/thread_pool.h"

#include <boost/asio.hpp>
#include <boost/asio/thread_pool.hpp>

namespace cxcloud {

  struct ThreadPool::Internal {
    boost::asio::thread_pool pool;

    Internal(size_t thread_number) : pool{ thread_number } {}
  };

  ThreadPool::ThreadPool(size_t thread_number)
    : internal_{ std::make_unique<Internal>(thread_number) } {}

  ThreadPool::~ThreadPool() {
    internal_->pool.join();
  }

  auto ThreadPool::execute(std::function<void(void)> task) -> std::future<void> {
    return boost::asio::post(internal_->pool, std::packaged_task<void()>{ task });
  }

}  // namespace cxcloud
