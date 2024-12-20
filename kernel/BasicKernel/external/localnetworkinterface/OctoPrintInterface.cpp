#include "OctoPrintInterface.h"
#include <cpr/cpr.h>
#include<unordered_map>

namespace creative_kernel
{
	OctoPrintInterface::OctoPrintInterface()
	{

	}

	OctoPrintInterface::~OctoPrintInterface()
	{

	}
	
	void OctoPrintInterface::getDeviceState(const std::string& strServerIp, const std::string& strToken, std::function<void(const std::string&&)> callback)
	{
		std::string strUrl = "http://" + strServerIp + ":" + std::to_string(cServerPort) + cUrlSuffixState;

		auto future_text = cpr::GetCallback([](cpr::Response r) {
			return r.text;
			}, cpr::Url{ strUrl },
				cpr::Header{ {"X-Api-Key:", strToken} });

		if (future_text.wait_for(std::chrono::seconds(10)) == std::future_status::ready)
		{
			auto ret = future_text.get();
			if (ret.empty())
			{
				return;
			}

			if (callback != nullptr)
			{
				callback(std::move(ret));
			}
		}
	}
	
	void OctoPrintInterface::sendFileToDevice(const std::string& strServerIp, const std::string& strToken, const std::string& filePath)
	{
		std::string strUrl = "http://" + strServerIp + ":" + std::to_string(cServerPort) + cUrlSuffixFile;

		auto future_text = cpr::PostCallback([](cpr::Response r) {
			return r.text;
			}, cpr::Url{ strUrl },
				cpr::Multipart{ {"file", cpr::File{filePath}},{"filename", filePath} },
				cpr::Header{ {"X-Api-Key:", strToken},{"Content-Type", "application/octet-stream"} });

		if (future_text.wait_for(std::chrono::seconds(10)) == std::future_status::ready)
		{
			auto ret = future_text.get();
			if (ret.empty())
			{
				return;
			}
		}
	}
	
	void OctoPrintInterface::getFileListFromDevice(const std::string& strIP)
	{
	}

	void OctoPrintInterface::getFileList(const std::string& strIp)
	{
	}

	void OctoPrintInterface::controlPrinter(const std::string& strServerIp, const std::string& strToken, const PrintControlType& cmdType, const std::string& value)
	{
		std::unordered_map<std::string, std::string> retKvs;
		std::string strUrl = "http://" + strServerIp + ":" + std::to_string(cServerPort);
		cpr::Payload payload = cpr::Payload{};
		switch (cmdType)
		{
		case PrintControlType::PRINT_START:
		{
			strUrl += cUrlSuffixJob;
			payload.Add({"command", "start"});
			break;
		}
		case PrintControlType::PRINT_PAUSE:
		{
			strUrl += cUrlSuffixJob;
			payload.Add({ "command", "pause" });
			break;
		}
		case PrintControlType::PRINT_CONTINUE:
		{
			strUrl += cUrlSuffixJob;
			payload.Add({ "command", "pause" });
			payload.Add({ "action", "pause" });
			break;
		}
		case PrintControlType::PRINT_STOP:
		{
			strUrl += cUrlSuffixJob;
			payload.Add({ "command", "cancel" });
			break;
		}
		case PrintControlType::CONTROL_CMD_BEDTEMP:
		{
			strUrl += cUrlSuffixJob;
			payload.Add({ "command", "target" });
			payload.Add({ "target", value });
			break;
		}
		default:
			break;
		}

		auto future_text = cpr::PostCallback([](cpr::Response r) {
			return r.text;
			}, cpr::Url{ strUrl },
				payload,
				cpr::Header{ {"X-Api-Key:", strToken},{"Content-Type", "application/json"} });

		if (future_text.wait_for(std::chrono::seconds(10)) == std::future_status::ready)
		{
			auto ret = future_text.get();
			if (ret.empty())
			{
				return;
			}
		}
	}
	
	void OctoPrintInterface::transparentCommand(const std::string& strServerIp, const std::string& strToken, const std::string& value)
	{
		std::string strUrl = "http://" + strServerIp + ":" + std::to_string(cServerPort) + cUrlSuffixState;

		auto future_text = cpr::PostCallback([](cpr::Response r) {
			return r.text;
			}, cpr::Url{ strUrl },
				cpr::Payload{ {"command", value} },
				cpr::Header{ {"X-Api-Key:", strToken},{"Content-Type", "application/json"} });

		if (future_text.wait_for(std::chrono::seconds(10)) == std::future_status::ready)
		{
			auto ret = future_text.get();
			if (ret.empty())
			{
				return;
			}
		}
	}
}
