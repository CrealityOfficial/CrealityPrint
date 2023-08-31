#include "WsClient.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"

#include <cpr/cpr.h>
#include <unordered_map>

namespace creative_kernel
{
	WsClient::WsClient(net::io_context& ioc, const std::string& ip, const int& port) : m_resolver(net::make_strand(ioc)), m_serverIp(ip), m_serverPort(port)
	{
		m_ws = std::make_unique<websocket::stream<beast::tcp_stream>>(net::make_strand(ioc));
		m_printer.ipAddress = QString::fromStdString(ip);
		m_printer.macAddress = QString::fromStdString(ip);
		m_printer.fluiddPort = 8888; m_printer.mainsailPort = 8888;
		m_printer.printMode = 1; m_printer.type = RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408;
	}

	void WsClient::start()
	{
		m_resolver.async_resolve(m_serverIp, std::to_string(m_serverPort), beast::bind_front_handler(&WsClient::onResolve, shared_from_this()));
	}

	void WsClient::stop()
	{
		needStop = true;
		// Close the WebSocket connection
		m_ws->async_close(websocket::close_code::normal, beast::bind_front_handler(&WsClient::onClose, shared_from_this()));
	}

	void WsClient::sendTextMessage(const std::string& message)
	{
		m_writeQueue.push_back(message);

		if (m_writeQueue.size() > 1) return;

		m_ws->async_write(net::buffer(m_writeQueue.front()), beast::bind_front_handler(&WsClient::onWrite, shared_from_this()));
	}

	void WsClient::onResolve(beast::error_code ec, tcp::resolver::results_type results)
	{
		if (ec) return onFail(ec, "resolve");

		beast::get_lowest_layer(*m_ws.get()).expires_after(std::chrono::seconds(30));

		beast::get_lowest_layer(*m_ws.get()).async_connect(results, beast::bind_front_handler(&WsClient::onConnect, shared_from_this()));
	}

	void WsClient::onConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep)
	{
		if (ec) return onFail(ec, "connect");

		beast::get_lowest_layer(*m_ws.get()).expires_never();

		beast::websocket::stream_base::timeout opt{
		std::chrono::seconds(20),   // handshake timeout
		std::chrono::seconds(3),       // idle timeout. no data recv from peer for 6 sec, then it is timeout
		true   //enable ping-pong to keep alive
		};
		m_ws->set_option(opt);
		//m_ws->set_option(
		//	websocket::stream_base::timeout::suggested(
		//		beast::role_type::client));

		// Set a decorator to change the User-Agent of the handshake
		m_ws->set_option(websocket::stream_base::decorator(
			[](websocket::request_type& req)
			{
				req.set(http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async");
			}
		));

		// Perform the websocket handshake
		m_ws->async_handshake(m_serverIp + ':' + std::to_string(ep.port()), "/", beast::bind_front_handler(&WsClient::onHandshake, shared_from_this()));
	}

	void WsClient::onHandshake(beast::error_code ec)
	{
		if (ec) return onFail(ec, "handshake");

		rapidjson::Document doc; doc.SetObject();
		rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

		doc.AddMember("method", "get", allocator);
		rapidjson::Value paramObject(rapidjson::kObjectType);
		paramObject.AddMember("reqGcodeFile", 1, allocator);
		paramObject.AddMember("reqHistory", 1, allocator);
		paramObject.AddMember("reqElapseVideoList", 1, allocator);
		doc.AddMember("params", paramObject, allocator);

		rapidjson::StringBuffer strbuf;
		rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
		doc.Accept(writer);

		std::string json(strbuf.GetString(), strbuf.GetSize());
		// Write getfilelist payload
		m_ws->async_write(net::buffer(json), beast::bind_front_handler(&WsClient::onWriteGetfilelist, shared_from_this()));
	}

	void WsClient::onWriteGetfilelist(beast::error_code ec, std::size_t bytes_transferred)
	{
		// Read a message into our buffer
		m_ws->async_read(m_readBuffer, beast::bind_front_handler(&WsClient::onRead, shared_from_this()));
	}

	void WsClient::onRead(beast::error_code ec, std::size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		if (ec) return onFail(ec, "read");
		if (m_retryTimes)
		{
			m_retryTimes = 0;
		}
		//parse message
		auto msg = boost::beast::buffers_to_string(m_readBuffer.cdata());
		m_readBuffer.consume(m_readBuffer.size());
		ParseMsg(msg);

		m_ws->async_read(m_readBuffer, beast::bind_front_handler(&WsClient::onRead, shared_from_this()));
	}

	void WsClient::onClose(beast::error_code ec)
	{
		if (ec) return onFail(ec, "close");
	}

	void WsClient::onWrite(beast::error_code ec, std::size_t bytes_transferred)
	{
		if (ec) return onFail(ec, "write");

		m_writeQueue.erase(m_writeQueue.begin());
		if (m_writeQueue.empty())
		{
			return;
		}

		m_ws->async_write(net::buffer(m_writeQueue.front()), beast::bind_front_handler(&WsClient::onWrite, shared_from_this()));
	}

	void WsClient::onFail(beast::error_code ec, char const* what)
	{
		if (m_ws->is_open())
		{
			m_ws->close(websocket::close_code::normal);
		}
		if (needStop)
		{
			return;
		}
		++m_retryTimes;
		if (m_printer.printerStatus != 0)
		{
			m_printer.printerStatus = 0;
		}
		if (infoCallback && m_retryTimes > 2)
		{
			infoCallback(m_printer);
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));

		m_ws.reset(new websocket::stream<beast::tcp_stream>(m_ws->get_executor()));
		start();
	}

	void WsClient::ParseMsg(const std::string& message)
	{
		rapidjson::Document doc;
		doc.Parse(message.c_str());

		if (doc.HasParseError()) return;
		if (doc.HasMember("curFeedratePct") && doc["curFeedratePct"].IsInt())
		{
			m_printer.printSpeed = doc["curFeedratePct"].GetInt();
		}
		if (doc.HasMember("connect") && doc["connect"].IsInt())
		{
			m_printer.printerStatus = doc["connect"].GetInt();
		}
		if (doc.HasMember("video") && doc["video"].IsInt())
		{
			m_printer.video = doc["video"].GetInt();
		}
		if (doc.HasMember("tfCard") && doc["tfCard"].IsInt())
		{
			m_printer.TFCardStatus = doc["tfCard"].GetInt();
		}
		if (doc.HasMember("curPosition") && doc["curPosition"].IsString())
		{
			m_printer.curPosition = doc["curPosition"].GetString();
		}
		if (doc.HasMember("autohome") && doc["autohome"].IsString())
		{
			m_printer.autohome = doc["autohome"].GetString();
		}
		if (doc.HasMember("model") && doc["model"].IsString())
		{
			m_printer.printerName = doc["model"].GetString();
		}
		if (doc.HasMember("state") && doc["state"].IsInt())
		{
			m_printer.printState = doc["state"].GetInt();
		}
		if (doc.HasMember("fan") && doc["fan"].IsInt())
		{
			m_printer.fanSwitchState = doc["fan"].GetInt();
		}
		if (doc.HasMember("fanCase") && doc["fanCase"].IsInt())
		{
			m_printer.caseFanSwitchState = doc["fanCase"].GetInt();
		}
		if (doc.HasMember("fanAuxiliary") && doc["fanAuxiliary"].IsInt())
		{
			m_printer.auxiliaryFanSwitchState = doc["fanAuxiliary"].GetInt();
		}
		if (doc.HasMember("targetNozzleTemp") && doc["targetNozzleTemp"].IsInt())
		{
			m_printer.nozzleTempTarget = doc["targetNozzleTemp"].GetInt();
		}
		if (doc.HasMember("targetBedTemp0") && doc["targetBedTemp0"].IsInt())
		{
			m_printer.bedTempTarget = doc["targetBedTemp0"].GetInt();
		}
		if (doc.HasMember("printFileName") && doc["printFileName"].IsString())
		{
			std::string printFileName = doc["printFileName"].GetString();
			size_t last_of = printFileName.find_last_of("/");
			if (last_of != std::string::npos) printFileName = printFileName.substr(last_of + 1);
			m_printer.printFileName = QString::fromStdString(printFileName);
		}
		if (doc.HasMember("layer") && doc["layer"].IsInt())
		{
			m_printer.layer = doc["layer"].GetInt();
		}
		if (doc.HasMember("TotalLayer") && doc["TotalLayer"].IsInt())
		{
			m_printer.totalLayer = doc["TotalLayer"].GetInt();
		}
		if (doc.HasMember("boxTemp") && doc["boxTemp"].IsInt())
		{
			m_printer.chamberTemp = doc["boxTemp"].GetInt();
		}
		if (doc.HasMember("lightSw") && doc["lightSw"].IsInt())
		{
			m_printer.ledState = doc["lightSw"].GetInt();
		}
		if (doc.HasMember("nozzleTemp") && doc["nozzleTemp"].IsString())
		{
			m_printer.nozzleTemp = atof(doc["nozzleTemp"].GetString());
		}
		if (doc.HasMember("bedTemp0") && doc["bedTemp0"].IsString())
		{
			m_printer.bedTemp = atof(doc["bedTemp0"].GetString());
		}
		if (doc.HasMember("printProgress") && doc["printProgress"].IsInt())
		{
			m_printer.printProgress = doc["printProgress"].GetInt();
		}
		if (doc.HasMember("printJobTime") && doc["printJobTime"].IsInt())
		{
			m_printer.printJobTime = doc["printJobTime"].GetInt();
		}
		if (doc.HasMember("printLeftTime") && doc["printLeftTime"].IsInt())
		{
			m_printer.printLeftTime = doc["printLeftTime"].GetInt();
		}
		if (doc.HasMember("err") && doc["err"].IsObject())
		{
			auto errorObject = doc["err"].GetObj();
			if (errorObject.HasMember("errcode") && errorObject["errcode"].IsInt())
			{
				int errorKey = errorObject["key"].GetInt();
				int errorCode = errorObject["errcode"].GetInt();

				m_printer.errorKey = errorKey;
				m_printer.errorCode = errorCode;
			}
		}
		if (doc.HasMember("repoPlrStatus") && doc["repoPlrStatus"].IsInt())
		{
			m_printer.repoPlrStatus = doc["repoPlrStatus"].GetInt();
			if (m_printer.repoPlrStatus) m_printer.errorCode = 2000;
			else if (!m_printer.repoPlrStatus && m_printer.errorCode == 2000) m_printer.errorCode = 0;
		}
		if (doc.HasMember("materialStatus") && doc["materialStatus"].IsInt())
		{
			m_printer.materialStatus = doc["materialStatus"].GetInt();
			if (m_printer.materialStatus) m_printer.errorCode = 2001;
			else if (!m_printer.materialStatus && m_printer.errorCode == 2001) m_printer.errorCode = 0;
		}
		if (doc.HasMember("retGcodeFileInfo") && doc["retGcodeFileInfo"].IsObject())
		{
			auto retGcodeFileInfo = doc["retGcodeFileInfo"].GetObj();
			if (retGcodeFileInfo.HasMember("fileInfo") && retGcodeFileInfo["fileInfo"].IsString())
			{
				auto fileList = retGcodeFileInfo["fileInfo"].GetString();
				if (fileCallback)
				{
					fileCallback(m_printer.ipAddress.toStdString(), fileList);
				}
			}
		}
		if (doc.HasMember("historyList") && doc["historyList"].IsArray())
		{
			std::string fileList;
			auto historyList = doc["historyList"].GetArray();
			for (int i = 0; i < historyList.Size(); i++)
			{
				std::string retHistory;
				if (historyList[i].HasMember("id") && historyList[i]["id"].IsInt())
				{
					std::string id = std::to_string(historyList[i]["id"].GetInt());
					retHistory += id; retHistory += ":";
				}
				if (historyList[i].HasMember("filename") && historyList[i]["filename"].IsString())
				{
					std::string filename = historyList[i]["filename"].GetString();
					retHistory += filename; retHistory += ":";
				}
				if (historyList[i].HasMember("size") && historyList[i]["size"].IsInt())
				{
					std::string fileSize = std::to_string(historyList[i]["size"].GetInt());
					retHistory += fileSize; retHistory += ":";
				}
				if (historyList[i].HasMember("starttime") && historyList[i]["starttime"].IsInt64())
				{
					std::string startTime = std::to_string(historyList[i]["starttime"].GetInt64());
					retHistory += startTime; retHistory += ":";
				}
				if (historyList[i].HasMember("usagetime") && historyList[i]["usagetime"].IsInt())
				{
					std::string usageTime = std::to_string(historyList[i]["usagetime"].GetInt());
					retHistory += usageTime; retHistory += ":";
				}
				if (historyList[i].HasMember("usagematerial") && historyList[i]["usagematerial"].IsDouble())
				{
					std::string usageMaterial = std::to_string(historyList[i]["usagematerial"].GetDouble());
					retHistory += usageMaterial; retHistory += ":";
				}
				if (historyList[i].HasMember("thumbnail") && historyList[i]["thumbnail"].IsString())
				{
					std::string thumbnail = historyList[i]["thumbnail"].GetString();
					retHistory += thumbnail; retHistory += ":";
				}
				if (historyList[i].HasMember("printfinish") && historyList[i]["printfinish"].IsInt())
				{
					std::string printFinish = std::to_string(historyList[i]["printfinish"].GetInt());
					retHistory += printFinish; 
					retHistory += ";";
				}
				fileList += retHistory;
			}
			if (historyFileListCb)
			{
				historyFileListCb(m_printer.ipAddress.toStdString(), fileList);
			}
		}

		if (doc.HasMember("elapseVideoList") && doc["elapseVideoList"].IsArray())
		{
			std::string fileList;
			auto videoList = doc["elapseVideoList"].GetArray();
			for (int i = 0; i < videoList.Size(); i++)
			{
				std::string retVideo;
				if (videoList[i].HasMember("id") && videoList[i]["id"].IsInt())
				{
					std::string id = std::to_string(videoList[i]["id"].GetInt());
					retVideo += id; retVideo += ":";
				}
				if (videoList[i].HasMember("name") && videoList[i]["name"].IsString())
				{
					std::string filename = videoList[i]["name"].GetString();
					retVideo += filename; retVideo += ":";
				}
				if (videoList[i].HasMember("size") && videoList[i]["size"].IsInt())
				{
					std::string fileSize = std::to_string(videoList[i]["size"].GetInt());
					retVideo += fileSize; retVideo += ":";
				}
				if (videoList[i].HasMember("video") && videoList[i]["video"].IsString())
				{
					std::string video = videoList[i]["video"].GetString();
					retVideo += video; retVideo += ":";
				}
				if (videoList[i].HasMember("duration") && videoList[i]["duration"].IsInt())
				{
					std::string duration = std::to_string(videoList[i]["duration"].GetInt());
					retVideo += duration; retVideo += ":";
				}
				if (videoList[i].HasMember("cover") && videoList[i]["cover"].IsString())
				{
					std::string cover = videoList[i]["cover"].GetString();
					retVideo += cover; retVideo += ":";
				}
				if (videoList[i].HasMember("starttime") && videoList[i]["starttime"].IsInt64())
				{
					std::string startTime = std::to_string(videoList[i]["starttime"].GetInt64());
					retVideo += startTime; retVideo += ":";
				}
				if (videoList[i].HasMember("printtime") && videoList[i]["printtime"].IsInt())
				{
					std::string printtime = std::to_string(videoList[i]["printtime"].GetInt());
					retVideo += printtime; retVideo += ":";
				}
				if (videoList[i].HasMember("videoname") && videoList[i]["videoname"].IsString())
				{
					std::string printtime = videoList[i]["videoname"].GetString();
					retVideo += printtime; retVideo += ";";
				}
				//if (videoList[i].HasMember("location") && videoList[i]["location"].IsInt())
				//{
				//	std::string location = std::to_string(videoList[i]["location"].GetInt());
				//	retVideo += location;retVideo += ";";
				//}
				//if (videoList[i].HasMember("interval") && videoList[i]["interval"].IsInt())
				//{
				//	std::string interval = std::to_string(videoList[i]["interval"].GetInt());
				//	retVideo += interval; retVideo += ";";
				//}
				//if (videoList[i].HasMember("render") && videoList[i]["render"].IsInt())
				//{
				//	std::string render = std::to_string(videoList[i]["render"].GetInt());
				//	retVideo += render; retVideo += ";";
				//}
				fileList += retVideo;
			}
			if (videoCallback)
			{
				videoCallback(m_printer.ipAddress.toStdString(), fileList);
			}
		}

		if (doc.HasMember("modelFanPct") && doc["modelFanPct"].IsInt())
		{
			m_printer.fanSpeedState = doc["modelFanPct"].GetInt();
		}
		if (doc.HasMember("caseFanPct") && doc["caseFanPct"].IsInt())
		{
			m_printer.caseFanSpeedState = doc["caseFanPct"].GetInt();
		}
		if (doc.HasMember("auxiliaryFanPct") && doc["auxiliaryFanPct"].IsInt())
		{
			m_printer.auxiliaryFanSpeedState = doc["auxiliaryFanPct"].GetInt();
		}

		m_printer.printerStatus = 1;
		if (infoCallback) infoCallback(m_printer);
	}
}
