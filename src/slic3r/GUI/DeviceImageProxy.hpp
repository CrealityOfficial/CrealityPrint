#ifndef slic3r_DeviceImageProxy_hpp_
#define slic3r_DeviceImageProxy_hpp_

#include <boost/asio/io_context.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

// 管理共享 SSL Context、并发数和等待队列
namespace Slic3r { namespace GUI {

struct DeviceImageRequest
{
    std::string address;
    std::string target_path;
    bool        secure{true};
};

struct DeviceImageResult
{
    unsigned                   status_code{0};
    std::string                content_type;
    std::vector<unsigned char> body;
    std::string                error;
};

//暴露外部的接口
class DeviceImageProxy
{
public:
    using Completion = std::function<void(DeviceImageResult)>;

    explicit DeviceImageProxy(boost::asio::io_context& io_context);
    ~DeviceImageProxy();

    DeviceImageProxy(const DeviceImageProxy&) = delete;
    DeviceImageProxy& operator=(const DeviceImageProxy&) = delete;

    void async_get(DeviceImageRequest request, Completion completion);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

}} // namespace Slic3r::GUI

#endif
