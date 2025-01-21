#ifndef KLIPPERCX_INTERFACE_H
#define KLIPPERCX_INTERFACE_H

#include <future>
#include <functional>
#include <string>
#include "../UploadFile.hpp"
#include "nlohmann/json_fwd.hpp"
namespace RemotePrint {
	class KlipperCXInterface {
public:
	KlipperCXInterface();
	virtual ~KlipperCXInterface();

	std::future<void> sendFileToDevice(const std::string& serverIp, int port, const std::string& uploadFileName, const std::string& localFilePath, std::function<void(float)> progressCallback, std::function<void(int)> uploadStatusCallback, std::function<void(std::string)> onCompleteCallback);

private:
	const std::string urlSuffixUpload = "/upload/";
    Slic3r::GUI::UploadFile m_upload_file;
};
}


#endif // KLIPPER4408_INTERFACE_H
