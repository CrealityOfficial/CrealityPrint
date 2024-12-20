#ifndef OCTOPRINT_INTERFACE_H
#define OCTOPRINT_INTERFACE_H
#include "basickernelexport.h"
#include "RemotePrinter.h"
#include <functional>

namespace creative_kernel
{
	class BASIC_KERNEL_API OctoPrintInterface
	{
	public:
		OctoPrintInterface();
		virtual ~OctoPrintInterface();

	private:
		const int cServerPort = 80;
		const std::string cUrlSuffixState = "/api/printer";
		const std::string cUrlSuffixJob = "/api/job";
		const std::string cUrlSuffixBed = "/api/printer/bed";
		const std::string cUrlSuffixFile = "/api/files/sdcard";

	public:
		void getDeviceState(const std::string& strServerIp, const std::string& strToken, std::function<void(const std::string&&)> callback);
		void sendFileToDevice(const std::string& strServerIp, const std::string& strToken, const std::string& filePath);
		void getFileListFromDevice(const std::string& strIp);
		void getFileList(const std::string& strIp);
		void controlPrinter(const std::string& strServerIp, const std::string& strToken, const PrintControlType& cmdType, const std::string& value = "");
		void transparentCommand(const std::string& strServerIp, const std::string& strToken, const std::string& value);
	};
}


#endif // OCTOPRINT_INTERFACE_H


