#ifndef KLIPPER_INTERFACE_H
#define KLIPPER_INTERFACE_H

#include <functional>
#include <future>
#include <string>
#include "slic3r/Utils/Http.hpp"
#include "../UploadCancellation.hpp"

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
                                       std::function<void(float,double)> progressCallback,
                                       std::function<void(int)>   errorCallback, 
                                       std::function<void(std::string)> onCompleteCallback,
                                       UploadCancelToken cancelToken);
    std::string extractIP(const std::string& str); 
    void cancelSendFileToDevice(const UploadCancelToken& cancelToken);

private:
	const std::string urlSuffixUpload = "/server/files/upload";
};
} // namespace RemotePrint

#endif // KLIPPER_INTERFACE_H
