#include <cpr/cpr.h>
#include "KlipperInterface.h"
#include "rapidjson/document.h"

namespace creative_kernel
{
	KlipperInterface::KlipperInterface()
	{

	}

	KlipperInterface::~KlipperInterface()
	{

	}
	
	void KlipperInterface::getDeviceState(const std::string& strServerIp, const int& port, std::function<void(const std::string&, RemotePrinterSession)> callback, RemotePrinterSession printerSession)
	{
		std::string strUrl = "http://" + strServerIp + ":" + std::to_string(port) + cUrlSuffixState;

		cpr::Payload payload = cpr::Payload{};
		payload.Add({ "toolhead", "position" });
		payload.Add({ "extruder", "target,temperature" });
		payload.Add({ "heater_bed", "target,temperature" });
		payload.Add({ "fan", "speed" });
		payload.Add({ "print_stats", "filename,state,print_duration" });
		payload.Add({ "display_status", "progress" });
		payload.Add({ "gcode_move", "speed_factor" });

		cpr::GetCallback([=](cpr::Response r) {
			if (callback)
			{
				callback(r.text, printerSession);
			}
			}, cpr::Url{ strUrl }, cpr::Timeout{ 1000 },
				payload,
				cpr::Header{});
	}

	void KlipperInterface::getSystemInfo(const std::string& strServerIp, const int& port, std::function<void(const std::string&, RemotePrinterSession)> callback, RemotePrinterSession printerSession)
	{
		std::string strUrl = "http://" + strServerIp + ":" + std::to_string(port) + cUrlSuffixSystemInfo;

		cpr::GetCallback([=](cpr::Response r) {
			if (callback)
			{
				callback(r.text, printerSession);
			}
		}, cpr::Timeout{ 1000 },
			cpr::Url{ strUrl },
			cpr::Payload{},
			cpr::Header{}
		);
	}
	
	void KlipperInterface::sendFileToDevice(const std::string& strServerIp, const int& port, std::string fileName, std::string filePath, std::function<void(float)> callback, std::function<void(int)> errorCallback)
	{
		std::string strUrl = "http://" + strServerIp + ":" + std::to_string(port) + cUrlSuffixUpload;

		std::string realFileName = fileName.empty() ? filePath.substr(filePath.find_last_of('/') + 1) : fileName;

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
	
	void KlipperInterface::getFileListFromDevice(const std::string& strIP)
	{
	}

	void KlipperInterface::getFileList(const std::string& strIp)
	{
	}

	void KlipperInterface::controlPrinter(const std::string& strServerIp, const int& port, const PrintControlType& cmdType, const std::string& value)
	{
		std::string strUrl = "http://" + strServerIp + ":" + std::to_string(port);
		cpr::Payload payload = cpr::Payload{};
		std::string fullRequestUrl;
		switch (cmdType)
		{
		case PrintControlType::PRINT_START:
		{
			strUrl += cUrlSuffixJob;

			cpr::Session session;
			session.SetUrl(strUrl);
			session.SetParameters(cpr::Parameters{ {"filename", value} });

			fullRequestUrl = session.GetFullRequestUrl();

			strUrl = strUrl + "filename=" + value;
			break;
		}
		case PrintControlType::PRINT_PAUSE:
		{
			break;
		}
		case PrintControlType::PRINT_CONTINUE:
		{
			break;
		}
		case PrintControlType::PRINT_STOP:
		{
			break;
		}
		case PrintControlType::CONTROL_CMD_BEDTEMP:
		{
			break;
		}
		default:
			break;
		}

		cpr::PostCallback([](cpr::Response r) {
			return r.text;
			}, cpr::Url{ fullRequestUrl },
				cpr::Payload{},
				cpr::Header{});
	}
	
	void KlipperInterface::transparentCommand(const std::string& strServerIp, const int& port, const std::string& value)
	{
	}

	void KlipperInterface::getMultiMachine(const std::string& strServerIp, const int& port, std::function<void(std::vector<RemotePrinter>&)> callback)
	{
		std::string strUrl = "http://" + strServerIp + ":" + std::to_string(port) + cUrlSuffixGetMultiMachine;

		cpr::GetCallback([=](cpr::Response r) {
			auto ret = r.text;
			if (ret.size() > 0)
			{
				rapidjson::Document doc;
				doc.Parse(ret.c_str());
				if (doc.HasParseError()) return;

				std::vector<RemotePrinter> printers;
				if (doc.HasMember("result") && doc["result"].IsObject())
				{
					const rapidjson::Value& objectResult = doc["result"];
					if (objectResult.HasMember("multi_printer_info") && objectResult["multi_printer_info"].IsArray())
					{
						const auto& array = objectResult["multi_printer_info"];
						for (size_t i = 0; i < array.Size(); i++)
						{
							RemotePrinter printer;
							printer.type = RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER;
							const auto& modelInfo = array[i];
							if (modelInfo.HasMember("ip") && modelInfo["ip"].IsString())
							{
								printer.ipAddress = modelInfo["ip"].GetString();
							}
							if (modelInfo.HasMember("machine_name") && modelInfo["machine_name"].IsString())
							{
								printer.deviceName = modelInfo["machine_name"].GetString(); 
							}
							if (modelInfo.HasMember("machine_type") && modelInfo["machine_type"].IsString())
							{
								printer.printerName = modelInfo["machine_type"].GetString();
							}
							if (modelInfo.HasMember("moonraker_port") && modelInfo["moonraker_port"].IsInt())
							{
								printer.moonrakerPort = modelInfo["moonraker_port"].GetInt();
							}
							if (modelInfo.HasMember("printer_id") && modelInfo["printer_id"].IsInt())
							{
								printer.printerId = modelInfo["printer_id"].GetInt();
							}
							if (modelInfo.HasMember("fluidd_port") && modelInfo["fluidd_port"].IsInt())
							{
								printer.fluiddPort = modelInfo["fluidd_port"].GetInt();
							}
							if (modelInfo.HasMember("mainsail_port") && modelInfo["mainsail_port"].IsInt())
							{
								printer.mainsailPort = modelInfo["mainsail_port"].GetInt();
							}
							printer.uniqueId = printer.ipAddress + "_" + std::to_string(printer.printerId).c_str();
							printers.push_back(printer);
						}
					}
				}
				if (callback)
				{
					callback(printers);
				}
			}
		}, 
			cpr::Url{ strUrl }, cpr::Timeout{ 1000 },
			cpr::Payload{},
			cpr::Header{}
		);
	}

}
