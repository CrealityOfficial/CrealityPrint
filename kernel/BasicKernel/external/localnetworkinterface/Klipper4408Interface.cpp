#include "Klipper4408Interface.h"
#include "WsClient.h"

#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"

#include <cmath>
#include <cpr/cpr.h>
#include <unordered_map>

namespace creative_kernel
{
	Klipper4408Interface::Klipper4408Interface()
	{
		qRegisterMetaType<std::string>("std::string");
	}

	Klipper4408Interface::~Klipper4408Interface()
	{
		m_webSockets.clear();
	}

	void Klipper4408Interface::addClient(const std::string& strServerIp, const std::string& strMac, int port, std::function<void(const RemotePrinter&)> infoCallback, std::function<void(const std::string&, const std::string&)> fileCallback, std::function<void(std::string, std::string)> historyFileListCb, std::function<void(const std::string&, const std::string&)> videoCallback)
	{
		if (m_webSockets.find(strServerIp) == m_webSockets.end())
		{
			if (m_ioc == nullptr)
			{
				m_ioc = new net::io_context();
			}
			auto webSocket = std::make_shared<WsClient>(*m_ioc, strServerIp, strMac, port);
			webSocket->start();
			webSocket->infoCallback = infoCallback;
			webSocket->fileCallback = fileCallback;
			webSocket->historyFileListCb = historyFileListCb;
			webSocket->videoCallback = videoCallback;
			if (m_webSockets.empty())
			{
				//auto t = std::thread(std::bind(&boost::asio::io_context::run, m_ioc));
				auto t = std::thread([&]() {
					if (m_ioc->stopped())
					{
						m_ioc->reset();
					}
					m_ioc->run();
					});
				t.detach();
			}
			m_webSockets[strServerIp] = webSocket.get();
		}
	}

	void Klipper4408Interface::removeClient(const std::string& strServerIp)
	{
		if (m_webSockets.find(strServerIp) != m_webSockets.end())
		{
			m_webSockets[strServerIp]->stop([=]() {
				m_webSockets.erase(strServerIp);
				}
			);
		}
	}

	void Klipper4408Interface::sendFileToDevice(const std::string& strServerIp, const int& port, const std::string& fileName, const std::string& filePath, std::function<void(float)> callback, std::function<void(int)> errorCallback)
	{
		std::string strUrl = "http://" + strServerIp + ":" + std::to_string(port) + cUrlSuffixUpload;

		std::string realFileName = fileName.empty() ? filePath.substr(filePath.find_last_of('/') + 1) : fileName;

		cpr::Session session;
		session.SetUrl(strUrl);
		session.SetParameters(cpr::Parameters{ {"filename", realFileName} });

		auto fullRequestUrl = session.GetFullRequestUrl();
		strUrl += fullRequestUrl.substr(fullRequestUrl.find_last_of('=') + 1);

		cpr::PostCallback([=](cpr::Response r) {
			if (r.error.code != cpr::ErrorCode::OK)
			{
				if (errorCallback)
				{
					errorCallback(int(r.error.code));
				}
			}
			else
			{
				if (callback)
				{
					callback(1);
				}
			}
		}, 
			cpr::Url{ strUrl },
			cpr::Multipart{ {"file", realFileName, cpr::File(filePath) } },
			cpr::Header{ {"Content-Type", "multipart/form-data"} },
			cpr::ProgressCallback([=](cpr::cpr_off_t downloadTotal, cpr::cpr_off_t downloadNow, cpr::cpr_off_t uploadTotal, cpr::cpr_off_t uploadNow, intptr_t userdata) {
				if (uploadTotal && uploadNow)
				{
					if (callback != nullptr)
					{
						uploadNow == uploadTotal ? callback(0.99f) : callback((float)uploadNow / uploadTotal);
					}
				}
				return true;
			})
		);
	}

	void Klipper4408Interface::getHistoryListFromDevice(const std::string& strServerIp, std::function<void(std::string, std::string)> callback)
	{
		rapidjson::Document doc; doc.SetObject();
		rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

		doc.AddMember("method", "get", allocator);
		rapidjson::Value paramObject(rapidjson::kObjectType);
		paramObject.AddMember("reqHistory", 1, allocator);
		doc.AddMember("params", paramObject, allocator);

		rapidjson::StringBuffer strbuf;
		rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
		doc.Accept(writer);

		std::string json(strbuf.GetString(), strbuf.GetSize());

		if (m_webSockets.find(strServerIp) != m_webSockets.end())
		{
			m_pfnHistoryFileListCb = callback;
			m_webSockets[strServerIp]->sendTextMessage(json);
		}
	}

	void Klipper4408Interface::getPrinterInfo(const std::string& strServerIp, int port, std::function<void(std::string, std::string)> callback)
	{
		std::string strUrl = "http://" + strServerIp + ":" + std::to_string(port) + cUrlSuffixGetPrinterInfo;

		cpr::GetCallback([=](cpr::Response r) {
			auto ret = r.text;
			if (ret.size() > 0)
			{
				rapidjson::Document doc;
				doc.Parse(ret.c_str());
				if (doc.HasParseError()) return;

				if (doc.HasMember("mac") && doc["mac"].IsString())
				{
					std::string mac = doc["mac"].GetString();
					if (doc.HasMember("model") && doc["model"].IsString())
					{
						std::string model = doc["model"].GetString();
						if (callback)
						{
							callback(mac, model);
						}
					}	
				}
			}
		},
			cpr::Url{ strUrl },
			cpr::Payload{},
			cpr::Header{}
		);
	}

	void Klipper4408Interface::slotRemoveClient(const std::string& ipAddrClient)
	{
		std::string key = ipAddrClient;
		if (m_webSockets.find(key) != m_webSockets.end())
		{
			delete m_webSockets[key];
			m_webSockets.erase(key);
		}
	}

	void Klipper4408Interface::slotRetDeviceState(const RemotePrinter& printer)
	{
		if (m_pfnDeviceStateCb) m_pfnDeviceStateCb(printer);
	}

	void Klipper4408Interface::slotRetGcodeFileList(const std::string& ipAddrClient, const std::string& fileList)
	{
		if (m_pfnGcodeFileListCb) m_pfnGcodeFileListCb(ipAddrClient, fileList);
	}

	void Klipper4408Interface::slotRetHistoryFileList(const std::string& ipAddrClient, const std::string& fileList)
	{
		if (m_pfnHistoryFileListCb) m_pfnHistoryFileListCb(ipAddrClient, fileList);
	}

	void Klipper4408Interface::controlPrinter(const std::string& strServerIp, const int& port, const PrintControlType& cmdType, const std::string& value)
	{
		rapidjson::Document doc; doc.SetObject();
		rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

		doc.AddMember("method", "set", allocator);
		rapidjson::Value paramObject(rapidjson::kObjectType);
		auto paramValue = rapidjson::Value(value.c_str(), allocator);

		switch (cmdType)
		{
			case PrintControlType::PRINT_START:
			{
				std::string filepath = "/usr/data/printer_data/gcodes/" + value;
				auto paramValue = rapidjson::Value(("printprt:" + filepath).c_str(), allocator);
				paramObject.AddMember("opGcodeFile", paramValue, allocator);
				paramObject.AddMember("enableSelfTest", 0, allocator);
				break;
			}
			case PrintControlType::PRINT_WITH_CALIBRATION:
			{
				std::string filepath = "/usr/data/printer_data/gcodes/" + value;
				auto paramValue = rapidjson::Value(("printprt:" + filepath).c_str(), allocator);
				paramObject.AddMember("opGcodeFile", paramValue, allocator);
				paramObject.AddMember("enableSelfTest", 1, allocator);
				break;
			}
			case PrintControlType::PRINT_CONTINUE:
			{
				paramObject.AddMember("pause", 0, allocator);
				break;
			}
			case PrintControlType::PRINT_PAUSE:
			{
				paramObject.AddMember("pause", 1, allocator);
				break;
			}
			case PrintControlType::PRINT_STOP:
			{
				paramObject.AddMember("stop", 1, allocator);
				break;
			}
			case PrintControlType::CONTROL_MOVE_X:
			case PrintControlType::CONTROL_MOVE_Y:
			case PrintControlType::CONTROL_MOVE_Z:
			{
				auto paramValue = rapidjson::Value(value.c_str(), allocator);
				paramObject.AddMember("setPosition", paramValue, allocator);
				break;
			}
			case PrintControlType::CONTROL_MOVE_XY0:
			{
				paramObject.AddMember("gcodeCmd", "G28 X0 Y0", allocator);
				break;
			}
			case PrintControlType::CONTROL_MOVE_Z0:
			{
				paramObject.AddMember("gcodeCmd", "G28 Z0", allocator);
				break;
			}
			case PrintControlType::CONTROL_CMD_FAN:
			{
				paramObject.AddMember("fan", atoi(value.c_str()), allocator);
				break;
			}
			case PrintControlType::CONTROL_CMD_BEDTEMP:
			{
				rapidjson::Value bedTempObject(rapidjson::kObjectType);
				bedTempObject.AddMember("num", 0, allocator);
				bedTempObject.AddMember("val", atoi(value.c_str()), allocator);

				paramObject.AddMember("bedTempControl", bedTempObject, allocator);
				break;
			}
			case PrintControlType::CONTROL_CMD_NOZZLETEMP:
			{
				paramObject.AddMember("nozzleTempControl", atoi(value.c_str()), allocator);
				break;
			}
			case PrintControlType::CONTROL_CMD_FEEDRATEPCT:
			{
				paramObject.AddMember("setFeedratePct", atoi(value.c_str()), allocator);
				break;
			}
			case PrintControlType::CONTROL_CMD_AUTOHOME:
			{
				paramObject.AddMember("autohome", "0", allocator);
				break;
			}
			case PrintControlType::CONTROL_CMD_LED:
			{
				paramObject.AddMember("lightSw", atoi(value.c_str()), allocator);
				break;
			}
			case PrintControlType::CONTROL_CMD_FANCASE:
			{
				paramObject.AddMember("fanCase", atoi(value.c_str()), allocator);
				break;
			}
			case PrintControlType::CONTROL_CMD_FANAUXILIARY:
			{
				paramObject.AddMember("fanAuxiliary", atoi(value.c_str()), allocator);
				break;
			}
			case PrintControlType::CONTROL_CMD_RESTART_KLIPPER:
			{
				paramObject.AddMember("restartKlipper", 1, allocator);
				break;
			}
			case PrintControlType::CONTROL_CMD_RESTART_FIRMWARE:
			{
				paramObject.AddMember("restartFirmware", 1, allocator);
				break;
			}
			case PrintControlType::CONTROL_CMD_REPOPLRSTATUS:
			{
				paramObject.AddMember("repoPlrStatus", atoi(value.c_str()), allocator);
				break;
			}
			case PrintControlType::CONTROL_CMD_CLEANERR:
			{
				paramObject.AddMember("cleanErr", 1, allocator);
				break;
			}
			case PrintControlType::CONTROL_CMD_MODELFANPCT:
			{
				double result = 255 * (atoi(value.c_str()) / 100.0);
				int roundValue = round(result);
				std::string gcodeCmd = "M106 S"+ std::to_string(roundValue);
				auto paramValue = rapidjson::Value(gcodeCmd.c_str(), allocator);
				paramObject.AddMember("gcodeCmd", paramValue, allocator);
				break;
			}
			case PrintControlType::CONTROL_CMD_CASEFANPCT:
			{
				double result = 255 * (atoi(value.c_str()) / 100.0);
				int roundValue = round(result);
				std::string gcodeCmd = "M106 P1 S" + std::to_string(roundValue);
				auto paramValue = rapidjson::Value(gcodeCmd.c_str(), allocator);
				paramObject.AddMember("gcodeCmd", paramValue, allocator);
				break;
			}
			case PrintControlType::CONTROL_CMD_AUXILIARYFANPCT:
			{
				double result = 255 * (atoi(value.c_str()) / 100.0);
				int roundValue = round(result);
				std::string gcodeCmd = "M106 P2 S" + std::to_string(roundValue);
				auto paramValue = rapidjson::Value(gcodeCmd.c_str(), allocator);
				paramObject.AddMember("gcodeCmd", paramValue, allocator);
				break;
			}
			case PrintControlType::CONTROL_CMD_EXCLUDEOBJECTS:
			{
				rapidjson::Value arrayValue(rapidjson::kArrayType);
				auto paramValue = rapidjson::Value(value.c_str(), allocator);
				arrayValue.PushBack(paramValue, allocator);

				paramObject.AddMember("excludeObjects", arrayValue, allocator);
				
				break;
			}
			default:

				break;
		}

		doc.AddMember("params", paramObject, allocator);
		rapidjson::StringBuffer strbuf;
		rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
		doc.Accept(writer);

		std::string json(strbuf.GetString(), strbuf.GetSize());

		if (m_webSockets.find(strServerIp) != m_webSockets.end())
		{
			m_webSockets[strServerIp]->sendTextMessage(json);
		}
	}

	void Klipper4408Interface::transparentCommand(const std::string& strServerIp, const int& port, const std::string& value)
	{

	}

	void Klipper4408Interface::deleteGcodeFile(const std::string& strIp, const std::string& filePath)
	{
		rapidjson::Document doc; doc.SetObject();
		rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

		doc.AddMember("method", "set", allocator);
		rapidjson::Value paramObject(rapidjson::kObjectType);

		std::string filepath = "/usr/data/printer_data/gcodes/" + filePath;
		auto paramValue = rapidjson::Value(("deleteprt:" + filepath).c_str(), allocator);
		paramObject.AddMember("opGcodeFile", paramValue, allocator);
		doc.AddMember("params", paramObject, allocator);

		rapidjson::StringBuffer strbuf;
		rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
		doc.Accept(writer);

		std::string json(strbuf.GetString(), strbuf.GetSize());

		if (m_webSockets.find(strIp) != m_webSockets.end())
		{
			m_webSockets[strIp]->sendTextMessage(json);
		}
	}

	void Klipper4408Interface::deleteHistoryFile(const std::string& strIp, int id)
	{
		rapidjson::Document doc; doc.SetObject();
		rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

		doc.AddMember("method", "set", allocator);
		rapidjson::Value deleteArray(rapidjson::kArrayType);
		rapidjson::Value paramObject(rapidjson::kObjectType);

		deleteArray.PushBack(id, allocator);
		paramObject.AddMember("deleteHistory", deleteArray, allocator);
		doc.AddMember("params", paramObject, allocator);

		rapidjson::StringBuffer strbuf;
		rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
		doc.Accept(writer);

		std::string json(strbuf.GetString(), strbuf.GetSize());

		if (m_webSockets.find(strIp) != m_webSockets.end())
		{
			m_webSockets[strIp]->sendTextMessage(json);
		}
	}

	void Klipper4408Interface::deleteVideoFile(const std::string& strIp, const std::string& filePath)
	{
		rapidjson::Document doc; doc.SetObject();
		rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

		doc.AddMember("method", "set", allocator);
		rapidjson::Value ctrlVideoFilesObject(rapidjson::kObjectType);
		rapidjson::Value paramObject(rapidjson::kObjectType);

		ctrlVideoFilesObject.AddMember("cmd", "remove", allocator);
		ctrlVideoFilesObject.AddMember("printId", "", allocator);
		std::string filepath = "\/usr\/data\/\/creality\/userdata\/delay_image\/video\/" + filePath;
		auto paramValue = rapidjson::Value(filepath.c_str(), allocator);
		ctrlVideoFilesObject.AddMember("file", paramValue, allocator);
		paramObject.AddMember("ctrlVideoFiles", ctrlVideoFilesObject, allocator);
		doc.AddMember("params", paramObject, allocator);

		rapidjson::StringBuffer strbuf;
		rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
		doc.Accept(writer);

		std::string json(strbuf.GetString(), strbuf.GetSize());

		if (m_webSockets.find(strIp) != m_webSockets.end())
		{
			m_webSockets[strIp]->sendTextMessage(json);
		}
	}
	void Klipper4408Interface::renameVideoFile(const std::string& strIp, const std::string& filePath, const std::string& targetName)
	{
		rapidjson::Document doc; doc.SetObject();
		rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

		doc.AddMember("method", "set", allocator);
		rapidjson::Value ctrlVideoFilesObject(rapidjson::kObjectType);
		rapidjson::Value paramObject(rapidjson::kObjectType);

		ctrlVideoFilesObject.AddMember("cmd", "rename", allocator);
		ctrlVideoFilesObject.AddMember("printId", "", allocator);
		std::string filepath = "\/usr\/data\/\/creality\/userdata\/delay_image\/video\/" + filePath;
		ctrlVideoFilesObject.AddMember("file", rapidjson::Value(filepath.c_str(), allocator), allocator);
		ctrlVideoFilesObject.AddMember("targetname", rapidjson::Value(targetName.c_str(), allocator), allocator);
		paramObject.AddMember("ctrlVideoFiles", ctrlVideoFilesObject, allocator);
		doc.AddMember("params", paramObject, allocator);

		rapidjson::StringBuffer strbuf;
		rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
		doc.Accept(writer);

		std::string json(strbuf.GetString(), strbuf.GetSize());

		if (m_webSockets.find(strIp) != m_webSockets.end())
		{
			m_webSockets[strIp]->sendTextMessage(json);
		}
	}

	void Klipper4408Interface::renameGcodeFile(const std::string& strIp, const std::string& filePath, const std::string& targetName)
	{
		rapidjson::Document doc; doc.SetObject();
		rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

		doc.AddMember("method", "set", allocator);
		rapidjson::Value paramObject(rapidjson::kObjectType);

		std::string filepath = "/usr/data/printer_data/gcodes/" + filePath;
		std::string targetPath = "/usr/data/printer_data/gcodes/" + targetName;
		auto paramValue = rapidjson::Value(("renameprt:" + filepath + ":" + targetPath).c_str(), allocator);
		paramObject.AddMember("opGcodeFile", paramValue, allocator);
		doc.AddMember("params", paramObject, allocator);

		rapidjson::StringBuffer strbuf;
		rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
		doc.Accept(writer);

		std::string json(strbuf.GetString(), strbuf.GetSize());

		if (m_webSockets.find(strIp) != m_webSockets.end())
		{
			m_webSockets[strIp]->sendTextMessage(json);
		}
	}
}
