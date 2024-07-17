#ifndef CXCLOUD_TOOL_THREAD_POOL_H_
#define CXCLOUD_TOOL_THREAD_POOL_H_

#include <functional>
#include <future>
#include <memory>
#include <thread>

#include "cxcloud/interface.hpp"

namespace cxcloud {

  class CXCLOUD_API ThreadPool final {
   public:
    explicit ThreadPool(size_t thread_number = std::thread::hardware_concurrency());
    ~ThreadPool();

   public:
    auto execute(std::function<void(void)> task) -> std::future<void>;

   private:
    struct Internal;

   private:
    std::unique_ptr<Internal> internal_{ nullptr };
  };

}  // namespace cxcloud

#endif  // CXCLOUD_TOOL_THREAD_POOL_H_
