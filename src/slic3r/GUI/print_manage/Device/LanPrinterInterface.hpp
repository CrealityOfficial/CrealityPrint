#ifndef LANPRINTER_INTERFACE_H
#define LANPRINTER_INTERFACE_H

#include <functional>
#include <future>
#include <string>
#include "../UploadCancellation.hpp"

namespace RemotePrint {
class LanPrinterInterface
{
public:
    LanPrinterInterface();
    virtual ~LanPrinterInterface();

    std::future<void> sendFileToDevice(const std::string&         strIp,
                                       const std::string&         fileName,
                                       const std::string&         filePath,
                                       std::function<void(float,double)> callback,
                                       std::function<void(int)>   errorCallback,
                                       std::function<void(std::string)> onCompleteCallback,
                                       UploadCancelToken cancelToken);

private:
    const int         cServerPort      = 81;
    const std::string cUrlSuffixUpload = "/server/files/upload";
};
} // namespace RemotePrint

#endif // LANPRINTER_INTERFACE_H
