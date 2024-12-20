#ifndef KLIPPER_INTERFACE_H
#define KLIPPER_INTERFACE_H
#include "basickernelexport.h"
#include "RemotePrinter.h"
#include <functional>
#include "RemotePrinterSession.h"
#include <future>
namespace creative_kernel
{
	class BASIC_KERNEL_API KlipperInterface
	{
	public:
		KlipperInterface();
		virtual ~KlipperInterface();

	private:
		const std::string cUrlSuffixState = "/printer/objects/query?";
		const std::string cUrlSuffixJob = "/printer/print/start";
		const std::string cUrlSuffixBed = "/api/printer/bed";
		const std::string cUrlSuffixFile = "/server/files/list";
		const std::string cUrlSuffixUpload = "/server/files/upload";
		const std::string cUrlSuffixSystemInfo = "/machine/system_info";
		const std::string cUrlSuffixGetMultiMachine = "/machine/multi_machine";

	public:
		void getDeviceState(const std::string& strServerIp, const int& port, std::function<void(const std::string&, RemotePrinterSession)> callback, RemotePrinterSession printerSession);
		void getSystemInfo(const std::string& strServerIp, const int& port, std::function<void(const std::string&, RemotePrinterSession)> callback, RemotePrinterSession printerSession);
		std::future<void> sendFileToDevice(const std::string& strServerIp, const int& port, std::string fileName, std::string filePath, std::function<void(float)> callback, std::function<void(int)> errorCallback);
		void getFileListFromDevice(const std::string& strIp);
		void getFileList(const std::string& strIp);
		void controlPrinter(const std::string& strServerIp, const int& port, const PrintControlType& cmdType, const std::string& value = "");
		void transparentCommand(const std::string& strServerIp, const int& port, const std::string& value);
		void getMultiMachine(const std::string& strServerIp, const int& port = 80, std::function<void(std::vector<RemotePrinter>&)> callback = nullptr);
	};
}


#endif // OCTOPRINT_INTERFACE_H


