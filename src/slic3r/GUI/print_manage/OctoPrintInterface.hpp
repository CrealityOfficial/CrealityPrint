#ifndef OCTOPRINT_INTERFACE_H
#define OCTOPRINT_INTERFACE_H

#include <functional>
#include <string>

namespace RemotePrint {
class OctoPrintInterface
{
public:
    OctoPrintInterface();
    virtual ~OctoPrintInterface();

    void sendFileToDevice(const std::string& strServerIp, const std::string& strToken, const std::string& filePath);

private:
    const int         cServerPort    = 80;
    const std::string cUrlSuffixFile = "/api/files/sdcard";
};
} // namespace RemotePrint

#endif // OCTOPRINT_INTERFACE_H