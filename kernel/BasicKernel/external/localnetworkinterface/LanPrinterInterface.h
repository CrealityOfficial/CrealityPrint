#ifndef LANPRINTER_INTERFACE_H
#define LANPRINTER_INTERFACE_H
#include "basickernelexport.h"
#include "RemotePrinter.h"
#include <unordered_map>
#include <functional>
#include "RemotePrinterSession.h"
#include <future>
namespace creative_kernel
{
	class BASIC_KERNEL_API LanPrinterInterface
	{
	public:
		LanPrinterInterface();
		virtual ~LanPrinterInterface();
	private:
		const int cServerPort = 81;
		const std::string cUrlSuffixState = "/protocal.csp";

	public:
		void getDeviceState(const std::string& strServerIp, std::function<void(std::unordered_map<std::string, std::string>, RemotePrinterSession)> callback, RemotePrinterSession printerSession = {});
		std::future<void> sendFileToDevice(const std::string& strIp, const std::string& fileName, const std::string& filePath, std::function<void(float)> callback, std::function<void(int)> errorCallback);
		std::list<std::string> getFileListFromDevice(const std::string& strIp, std::function<void(std::string)> callback);
		void getFileList(const std::string& strIp);
		int controlPrinter(const std::string& strServerIp, const PrintControlType& cmdType, const std::string& value = "", std::function<void()> callback = nullptr);
		void transparentCommand(const std::string& strServerIp, const std::string& value, std::function<void()> callback = nullptr);
		void fetchHead(const std::string& strIp, std::string filePath, std::function<void(std::string, std::string)> callback);
		bool fetchHeadCallback(std::string data, intptr_t userdata, std::string fullPath, std::function<void(std::string, std::string)> callback);
		void fetchBody(const std::string& fullPath, std::string range, std::function<void(std::string, std::string)> callback, std::string imageType, int size);
		bool fetchBodyCallback(std::string data, intptr_t userdata, std::function<void(std::string, std::string)> callback, std::string imageType, int size);
		void deleteFile(const std::string& strIp, std::string filePath);
	};
}


#endif // LANPRINTER_INTERFACE_H


