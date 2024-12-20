#ifndef KLIPPER4408_INTERFACE_H
#define KLIPPER4408_INTERFACE_H

#include "RemotePrinter.h"
#include <future>
#include "basickernelexport.h"
//#include "materialboxmodellist.h"
#include <functional>

namespace boost
{
	namespace asio {
		class io_context;
	}
}
namespace creative_kernel
{
	using FuncGetDevStateCb = std::function<void(const RemotePrinter&)>;
	using FuncGetFileInfoCb = std::function<void(std::string, std::string)>;
	using FuncGetMaterialBoxInfoCb = std::function<void(std::string, std::string)>;

	struct PrinterMaterialItem {
		std::string type;
		std::string color;
		int boxId;
		int materialId;
	};

	struct MaterialParams {
		int boxId;        
		int id;          
		std::string rfid;  
		std::string vendor;  
		std::string type;  
		std::string name; 
		std::string color;  
		double minTemp;   
		double maxTemp;    
		double pressure;  
	};

	class WsClient;
	class BASIC_KERNEL_API Klipper4408Interface: public QObject
	{
		Q_OBJECT

		public:
			Klipper4408Interface();
			virtual ~Klipper4408Interface();

		private:
			const std::string cUrlSuffixUpload = "/upload/";
			const std::string cUrlSuffixGetPrinterInfo= "/info";
			
			boost::asio::io_context* m_ioc = nullptr;
			std::map<std::string, WsClient*> m_webSockets;
			FuncGetDevStateCb m_pfnDeviceStateCb = nullptr;
			FuncGetFileInfoCb m_pfnGcodeFileListCb = nullptr;
			FuncGetFileInfoCb m_pfnHistoryFileListCb = nullptr;
			FuncGetMaterialBoxInfoCb m_pfnMaterialBoxListCb = nullptr;


		public:
			void addClient(const std::string& strServerIp, const std::string& strMac, int port, std::function<void(const RemotePrinter&)> infoCallback, std::function<void(const std::string&, const std::string&)> fileCallback, std::function<void(std::string, std::string)> historyFileListCb, std::function<void(const std::string&, const std::string&)> videoCallback);
			void removeClient(const std::string& strServerIp);
			std::future<void> sendFileToDevice(const std::string& strServerIp, const int& port, const std::string& fileName, const std::string& filePath, std::function<void(float)> callback, std::function<void(int)> errorCallback);
			void getHistoryListFromDevice(const std::string& strServerIp, std::function<void(std::string, std::string)> callback);
			void getMaterialBoxListFromDevice(const std::string& strServerIp, std::function<void(std::string, std::string)> callback);
			void getPrinterInfo(const std::string& strServerIp, int port, std::function<void(std::string, std::string)> callback);
			void controlPrinter(const std::string& strServerIp, const int& port, const PrintControlType& cmdType, const std::string& value = "");
			void transparentCommand(const std::string& strServerIp, const int& port, const std::string& value);
			void deleteGcodeFile(const std::string& strIp, const std::string& filePath);
			void deleteHistoryFile(const std::string& strIp, int id);
			void deleteVideoFile(const std::string& strIp, const std::string& filePath);
			void renameVideoFile(const std::string& strIp, const std::string& filePath, const std::string& targetName);
			void renameGcodeFile(const std::string& strIp, const std::string& filePath, const std::string& targetName);
			void setMaterialColorMap(const QString& ip, QString path,const QList<QVariantMap>& colorMap);
			void feedInOrOutMaterial(const std::string& strIp, const std::string& boxId, const std::string& materialId, const std::string& isFeed);
			void refreshBox(const std::string& strIp, const std::string& boxId, const std::string& materialId);
			void modifyMaterial(const std::string& strIp, const QVariantMap& materialParams);
			void boxConfig(const std::string& strIp, const std::string& autoRefill, const std::string& cAutoFeed, const std::string& cSelfTest);
			void getColorMap(const std::string& strIp, const std::string& filePath);
		
		private slots:
			void slotRemoveClient(const std::string& ipAddrClient);
			void slotRetDeviceState(const RemotePrinter& printer);
			void slotRetGcodeFileList(const std::string& ipAddrClient, const std::string& fileList);
			void slotRetHistoryFileList(const std::string& ipAddrClient, const std::string& fileList);
			void slotRetMaterialBoxList(const std::string& ipAddrClient, const std::string& boxList);
	};
}


#endif
