#ifndef REMOTE_PRINTER_MANAGER_H
#define REMOTE_PRINTER_MANAGER_H

#include "basickernelexport.h"
#include "qtusercore/module/singleton.h"
//#include "RemotePrinter.h"
#include "RemotePrinterSession.h"
#include <functional>
#include <mutex>
#include <condition_variable>
#include "LanPrinterInterface.h"
#include "OctoPrintInterface.h"
#include "KlipperInterface.h"
//#include "Klipper4408Interface.h"
#include "cpr/cpr.h"

struct RemotePrinter;

//�������Ƕ����ûص��ķ�ʽ��֪ͨUI���Ʋ㣬����Qt�ź���۵����������ں�����ֲ

namespace creative_kernel
{
	class Klipper4408Interface;

	using FuncSearchCb = std::function<void(const RemotePrinter&)>;
	using FuncGetPreviewCb = std::function<void(std::string, std::string, std::string)>;
	using FuncGetFileListCb = std::function<void(std::string, std::string, RemotePrinerType)>;
	using FuncGetPrinterInfoCb = std::function<void(const RemotePrinter&)>;
	using FuncGetMaterialBoxCb = std::function<void(std::string, std::string, RemotePrinerType)>;

	class BASIC_KERNEL_API RemotePrinterManager
	{
		SINGLETON_DECLARE(RemotePrinterManager)

	public:
		virtual ~RemotePrinterManager();

	public:
		void Init();
		void searchDevices();
		void searchLanDeviceThread();
		void refreshStateThread();
		
		void addPrinter(const RemotePrinter& printer);
		void tryAddPrinter(cpr::Response r, RemotePrinter printer);
		void addPrinter(const QList<RemotePrinter>& listPrinter);
		void doAddPrinter(const RemotePrinter& printer, bool lockEnabled = true);

		std::string deletePrinter(const RemotePrinter& printer);
		void getPrinterInfo(RemotePrinter& printer);

		//void getFileList(const RemotePrinter& printer, const std::string& strDir);
		void pushFile(const RemotePrinter& printer, const std::string& fileName, const std::string& filePath, std::function<void(std::string, std::string, float)> callback, std::function<void(std::string, std::string, int)> errorCallback = nullptr);
		//void pushFile2AllPrinters(const std::string& filePath);
		void getPreviewImg(const RemotePrinter& printer, const std::string& filePath);
		void deleteFile(const std::string& strIp, const std::string& value, RemotePrinerType printerType, const OpPrinerFileType& fileType = OpPrinerFileType::GCODE_FILE);
		void renameFile(const std::string& strIp, const std::string& value, const std::string& targetName, RemotePrinerType printerType, const OpPrinerFileType& fileType = OpPrinerFileType::GCODE_FILE);
		void materialColorMap(const QString& ip, QString path, const QList<QVariantMap>& colorMap);
		void feedInOrOutMaterial(const std::string& strIp, const std::string& boxId, const std::string& materialId, const std::string& isFeed);
		void refreshBox(const std::string& strIp, const std::string& boxId, const std::string& materialId);
		void boxConfig(const std::string& strIp, const std::string& autoRefill, const std::string& cAutoFeed, const std::string& cSelfTest);
		void modifyMaterial(const std::string& strIp, const QVariantMap& materialParams);
		void getColorMap(const std::string& strIp, const std::string& filePath);
		//void getCurrentJob(RemotePrinter& printer);
		//void controlJob(RemotePrinter& printer, const int& controlType);//0:start 1:cancel 2:restart 3:pause 4:resume

		void controlPrinter(const RemotePrinter& printer, const PrintControlType& cmdType, const std::string& value = "");
		void transparentCommand(const RemotePrinter& printer, const std::string& value);
		void uploadFile(const RemotePrinter& printer, const std::string& fileName, const std::string& filePath, std::function<void(std::string, std::string, float)> callback, std::function<void(std::string, std::string, int)> errorCallback = nullptr);
		void downloadFile(const std::string& ipAddress, const std::string& filePath, std::function<void(const float&)> callback);
		void downloadThread();
		void uploadThread();
	public:
		void setSearchCb(FuncSearchCb callback) { m_pfnSearchCb = callback; }
		void setGetPreviewCb(FuncGetPreviewCb callback) { m_pfnGetPreviewCb = callback; }
		void setGetGcodeListCb(FuncGetFileListCb callback) { m_pfnGetGcodeListCb = callback; }
		void setGetHistoryListCb(FuncGetFileListCb callback) { m_pfnGetHistoryListCb = callback; }
		void setGetVideoListCb(FuncGetFileListCb callback) { m_pfnGetVideoListCb = callback; }
		void setGetPrinterInfoCb(FuncGetPrinterInfoCb callback) { m_pfnGetPrinterInfoCb = callback; }
		void setGetMaterialBoxCb(FuncGetMaterialBoxCb callback) { m_pfnGetMaterialBoxCb = callback; }

	private:
		void updateSessitonActive(const std::string& uuid, const time_t& tmActive = 0);

	private:
		FuncSearchCb m_pfnSearchCb = nullptr;
		FuncGetPreviewCb m_pfnGetPreviewCb = nullptr;
		FuncGetFileListCb m_pfnGetGcodeListCb = nullptr;
		FuncGetFileListCb m_pfnGetHistoryListCb = nullptr;
		FuncGetFileListCb m_pfnGetVideoListCb = nullptr;
		FuncGetPrinterInfoCb m_pfnGetPrinterInfoCb = nullptr;
		FuncGetMaterialBoxCb m_pfnGetMaterialBoxCb = nullptr;

	private:
		bool m_bExit = false;
		QRegExp answerRegExp;
		std::mutex m_mutex;
		std::condition_variable m_condition;
		std::map<std::string, RemotePrinterSession> m_mapID2Printer;
		LanPrinterInterface* m_pLanPrinterInterface;
		OctoPrintInterface* m_pOctoPrinterInterface;
		KlipperInterface* m_pKlipperInterface;
		Klipper4408Interface* m_pKlipper4408Interface;
		std::mutex m_mtxDownload;
		std::list<std::tuple<std::string, std::string, std::function<void(const float&)> > > m_listDownloads;
		std::mutex m_mtxUpload;
		std::list<std::tuple<RemotePrinter,std::string, std::string,std::function<void(std::string, std::string, float)>,std::function<void(std::string, std::string, int)>> > m_listUploads;
		
	};
	SINGLETON_EXPORT(BASIC_KERNEL_API, RemotePrinterManager)
}

#endif // REMOTE_PRINTER_MANAGER_H


