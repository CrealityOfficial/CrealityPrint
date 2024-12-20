#include "LanPrinterInterface.h"
#include <QMetaType>
#include <unordered_map>
#include <cpr/cpr.h>
#include "rapidjson/document.h"
#include <iostream>
#include <math.h>
namespace creative_kernel
{
	LanPrinterInterface::LanPrinterInterface()
	{

	}

	LanPrinterInterface::~LanPrinterInterface()
	{

	}
	
	void LanPrinterInterface::getDeviceState(const std::string& strServerIp, std::function<void(std::unordered_map<std::string, std::string>, RemotePrinterSession)> callback, RemotePrinterSession printerSession)
	{
		std::string strUrl = "http://" + strServerIp + ":" + std::to_string(cServerPort) + cUrlSuffixState;
		cpr::GetCallback([](cpr::Response r) {
			return r.text;
			}, cpr::Url{ strUrl }, cpr::Timeout{ 1000 },
				cpr::Parameters{ {"fname", "net"},{"opt","iot_conf"},{"function","set"},{"ReqPrinterPara","0"} });

		auto future_text_set = cpr::GetCallback([](cpr::Response r) {
			return r.text;
			}, cpr::Url{ strUrl }, cpr::Timeout{ 1000 },
				cpr::Parameters{ {"fname", "net"},{"opt","iot_conf"},{"function","set"},{"ReqPrinterPara","1"} });

		if (future_text_set.wait_for(std::chrono::seconds(10)) == std::future_status::ready)
		{
			if (future_text_set.get().empty())
			{
				return;
			}

			auto future_text_get = cpr::GetCallback([=](cpr::Response r) {
				rapidjson::Document d;
				d.Parse(r.text.c_str());

				if (d.HasParseError())
					return;
				std::unordered_map<std::string, std::string> retKvs;
				for (rapidjson::Value::ConstMemberIterator itr = d.MemberBegin(); itr != d.MemberEnd(); itr++)
				{
					rapidjson::Value JsonName;
					rapidjson::Value JsonValue;
					rapidjson::Document::AllocatorType allocator;
					JsonName.CopyFrom(itr->name, allocator);
					JsonValue.CopyFrom(itr->value, allocator);
					if (JsonValue.IsString())
					{
						retKvs.insert(std::make_pair(JsonName.GetString(), JsonValue.GetString()));
					}
					else if (JsonValue.IsNumber())
					{
						retKvs.insert(std::make_pair(JsonName.GetString(), std::to_string(JsonValue.GetInt())));
					}
					else
					{
						printf("waring,none analyse type");
					}
				}
				if (callback != nullptr)
				{
					callback(retKvs, printerSession);
				}
				return ;
				}, cpr::Url{ strUrl },
					cpr::Parameters{ {"fname", "Info"},{"opt","main"},{"function","get"} });
		}
	}
	
	std::future<void> LanPrinterInterface::sendFileToDevice(const std::string& strIP, const std::string& fileName, const std::string& filePath, std::function<void(float)> callback, std::function<void(int)> errorCallback)
	{
		return cpr::FtpPutCallback(fileName, filePath, [=](cpr::Response r) {
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
			cpr::Url{ "ftp://" + strIP + "/mmcblk0p1/creality/gztemp/"},
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
	
	std::list<std::string> LanPrinterInterface::getFileListFromDevice(const std::string& strIP, std::function<void(std::string)> callback)
	{
		std::list<std::string> fileList;
		cpr::FtpListFileAsync
		(
			"ftp://" + strIP + "/mmcblk0p1/creality/gztemp/", 
			cpr::WriteCallback([=](std::string data, intptr_t userdata) {
				if (callback != nullptr)
				{
					callback(data);
				}
				return true;
			})
		);
		return fileList;
	}

	void LanPrinterInterface::getFileList(const std::string& strIp)
	{
	}
	
	int LanPrinterInterface::controlPrinter(const std::string& strServerIp, const PrintControlType& cmdType, const std::string& value, std::function<void()> callback)
	{
		std::unordered_map<std::string, std::string> retKvs;
		std::string strUrl = "http://" + strServerIp + ":" + std::to_string(cServerPort) + cUrlSuffixState;
		cpr::Parameters parameters = cpr::Parameters{ {"fname", "net"},{"opt","iot_conf"},{"function","set"} };

		switch (cmdType)
		{
		case PrintControlType::PRINT_START:
		{
			std::string filepath = "/media/mmcblk0p1/creality/gztemp/" + value;
			parameters.Add(cpr::Parameter{ "print", filepath });
			break;
		}
		case PrintControlType::PRINT_PAUSE:
		{
			parameters.Add(cpr::Parameter{ "pause", "1" });
			break;
		}
		case PrintControlType::PRINT_CONTINUE:
		{
			parameters.Add(cpr::Parameter{ "pause", "0" });
			break;
		}
		case PrintControlType::PRINT_STOP:
		{
			parameters.Add(cpr::Parameter{ "stop", "1" });
			break;
		}
		case PrintControlType::CONTROL_MOVE_X:
		{
			parameters.Add(cpr::Parameter{ "setPosition", "X" + value });
			break;
		}
		case PrintControlType::CONTROL_MOVE_Y:
		{
			parameters.Add(cpr::Parameter{ "setPosition", "Y" + value });
			break;
		}
		case PrintControlType::CONTROL_MOVE_Z:
		{
			parameters.Add(cpr::Parameter{ "setPosition", "Z" + value });
			break;
		}
		case PrintControlType::CONTROL_MOVE_XY0:
		{
			parameters.Add(cpr::Parameter{ "gcodeCmd", "G28 X0 Y0" });
			break;
		}
		case PrintControlType::CONTROL_MOVE_Z0:
		{
			parameters.Add(cpr::Parameter{ "gcodeCmd", "G28 Z0" });
			break;
		}
		case PrintControlType::CONTROL_CMD_FAN:
		{
			parameters.Add(cpr::Parameter{ "fan", value });
			break;
		}
		case PrintControlType::CONTROL_CMD_BEDTEMP:
		{
			parameters.Add(cpr::Parameter{ "bedTemp2", value });
			break;
		}
		case PrintControlType::CONTROL_CMD_NOZZLETEMP:
		{
			parameters.Add(cpr::Parameter{ "nozzleTemp2", value });
			break;
		}
		case PrintControlType::CONTROL_CMD_FEEDRATEPCT:
		{
			parameters.Add(cpr::Parameter{ "setFeedratePct", value });
			break;
		}
		case PrintControlType::CONTROL_CMD_AUTOHOME:
		{
			parameters.Add(cpr::Parameter{ "autohome", "0" });
			break;
		}
		case PrintControlType::CONTROL_CMD_LED:
		{
			parameters.Add(cpr::Parameter{ "led", value });
			break;
		}
		case PrintControlType::CONTROL_CMD_1STNOZZLETEMP:
		{
			parameters.Add(cpr::Parameter{ "_1st_nozzleTemp2", value });
			break;
		}
		case PrintControlType::CONTROL_CMD_2NDNOZZLETEMP:
		{
			parameters.Add(cpr::Parameter{ "_2nd_nozzleTemp2", value });
			break;
		}
		case PrintControlType::CONTROL_CMD_CHAMBERTEMP:
		{
			parameters.Add(cpr::Parameter{ "chamberTemp2", value });
			break;
		}
		default:
			break;
		}

		auto r = cpr::GetCallback([=](cpr::Response r) {
			if (callback)
			{
				callback();
			}
		}, 
			cpr::Url{ strUrl }, parameters
		);
		return 0;
	}
	void LanPrinterInterface::transparentCommand(const std::string& strServerIp, const std::string& value, std::function<void()> callback)
	{
		std::unordered_map<std::string, std::string> retKvs;
		std::string strUrl = "http://" + strServerIp + ":" + std::to_string(cServerPort) + cUrlSuffixState;
		cpr::Parameters parameters = cpr::Parameters{ {"fname", "net"},{"opt","iot_conf"},{"function","set"} };

		auto r = cpr::GetCallback([=](cpr::Response r) {
			if (callback)
			{
				callback();
			}
			}, cpr::Url{ strUrl }, parameters);
	}

	void LanPrinterInterface::fetchHead(const std::string& strIp, std::string filePath, std::function<void(std::string, std::string)> callback)
	{
		std::string fullPath = "ftp://" + strIp + "/mmcblk0p1/creality/gztemp/" + filePath;
		cpr::FtpGetCallback("0-81",
			[=](cpr::Response r) {
				if (r.error.code != cpr::ErrorCode::OK)
				{
					if (callback)
					{
						callback("", r.error.message);
					}
				}
			}, cpr::Url{ fullPath },
			cpr::WriteCallback(std::bind(&LanPrinterInterface::fetchHeadCallback, this, std::placeholders::_1, std::placeholders::_2, fullPath, callback)));
	}

	bool LanPrinterInterface::fetchHeadCallback(std::string data, intptr_t userdata, std::string fullPath, std::function<void(std::string, std::string)> callback)
	{
		auto filetype = data.substr(2, 3);
		if (filetype == "jpg" || filetype == "bmp" || filetype == "png")
		{
			auto index = data.find_first_of("\n");
			QString temp = data.substr(0, index).c_str();
			auto content = temp.split(' ');
			int size = 0;
			if (temp.contains('*'))
			{
				size = content[4].toInt();
			}
			else
			{
				size = content[3].toInt();
			}
			size = ceil((double)size / 3.0) * 4;
			int lines = ceil(size /76.0);
			size = size + lines*4;
			std::string range = std::to_string(index+1) + "-" + std::to_string(size+1);
			fetchBody(fullPath, range, callback, filetype, size-index);
		}
		return true;
	}

	void LanPrinterInterface::fetchBody(const std::string& fullPath, std::string range, std::function<void(std::string, std::string)> callback, std::string imageType, int size)
	{
		std::string* buffer = new std::string();
		cpr::FtpGetCallback(range,
			[=](cpr::Response r) {
				if (r.error.code != cpr::ErrorCode::OK)
				{
					if (callback)
					{
						callback("", r.error.message);
					}
				}
			}, cpr::Url{ fullPath },
				cpr::WriteCallback(std::bind(&LanPrinterInterface::fetchBodyCallback, this, std::placeholders::_1, std::placeholders::_2, callback, imageType, size), (intptr_t)buffer));
	}

	bool LanPrinterInterface::fetchBodyCallback(std::string data, intptr_t userdata, std::function<void(std::string, std::string)> callback, std::string imageType, int size)
	{
		std::string* buffer = (std::string*)userdata;
		buffer->append(data);
		if (buffer->size() < size)
		{
			return true;
		}
		if (callback)
		{
			callback(imageType, *buffer);
			delete buffer;
		}
		return true;
	}
	void LanPrinterInterface::deleteFile(const std::string& strIp, std::string filePath)
	{
		std::string fullPath = "ftp://" + strIp + "/mmcblk0p1/creality/gztemp/" + filePath;
		cpr::FtpDeleteAsync(fullPath, cpr::Url{ fullPath });
	}
}
