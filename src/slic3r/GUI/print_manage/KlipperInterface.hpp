#ifndef KLIPPER_INTERFACE_H
#define KLIPPER_INTERFACE_H

#include <functional>
#include <future>
#include <string>

namespace RemotePrint {
class KlipperInterface
{
public:
    KlipperInterface();
    virtual ~KlipperInterface();

    std::future<void> sendFileToDevice(const std::string&         serverIp,
                                       int                        port,
                                       const std::string&         fileName,
                                       const std::string&         filePath,
                                       std::function<void(float)> progressCallback,
                                       std::function<void(int)>   errorCallback, 
                                       std::function<void(std::string)> onCompleteCallback);

private:
    const std::string urlSuffixUpload = "/server/files/upload";
};
} // namespace RemotePrint

#endif // KLIPPER_INTERFACE_H