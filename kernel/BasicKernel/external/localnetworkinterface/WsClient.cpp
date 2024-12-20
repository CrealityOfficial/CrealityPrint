#include "WsClient.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "materialboxmodellist.h"
#include "kernel/kernel.h"
#include "gcodecolormanager.h"
#include <cpr/cpr.h>
#include <qglobal.h>
#include <unordered_map>
#include <iostream>
#include <regex>
#include <chrono>
#include <QDebug>
#include <QApplication>
namespace creative_kernel
{
	long long SysMs() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}
	WsClient::WsClient(net::io_context& ioc, const std::string& ip, const std::string& strMac, const int& port) : m_resolver(net::make_strand(ioc)), m_serverIp(ip), m_serverPort(port)
	{
		m_ws = std::make_unique<websocket::stream<beast::tcp_stream>>(net::make_strand(ioc));
		m_printer.ipAddress = QString::fromStdString(ip);
		m_printer.macAddress = QString::fromStdString(strMac);
		m_printer.fluiddPort = 8888; m_printer.mainsailPort = 8888;
		m_printer.printMode = 1; m_printer.type = RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408;
	}

	void WsClient::start()
	{
		m_resolver.async_resolve(m_serverIp, std::to_string(m_serverPort), beast::bind_front_handler(&WsClient::onResolve, shared_from_this()));
	}

	void WsClient::stop(std::function<void()> callback)
	{
		needStop = true;
		// Close the WebSocket connection
		std::lock_guard<std::mutex> mtx(m_mtx);
		m_ws->async_close(websocket::close_code::normal, std::bind(&WsClient::onClose, shared_from_this(), std::placeholders::_1, callback));
	}

	void WsClient::sendTextMessage(const std::string& message)
	{
		if(!m_ws->is_open())//Ensure that the printer is functioning properly
		{
			return;
		}

		std::lock_guard<std::mutex> mtx(m_mtx);
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
		std::chrono::seconds(6),       // idle timeout. no data recv from peer for 6 sec, then it is timeout
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
		paramObject.AddMember("reqGcodeList", 1, allocator);	
		paramObject.AddMember("reqHistory", 1, allocator);
		paramObject.AddMember("reqElapseVideoList", 1, allocator);
		paramObject.AddMember("reqPrintObjects", 1, allocator);
		paramObject.AddMember("reqMaterialBoxsInfo", 1, allocator);
		paramObject.AddMember("boxsInfo", 1, allocator);
		paramObject.AddMember("reqMaterials", 1, allocator);
		rapidjson::Value boxConfig(rapidjson::kObjectType);
		paramObject.AddMember("boxConfig", boxConfig, allocator);
		doc.AddMember("params", paramObject, allocator);


		rapidjson::StringBuffer strbuf;
		rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
		doc.Accept(writer);

		std::string json(strbuf.GetString(), strbuf.GetSize());
		auto updateMacCallback = std::function<void(cpr::Response)>(std::bind(&WsClient::updatePrinterMac, this, std::placeholders::_1, m_printer));
		std::string strUrl  = "http://" + m_printer.ipAddress.toStdString() + ":" + std::to_string(80) + "/info";
		cpr::GetCallback(updateMacCallback, cpr::Url{ strUrl }, cpr::Url{ strUrl }, cpr::Timeout{ 2000 }, cpr::Parameters{ });
		// Write getfilelist payload
		m_ws->async_write(net::buffer(json), beast::bind_front_handler(&WsClient::onWriteGetfilelist, shared_from_this()));
	}
	void WsClient::updatePrinterMac(cpr::Response r, RemotePrinter& printer)
	{
		if (r.status_code == 200)
		{
			rapidjson::Document doc;
			doc.Parse(r.text.c_str());
			if (doc.HasParseError())
			{
				return;
			}
			m_printer.macAddress = doc["mac"].GetString();
		}
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
		if (m_offlineTime)
		{
			m_offlineTime = 0;
		}
		//parse message
		auto msg = boost::beast::buffers_to_string(m_readBuffer.cdata());
		m_readBuffer.consume(m_readBuffer.size());
		ParseMsg(msg);

		m_ws->async_read(m_readBuffer, beast::bind_front_handler(&WsClient::onRead, shared_from_this()));
	}

	void WsClient::onClose(beast::error_code ec, std::function<void()> callback)
	{
		if (callback && !m_ws->is_open())
		{
			callback();
		}
		if (ec) return onFail(ec, "close");
	}

	void WsClient::onWrite(beast::error_code ec, std::size_t bytes_transferred)
	{
		if (ec) return onFail(ec, "write");
		std::lock_guard<std::mutex> mtx(m_mtx);

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
		time_t tmNow = time(nullptr);
		if (m_offlineTime == 0)
		{
			m_offlineTime = tmNow;
		}
		if (m_printer.printerStatus && infoCallback && abs(tmNow - m_offlineTime) > 5)
		{
			m_printer.printerStatus = 0;
			infoCallback(m_printer);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		m_ws.reset(new websocket::stream<beast::tcp_stream>(m_ws->get_executor()));
		start();
	}

	void WsClient::ParseMsg(const std::string& message)
	{
		rapidjson::Document doc;
		doc.Parse(message.c_str());
		//qDebug()<<QString::fromStdString(message);
		if (doc.HasParseError()) return;
		if (doc.HasMember("curFeedratePct") && doc["curFeedratePct"].IsInt())
		{
			m_printer.printSpeed = doc["curFeedratePct"].GetInt();
		}
		if (doc.HasMember("connect") && doc["connect"].IsInt())
		{
			m_printer.printerStatus = doc["connect"].GetInt();				
		}
		if (doc.HasMember("cfsConnect") && doc["cfsConnect"].IsInt())
		{
			m_printer.cfsConnect = doc["cfsConnect"].GetInt();
			if (m_printer.cfsConnect==1)
			{
				rapidjson::Document doc; doc.SetObject();
				rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

				doc.AddMember("method", "get", allocator);
				rapidjson::Value paramObject(rapidjson::kObjectType);
				paramObject.AddMember("boxsInfo", 1, allocator);
				rapidjson::Value boxConfig(rapidjson::kObjectType);
				paramObject.AddMember("boxConfig", boxConfig, allocator);
				doc.AddMember("params", paramObject, allocator);
				rapidjson::StringBuffer strbuf;
				rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
				doc.Accept(writer);
				std::string json(strbuf.GetString(), strbuf.GetSize());
				sendTextMessage(json);
				
			}
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
		if (doc.HasMember("hostname") && doc["hostname"].IsString())
		{
			m_printer.deviceName = doc["hostname"].GetString();
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

				if (errorObject.HasMember("value"))
				{
					QString str = errorObject["value"].GetString();
					if (errorObject["value"].IsString())
					{
						QString errorPlateAndSlot = errorObject["value"].GetString();
						errorPlateAndSlot.remove("[");
						errorPlateAndSlot.remove("]");
						m_printer.errorPlate = errorPlateAndSlot.split(",")[0];
						m_printer.errorSlot = errorPlateAndSlot.split(",")[0];
					}
				}

				m_printer.errorKey = errorKey;
				m_printer.errorCode = errorCode;
			}
		}	
		if (doc.HasMember("repoPlrStatus") && doc["repoPlrStatus"].IsInt())
		{
			m_printer.repoPlrStatus = doc["repoPlrStatus"].GetInt();
			if (m_printer.repoPlrStatus) {
				m_printer.errorCode = 20000;
				m_printer.errorKey = 20000;
			}
			else if (!m_printer.repoPlrStatus && m_printer.errorCode == 20000)
			{
				m_printer.errorCode = 0;
				m_printer.errorKey = 0;
			}
		}
		if (doc.HasMember("materialStatus") && doc["materialStatus"].IsInt())
		{
			m_printer.materialStatus = doc["materialStatus"].GetInt();
			if (m_printer.materialStatus) 
			{
				m_printer.errorCode = 20010;
				m_printer.errorKey = 20010;
			}else if (!m_printer.materialStatus && m_printer.errorCode == 20010) 
			{
				m_printer.errorCode = 0;
				m_printer.errorKey = 0;
			};
		}
		if(doc.HasMember("retMaterials") && doc["retMaterials"].IsArray())
		{
			m_printer.remoteMaterial.clear();
			auto retMaterialsList = doc["retMaterials"].GetArray();
			for (int i = 0; i < retMaterialsList.Size(); i++)
			{
				RemoteMaterial material;
				if(retMaterialsList[i].HasMember("base")&& retMaterialsList[i]["base"].IsObject())
				{
					auto retMaterials = retMaterialsList[i]["base"].GetObj();
					if (retMaterials.HasMember("id") && retMaterials["id"].IsString())
					{
						material.id = retMaterials["id"].GetString();
						material.name = retMaterials["name"].GetString();
						material.brand = retMaterials["brand"].GetString();
						material.meterialType = retMaterials["meterialType"].GetString();
						material.maxTemp = retMaterials["maxTemp"].GetInt();
						material.minTemp = retMaterials["minTemp"].GetInt();
						
						
					}
				}
				if(retMaterialsList[i].HasMember("kvParam")&& retMaterialsList[i]["kvParam"].IsObject())
				{
					auto retKvParam = retMaterialsList[i]["kvParam"].GetObj();
					material.pressure_advance = retKvParam["pressure_advance"].GetString();
				}
				if(material.brand!="")
					m_printer.remoteMaterial.push_back(material);
				
			}
			
		}
		if (doc.HasMember("retGcodeFileInfo") && doc["retGcodeFileInfo"].IsObject())
		{
			auto retGcodeFileInfo = doc["retGcodeFileInfo"].GetObj();
			if (retGcodeFileInfo.HasMember("fileInfo") && retGcodeFileInfo["fileInfo"].IsString())
			{
				auto fileList = retGcodeFileInfo["fileInfo"].GetString();
				QString from_std_list = QString::fromStdString(fileList);
				QStringList splitList = from_std_list.split(":", QString::SkipEmptyParts);
				if(splitList.size()){
					std::string firstValue = splitList[0].toStdString();
					std::regex pattern("^\\/[a-zA-Z0-9_]+(\\/[a-zA-Z0-9_]+)*$");
					if (std::regex_match(firstValue, pattern))
					{
						m_printer.filePrefixPath = splitList[0];
					}
					else {
						m_printer.filePrefixPath = "";
					}
				}

				if (fileCallback)
				{
					fileCallback(m_printer.macAddress.toStdString(), fileList);
				}
			} 
		}
		if (doc.HasMember("retGcodeFileInfo2") && doc["retGcodeFileInfo2"].IsArray())
		{
			auto retGcodeFileInfos = doc["retGcodeFileInfo2"].GetArray();
			m_printer.remoteFileList.clear();
			for (int i = 0; i < retGcodeFileInfos.Size(); i++)
			{
				GcodeFile file;
				file.custom_types = retGcodeFileInfos[i].HasMember("custom_types")&&retGcodeFileInfos[i]["custom_types"].IsInt()?retGcodeFileInfos[i]["custom_types"].GetInt():0;
				if(file.custom_types == 2) continue; //目录咱不支持
				file.type = retGcodeFileInfos[i].HasMember("type")&&retGcodeFileInfos[i]["type"].IsInt()?retGcodeFileInfos[i]["type"].GetInt():0;
				file.file_size = retGcodeFileInfos[i].HasMember("file_size")&&retGcodeFileInfos[i]["file_size"].IsInt()?retGcodeFileInfos[i]["file_size"].GetInt():0;
				file.create_time = retGcodeFileInfos[i].HasMember("create_time")&&retGcodeFileInfos[i]["create_time"].IsInt()?retGcodeFileInfos[i]["create_time"].GetInt():0;
				file.name = retGcodeFileInfos[i].HasMember("name")&&retGcodeFileInfos[i]["name"].IsString()?retGcodeFileInfos[i]["name"].GetString():"";
				file.path = retGcodeFileInfos[i].HasMember("path")&&retGcodeFileInfos[i]["path"].IsString()?retGcodeFileInfos[i]["path"].GetString():"";
				if(file.path.size()>0) 
				{
					int index = file.path.find_last_of("/");
					if (index >0)
						m_printer.filePrefixPath = QString::fromStdString(file.path.substr(0, index));
				}
				
				file.thumbnail = retGcodeFileInfos[i].HasMember("thumbnail")&&retGcodeFileInfos[i]["thumbnail"].IsString()?retGcodeFileInfos[i]["thumbnail"].GetString():"";
				if(retGcodeFileInfos[i].HasMember("file")&&retGcodeFileInfos[i]["file"].IsArray())
				{
					auto retGcodeInfos = retGcodeFileInfos[i]["file"].GetArray();
					for (int j = 0; j < retGcodeInfos.Size(); j++)
					{
						GcodeFileInfo info;
						info.custom_types = retGcodeInfos[j].HasMember("custom_types")&&retGcodeInfos[j]["custom_types"].IsInt()?retGcodeInfos[j]["custom_types"].GetInt():0;
						info.type = retGcodeInfos[j].HasMember("type")&&retGcodeInfos[j]["type"].IsInt()?retGcodeInfos[j]["type"].GetInt():0;
						info.file_size = retGcodeInfos[j].HasMember("file_size")&&retGcodeInfos[j]["file_size"].IsInt()?retGcodeInfos[j]["file_size"].GetInt():0;
						info.create_time = retGcodeInfos[j].HasMember("create_time")&&retGcodeInfos[j]["create_time"].IsInt()?retGcodeInfos[j]["create_time"].GetInt():0;
						info.name = retGcodeInfos[j].HasMember("name")&&retGcodeInfos[j]["name"].IsString()?retGcodeInfos[j]["name"].GetString():"";
						info.path = retGcodeInfos[j].HasMember("path")&&retGcodeInfos[j]["path"].IsString()?retGcodeInfos[j]["path"].GetString():"";
						info.thumbnail = retGcodeInfos[j].HasMember("thumbnail")&&retGcodeInfos[j]["thumbnail"].IsString()?retGcodeInfos[j]["thumbnail"].GetString():"";
						info.timeCost = retGcodeInfos[j].HasMember("timeCost")&&retGcodeInfos[j]["timeCost"].IsInt()?retGcodeInfos[j]["timeCost"].GetInt():0;
						info.consumables = retGcodeInfos[j].HasMember("consumables")&&retGcodeInfos[j]["consumables"].IsInt()?retGcodeInfos[j]["consumables"].GetInt():0;
						info.floorHeight = retGcodeInfos[j].HasMember("floorHeight")&&retGcodeInfos[j]["floorHeight"].IsInt()?retGcodeInfos[j]["floorHeight"].GetInt():0;
						info.modelX = retGcodeInfos[j].HasMember("modelX")&&retGcodeInfos[j]["modelX"].IsInt()?retGcodeInfos[j]["modelX"].GetInt():0;
						info.modelY = retGcodeInfos[j].HasMember("modelY")&&retGcodeInfos[j]["modelY"].IsInt()?retGcodeInfos[j]["modelY"].GetInt():0;
						info.material = retGcodeInfos[j].HasMember("material")&&retGcodeInfos[j]["material"].IsString()?retGcodeInfos[j]["material"].GetString():"";
						info.nozzleTemp = retGcodeInfos[j].HasMember("nozzleTemp")&&retGcodeInfos[j]["nozzleTemp"].IsInt()?retGcodeInfos[j]["nozzleTemp"].GetInt():0;
						info.bedTemp = retGcodeInfos[j].HasMember("bedTemp")&&retGcodeInfos[j]["bedTemp"].IsInt()?retGcodeInfos[j]["bedTemp"].GetInt():0;
						info.software = retGcodeInfos[j].HasMember("software")&&retGcodeInfos[j]["software"].IsString()?retGcodeInfos[j]["software"].GetString():"";
						info.startPixel = retGcodeInfos[j].HasMember("startPixel")&&retGcodeInfos[j]["startPixel"].IsInt()?retGcodeInfos[j]["startPixel"].GetInt():0;
						info.endPixel = retGcodeInfos[j].HasMember("endPixel")&&retGcodeInfos[j]["endPixel"].IsInt()?retGcodeInfos[j]["endPixel"].GetInt():0;
						info.modelHeight = retGcodeInfos[j].HasMember("modelHeight")&&retGcodeInfos[j]["modelHeight"].IsInt()?retGcodeInfos[j]["modelHeight"].GetInt():0;
						info.layerHeight = retGcodeInfos[j].HasMember("layerHeight")&&retGcodeInfos[j]["layerHeight"].IsInt()?retGcodeInfos[j]["layerHeight"].GetInt():0;
						info.preview = retGcodeInfos[j].HasMember("preview")&&retGcodeInfos[j]["preview"].IsString()?retGcodeInfos[j]["preview"].GetString():"";
						info.materialColors = retGcodeInfos[j].HasMember("materialColors")&&retGcodeInfos[j]["materialColors"].IsString()?retGcodeInfos[j]["materialColors"].GetString():"";
						info.materialIds = retGcodeInfos[j].HasMember("materialIds")&&retGcodeInfos[j]["materialIds"].IsString()?retGcodeInfos[j]["materialIds"].GetString():"";
						info.match = retGcodeInfos[j].HasMember("match")&&retGcodeInfos[j]["match"].IsString()?retGcodeInfos[j]["match"].GetString():"";
						file.files.push_back(info);
					}
					
				}else{
					GcodeFileInfo info;
					info.timeCost = retGcodeFileInfos[i].HasMember("timeCost")&&retGcodeFileInfos[i]["timeCost"].IsInt()?retGcodeFileInfos[i]["timeCost"].GetInt():0;
					info.consumables = retGcodeFileInfos[i].HasMember("consumables")&&retGcodeFileInfos[i]["consumables"].IsInt()?retGcodeFileInfos[i]["consumables"].GetInt():0;
					info.floorHeight = retGcodeFileInfos[i].HasMember("floorHeight")&&retGcodeFileInfos[i]["floorHeight"].IsInt()?retGcodeFileInfos[i]["floorHeight"].GetInt():0;
					info.modelX = retGcodeFileInfos[i].HasMember("modelX")&&retGcodeFileInfos[i]["modelX"].IsInt()?retGcodeFileInfos[i]["modelX"].GetInt():0;
					info.modelY = retGcodeFileInfos[i].HasMember("modelY")&&retGcodeFileInfos[i]["modelY"].IsInt()?retGcodeFileInfos[i]["modelY"].GetInt():0;
					info.material = retGcodeFileInfos[i].HasMember("material")&&retGcodeFileInfos[i]["material"].IsString()?retGcodeFileInfos[i]["material"].GetString():"";
					info.nozzleTemp = retGcodeFileInfos[i].HasMember("nozzleTemp")&&retGcodeFileInfos[i]["nozzleTemp"].IsInt()?retGcodeFileInfos[i]["nozzleTemp"].GetInt():0;
					info.bedTemp = retGcodeFileInfos[i].HasMember("bedTemp")&&retGcodeFileInfos[i]["bedTemp"].IsInt()?retGcodeFileInfos[i]["bedTemp"].GetInt():0;
					info.startPixel = retGcodeFileInfos[i].HasMember("startPixel")&&retGcodeFileInfos[i]["startPixel"].IsInt()?retGcodeFileInfos[i]["startPixel"].GetInt():0;
					info.endPixel = retGcodeFileInfos[i].HasMember("endPixel")&&retGcodeFileInfos[i]["endPixel"].IsInt()?retGcodeFileInfos[i]["endPixel"].GetInt():0;
					info.modelHeight = retGcodeFileInfos[i].HasMember("modelHeight")&&retGcodeFileInfos[i]["modelHeight"].IsInt()?retGcodeFileInfos[i]["modelHeight"].GetInt():0;
					info.layerHeight = retGcodeFileInfos[i].HasMember("layerHeight")&&retGcodeFileInfos[i]["layerHeight"].IsInt()?retGcodeFileInfos[i]["layerHeight"].GetInt():0;
					info.preview = retGcodeFileInfos[i].HasMember("preview")&&retGcodeFileInfos[i]["preview"].IsString()?retGcodeFileInfos[i]["preview"].GetString():"";
					info.materialColors = retGcodeFileInfos[i].HasMember("materialColors")&&retGcodeFileInfos[i]["materialColors"].IsString()?retGcodeFileInfos[i]["materialColors"].GetString():"";
					info.materialIds = retGcodeFileInfos[i].HasMember("materialIds")&&retGcodeFileInfos[i]["materialIds"].IsString()?retGcodeFileInfos[i]["materialIds"].GetString():"";
					info.match = retGcodeFileInfos[i].HasMember("software")&&retGcodeFileInfos[i]["software"].IsString()?retGcodeFileInfos[i]["software"].GetString():"";
					file.files.push_back(info);
				}
				m_printer.remoteFileList.append(file);
			}
			m_printer.remoteFileUpdateTime = SysMs();
			qWarning() << "remoteFileUpdateTime:" << m_printer.remoteFileUpdateTime;
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
				historyFileListCb(m_printer.macAddress.toStdString(), fileList);
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
				videoCallback(m_printer.macAddress.toStdString(), fileList);
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
		if (doc.HasMember("objects") && doc["objects"].IsString())
		{
			m_printer.printObjects = doc["objects"].GetString();
		}
		if (doc.HasMember("excluded_objects") && doc["excluded_objects"].IsString())
		{
			m_printer.excludedObjects = doc["excluded_objects"].GetString();
		}
		if (doc.HasMember("current_object") && doc["current_object"].IsString())
		{
			m_printer.currentObject = doc["current_object"].GetString();
		}
		if (doc.HasMember("maxNozzleTemp") && doc["maxNozzleTemp"].IsInt())
		{
			m_printer.maxNozzleTemp = doc["maxNozzleTemp"].GetInt();
		}
		if (doc.HasMember("maxBedTemp") && doc["maxBedTemp"].IsInt())
		{
			m_printer.maxBedTemp = doc["maxBedTemp"].GetInt();
		}
		if (doc.HasMember("boxsInfo") && doc["boxsInfo"].IsObject())
		{
				m_printer.boxsInfo.exist = true;
				m_printer.boxsInfo.materialBoxs.clear();
				Material material1;
				std::string boxList;
				auto info = doc["boxsInfo"].GetObj();
				auto boxInfo = info["materialBoxs"].GetArray();
				if(info.HasMember("same_material") && info["same_material"].IsArray())
				{
					auto same_material = info["same_material"].GetArray();
					rapidjson::StringBuffer buffer;
					rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
					rapidjson::Value& valobj = same_material;
					valobj.Accept(writer);
					std::string jsonstr = buffer.GetString();
					m_printer.boxsInfo.sameMaterial = jsonstr;
				}

				
				int sz = boxInfo.Size();
				if (sz > 0) {
					for (rapidjson::SizeType i = 0; i < sz; i++) {
						std::string boxItem = "";
						const rapidjson::Value& item = boxInfo[i];
						Box materialBox1;
						if (item.HasMember("id") && item.HasMember("type") && item.HasMember("materials")) {
							const rapidjson::Value& id = item["id"];		
							const rapidjson::Value& type = item["type"];
							double boxHumidity = item.HasMember("humidity")? item["humidity"].GetDouble(): -1;
							int boxEditStatus = item.HasMember("editStatus") && item["editStatus"].IsInt()? item["editStatus"].GetInt(): 0;
							int boxId = id.GetInt();
							int boxType = type.GetInt();
							materialBox1.id = boxId;
							materialBox1.type = boxType;
							materialBox1.humidity = boxHumidity;
							materialBox1.editStatus = boxEditStatus;
							auto materialInfo = item["materials"].GetArray();
							int msz = materialInfo.Size();
							if (msz > 0) {
								for (rapidjson::SizeType i = 0; i < msz; i++) {
									const rapidjson::Value& item1 = materialInfo[i];

									const rapidjson::Value& id1 = item1["id"];
									int materialId1 = id1.GetInt();
									std::string  materialVendor1 = item1.HasMember("vendor") && item1["vendor"].IsString()
										? item1["vendor"].GetString()
										: "";
									std::string materialType1 = item1.HasMember("type") && item1["type"].IsString()
										? item1["type"].GetString()
										: "";
									std::string materialColor1 = item1.HasMember("color") && item1["color"].IsString()
										? item1["color"].GetString()
										: "#FFFFFF";

									std::string materialName1 = item1.HasMember("name") && item1["name"].IsString()
										? item1["name"].GetString()
										: "";

									int materialMinTemp1 = item1.HasMember("minTemp") && item1["minTemp"].IsInt()
										? item1["minTemp"].GetInt()
										: 0;

									int materialMaxTemp1 = item1.HasMember("maxTemp") && item1["maxTemp"].IsInt()
										? item1["maxTemp"].GetInt()
										: 0;
									int materialSelected1 = item1.HasMember("selected") && item1["selected"].IsInt()
										? item1["selected"].GetInt()
										: 0;

									std::string materialRfid1 = item1.HasMember("rfid") && item1["rfid"].IsString()
										? item1["rfid"].GetString()
										: "";
									double materialPressure1 = item1.HasMember("pressure") && item1["pressure"].IsDouble()
										? item1["pressure"].GetDouble()
										: 0.00;

									double materialPercent1 = item1.HasMember("percent")
										? item1["percent"].GetDouble()
										: 0.00;

									int materialState1 = item1.HasMember("state") && item1["state"].IsInt()
										? item1["state"].GetInt()
										: 0;
									int materialEditStatus1 = item1.HasMember("editStatus") && item1["editStatus"].IsInt()
										? item1["editStatus"].GetInt()
										: 0;

									material1.id = materialId1;
									material1.vendor = materialVendor1;
									material1.type = materialType1;
									material1.color = materialColor1;
									material1.name = materialName1;
									material1.minTemp = materialMinTemp1;
									material1.maxTemp = materialMaxTemp1;
									material1.selected = materialSelected1;
									material1.rfid = materialRfid1;
									material1.pressure = materialPressure1;
									material1.percent = materialPercent1;
									material1.state = materialState1;
									material1.editStatus = materialEditStatus1;
									materialBox1.materials.push_back(material1);
								}
							}

							m_printer.boxsInfo.materialBoxs.push_back(materialBox1);
						}
					}
					
				}
				m_printer.boxsInfo.updateTime = SysMs();
                //if (m_printer.materialboxList == nullptr) {
                //    m_printer.materialboxList = new MaterialBoxListModel();
                //    m_printer.materialboxList->moveToThread(qApp->thread());
                //}
                //m_printer.materialboxList->onGetMaterialBoxList(m_printer.macAddress.toStdString(), boxsInfo, m_printer.type);
	    }
		
			
		if (doc.HasMember("webrtcSupport") && doc["webrtcSupport"].IsInt())
		{
			m_printer.webrtcSupport = doc["webrtcSupport"].GetInt();
		}

		if (doc.HasMember("materialState") && doc["materialState"].IsObject())
		{
			auto info = doc["materialState"].GetObj();
			if(!info.ObjectEmpty())
			{
				MaterialState state;
				state.boxId = info.HasMember("boxId")&&info["boxId"].IsInt()?info["boxId"].GetInt():0;
				state.materialId = info.HasMember("materialId")&&info["materialId"].IsInt()?info["materialId"].GetInt():0;
				state.percent = info.HasMember("percent")&&info["percent"].IsInt()?info["percent"].GetInt():0;
				state.state = info.HasMember("state")&&info["state"].IsInt()?info["state"].GetInt():0;
				state.selected = info.HasMember("selected")&&info["selected"].IsInt()?info["selected"].GetInt():0;
                if(info.HasMember("editStatus") && info["editStatus"].IsInt())//This field is returned by the new firmware version, and crashes when encountering devices that have not been upgraded
				    state.editStatus = info["editStatus"].GetInt();
				
				state.updateTime = SysMs();
				m_printer.boxsInfo.materialState = state;
				
				//m_printer.materialboxList->onGetMaterialState(m_printer.macAddress, state, m_printer.type);
			}

		}

		if (doc.HasMember("boxConfig") && doc["boxConfig"].IsObject())
		{
			auto info = doc["boxConfig"].GetObj();
			if (!info.ObjectEmpty())
			{
				BoxConfig config;
				config.autoRefill = info.HasMember("autoRefill")&&info["autoRefill"].IsInt()?info["autoRefill"].GetInt():0;
				config.cAutoFeed = info.HasMember("cAutoFeed")&&info["cAutoFeed"].IsInt()?info["cAutoFeed"].GetInt():0;
				config.cSelfTest = info.HasMember("cSelfTest")&&info["cSelfTest"].IsInt()?info["cSelfTest"].GetInt():0;
				config.updateTime = SysMs();
				m_printer.boxsInfo.config = config;

				//m_printer.materialboxList->onGetBoxConfig(m_printer.macAddress.toStdString(), state, m_printer.type);
			}
		}


		if (doc.HasMember("feedState") && doc["feedState"].IsInt())
		{
			m_printer.feedState = doc["feedState"].GetInt();
		}

		if (doc.HasMember("feedState") && doc["feedState"].IsInt())
		{
			m_printer.feedState = doc["feedState"].GetInt();
		}

		if (doc.HasMember("colorMatch") && doc["colorMatch"].IsObject())
		{
			auto info = doc["colorMatch"].GetObj();
			//MaterialBoxListModel::ColorMap map;
			//MaterialBoxListModel::ColorMapInfo ColorMapList;
			if (!info.ObjectEmpty())
			{
				auto list = info["list"].GetArray();
				if (list.Size() > 0) {
					getKernel()->gCodeColorManage()->cleatDatas();
					for (rapidjson::SizeType i = 0; i < list.Size(); i++) {
						GCodeColorData* colorData = new GCodeColorData();
						const rapidjson::Value& item = list[i];
						colorData->colorId = item.HasMember("id") && item["id"].IsString()
							? item["id"].GetString()
							: "";
						colorData->type = item.HasMember("type") && item["type"].IsString()
							? item["type"].GetString()
							: "";
						colorData->color = item.HasMember("color") && item["color"].IsString()
							? item["color"].GetString()
							: "";
						colorData->boxId = item.HasMember("boxId") && item["boxId"].IsInt()
							? item["boxId"].GetInt()
							: 0;
						colorData->materialId = item.HasMember("materialId") && item["materialId"].IsInt()
							? item["materialId"].GetInt()
							: 0;
						getKernel()->gCodeColorManage()->addColorData(colorData);
					}
					
					getKernel()->gCodeColorManage()->setCount(list.Size());	
				}else{
					//如果颜色列表为空，需要重新映射颜色
					getKernel()->gCodeColorManage()->cleatDatas();
					getKernel()->gCodeColorManage()->addDefaultColorData();
				}
				

			}
		}

		m_printer.printerStatus = 1;
		if (infoCallback && !needStop) infoCallback(m_printer);
	}
}
