#include "RemotePrinter.h"
#include "RemotePrinterManager.h"
#include "rapidjson/document.h"
#include <QDebug>
#include <thread>
#include <cpr/api.h>
#include <external/interface/machineinterface.h>

#include "cxmdns.h"

namespace creative_kernel
{
	SINGLETON_IMPL(RemotePrinterManager)
	RemotePrinterManager::RemotePrinterManager()
	{
		m_pKlipperInterface = new KlipperInterface();
		m_pLanPrinterInterface = new LanPrinterInterface();
		m_pOctoPrinterInterface = new OctoPrintInterface();
		m_pKlipper4408Interface = new Klipper4408Interface();

		qRegisterMetaType<RemotePrinerType>("RemotePrinerType");

		answerRegExp = QRegExp("_creality([\\d]{2})([\\d]{4}).+");

		auto t = std::thread(&RemotePrinterManager::refreshStateThread, this);
		t.detach();

		t = std::thread(&RemotePrinterManager::downloadThread, this);
		t.detach();
	}

	void RemotePrinterManager::Init()
	{

	}

	RemotePrinterManager::~RemotePrinterManager()
	{
		m_bExit = true;
		m_mutex.lock();
		m_mapID2Printer.clear();
		m_mutex.unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(3000));

		delete m_pKlipperInterface;
		delete m_pLanPrinterInterface;
		delete m_pOctoPrinterInterface;
		delete m_pKlipper4408Interface;
	}

	void RemotePrinterManager::searchDevices()
	{
		//Ŀǰֻ֧�־�������������,����Ŀǰ��ȫ�����أ�����ĳ��ѵ�һ������һ��
		auto t = std::thread(&RemotePrinterManager::searchLanDeviceThread, this);
		t.detach();
	}

	void RemotePrinterManager::searchLanDeviceThread()
	{
		std::vector<std::string> prefix;
		prefix.push_back("CXSWBox");
		prefix.push_back("creality");
		prefix.push_back("Creality");
		auto devices = cxnet::syncDiscoveryService(prefix);

		for (auto item : devices)
		{
			QString answer = QString::fromStdString(item.answer);
			if (answer.startsWith("_CXSWBox"))
			{
				//REMOTE_PRINTER_TYPE_LAN
				std::string strServerIp = item.machineIp;
				m_pLanPrinterInterface->getDeviceState(strServerIp, [=](std::unordered_map<std::string, std::string> ret, RemotePrinterSession printerSession) {
						if (ret.size() > 0 && m_pfnSearchCb)
						{
							QString ssid = QString::fromStdString(ret["ssid"]);
							QStringList splitList = ssid.split("-");

							if (splitList.size() > 1)
							{
								RemotePrinter printer;
								printer.type = RemotePrinerType::REMOTE_PRINTER_TYPE_LAN;
								printer.ipAddress = QString::fromStdString(strServerIp);
								printer.printerName = QString::fromStdString(ret["model"]);
								printer.macAddress = splitList[1];
								m_pfnSearchCb(printer);
							}
						}
					}
				);
			}
			else if (answerRegExp.exactMatch(answer))//_creality
			{
				QString machineType = answerRegExp.cap(1);
				QString moonrakerPort = answerRegExp.cap(2);
				//qDebug() << "machineType" << machineType << "moonrakerPort" << moonrakerPort;

				//REMOTE_PRINTER_TYPE_KLIPPER
				std::string strServerIp = item.machineIp;
				if (machineType == "00" && m_pfnSearchCb)
				{
					m_pKlipperInterface->getMultiMachine(strServerIp, moonrakerPort.toInt(), [=](std::vector<RemotePrinter> & printers) {
							QString multiPrinterName;
							for (const auto& printer : printers)
							{
								//doAddPrinter(printer);
								multiPrinterName.append(printer.printerName + "<br>");
							}
							if (!multiPrinterName.isEmpty())
							{
								RemotePrinter sonicPad;
								sonicPad.type = RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER;
								sonicPad.ipAddress = QString::fromStdString(strServerIp);
								sonicPad.macAddress = QString::fromStdString(strServerIp);
								sonicPad.printerName = multiPrinterName.chopped(4);
								m_pfnSearchCb(sonicPad);
							}
						}
					);
				}
			}
			else if (answer.startsWith("_Creality")) 
			{
				//REMOTE_PRINTER_TYPE_KLIPPER4408
				std::string strServerIp = item.machineIp;
				m_pKlipper4408Interface->getPrinterInfo(strServerIp, 80, [=](std::string mac, std::string model) {
						RemotePrinter printer;
						printer.type = RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408;
						printer.ipAddress = QString::fromStdString(strServerIp);
						printer.macAddress = QString::fromStdString(mac);
						printer.printerName = QString::fromStdString(model);
						m_pfnSearchCb(printer);
					}
				);
			}
		}
	}

	void RemotePrinterManager::refreshStateThread()
	{
		while (!m_bExit)
		{
			m_mutex.lock();
			if (m_mapID2Printer.empty())
			{
				m_mutex.unlock();
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}
			for (auto& item : m_mapID2Printer)
			{
				auto& session = item.second;
				time_t tmNow = time(nullptr);
				if (session.tmLastActive == 0 || abs(tmNow - session.tmLastActive) > 3)
				{
					switch (session.type)
					{
						case RemotePrinerType::REMOTE_PRINTER_TYPE_LAN:
						{
							m_pLanPrinterInterface->getDeviceState(session.ipAddress, [=](std::unordered_map<std::string, std::string> ret, RemotePrinterSession printerSession) {
								RemotePrinterSession session(printerSession);
								if (ret.size() > 0 && m_pfnGetPrinterInfoCb)
								{
									RemotePrinter printer;
									printer.uniqueId = session.uniqueId.c_str();
									printer.ipAddress = session.ipAddress.c_str();
									printer.type = RemotePrinerType::REMOTE_PRINTER_TYPE_LAN;

									QString ssid = QString(ret["ssid"].c_str());
									QStringList strList = ssid.split("-");
									if (strList.size() > 1)
									{
										printer.macAddress = strList[1];
										printer.deviceName = strList[0];
										if (session.macAddress.empty())
										{
											session.macAddress = printer.macAddress.toStdString();
										}
									}
									else
									{
										printer.macAddress = "";
										printer.deviceName = "";
									}
									printer.productKey = "";
									printer.printerName = QString(ret["model"].c_str()).trimmed();
									QString filepath = QString(ret["print"].c_str());
									QStringList list = filepath.split("/");
									bool mcu_is_print = ret["mcu_is_print"] == "1";
									if(mcu_is_print)
									{
										printer.printFileName = filepath;
										printer.printMode = 0;
									}
									else if(list.size() > 0 && filepath != "localhost")
									{
										printer.printFileName = list[list.size() - 1];
										if (!printer.printFileName.endsWith(".gcode"))
										{
											auto index = printer.printFileName.lastIndexOf(".gcode");
											printer.printFileName = printer.printFileName.left(index + std::string(".gcode").length());
											printer.printMode = 2;
										}
										else
										{
											printer.printMode = 1;
										}
									}
									printer.curPosition = QString(ret["curPosition"].c_str());
									printer.printerStatus = atoi(ret["connect"].c_str());
									printer.TFCardStatus = atoi(ret["tfCard"].c_str());
									printer.printState = atoi(ret["state"].c_str());
									printer.fanSwitchState = atoi(ret["fan"].c_str());
									printer.printProgress = atoi(ret["printProgress"].c_str());

									QSharedPointer<us::USettings> settings = createDefaultMachineSetting(printer.printerName);
									printer.nozzleCount = settings->value(QStringLiteral("machine_extruder_count"), "1").toInt();
									if (printer.nozzleCount == 1)
									{
										printer.nozzleTemp = atof(ret["nozzleTemp"].c_str());
										printer.nozzleTempTarget = atof(ret["nozzleTemp2"].c_str());
									}
									else
									{
										printer.nozzleTemp = atof(ret["_1st_nozzleTemp"].c_str());
										printer.nozzleTempTarget = atof(ret["_1st_nozzleTemp2"].c_str());
										printer.nozzle2Temp = atof(ret["_2nd_nozzleTemp"].c_str());
										printer.nozzle2TempTarget = atof(ret["_2nd_nozzleTemp2"].c_str());
										printer.chamberTemp = atof(ret["chamberTemp"].c_str());
										printer.chamberTempTarget = atof(ret["chamberTemp2"].c_str());
									}
									printer.machineHeight = settings->value(QStringLiteral("machine_height"), "0").toFloat();
									printer.machineWidth = settings->value(QStringLiteral("machine_width"), "0").toFloat();
									printer.machineDepth = settings->value(QStringLiteral("machine_depth"), "0").toFloat();

									printer.bedTemp = atof(ret["bedTemp"].c_str());
									printer.bedTempTarget = atof(ret["bedTemp2"].c_str());
									printer.printSpeed = atoi(ret["curFeedratePct"].c_str());;
									printer.printJobTime = atoi(ret["printJobTime"].c_str());
									printer.printLeftTime = atoi(ret["printLeftTime"].c_str());
									printer.autoHomeState = atoi(ret["autohome"].c_str());
									//printer.errorCode = atoi(ret["err"].c_str());
									printer.video = atoi(ret["video"].c_str());
									if ((!mcu_is_print) && (!printer.printFileName.isEmpty()) && (session.previewImg != printer.printFileName.toStdString()))
									{
										session.previewImg = printer.printFileName.toStdString();
										getPreviewImg(printer, printer.printFileName.toStdString());
									}
									printer.ledState = atoi(ret["led_state"].c_str());
									printer.totalLayer = atoi(ret["TotalLayer"].c_str());
									printer.layer = atoi(ret["layer"].c_str());
									m_pfnGetPrinterInfoCb(printer);
								}

								session.connected = true;
								}, session
							);
							session.tmLastActive = tmNow;
						}
							break;
						case RemotePrinerType::REMOTE_PRINTER_TYPE_OCTOPRINT:
						{
							m_pOctoPrinterInterface->getDeviceState(session.ipAddress, session.token, [&](const std::string& ret) {
								if (ret.size() > 0 && m_pfnGetPrinterInfoCb)
								{
									RemotePrinter printer;
									printer.ipAddress = session.ipAddress.c_str();
									printer.type = RemotePrinerType::REMOTE_PRINTER_TYPE_OCTOPRINT;

									rapidjson::Document doc;
									doc.Parse(ret.c_str());
									if (doc.HasParseError())
										return;
									if (doc.HasMember("state") && doc["state"].IsObject())
									{
										const rapidjson::Value& objectState = doc["state"];
										if (objectState.HasMember("text") && objectState["text"].IsString())
										{
											objectState["name"].GetString();
										}
										if (objectState.HasMember("flags") && objectState["flags"].IsObject())
										{
											const rapidjson::Value& objectFlags = doc["flags"];
											if (objectFlags.HasMember("operational") && objectFlags["operational"].IsBool())
											{
												objectFlags["operational"].GetBool();
											}
										}
									}
									if (doc.HasMember("sd") && doc["sd"].IsObject())
									{
										const rapidjson::Value& objectSD = doc["sd"];
										if (objectSD.HasMember("ready") && objectSD["ready"].IsBool())
										{
											printer.TFCardStatus = objectSD["ready"].GetBool();
										}
									}
									if (doc.HasMember("temperature") && doc["temperature"].IsObject())
									{
										const rapidjson::Value& objectTemperature = doc["temperature"];
										if (objectTemperature.HasMember("tool0") && objectTemperature["tool0"].IsObject())
										{
											const rapidjson::Value& objectTool0 = doc["tool0"];
											if (objectTool0.HasMember("actual") && objectTool0["actual"].IsDouble())
											{
												objectTool0["actual"].GetDouble();
											}
											if (objectTool0.HasMember("target") && objectTool0["target"].IsDouble())
											{
												objectTool0["target"].GetDouble();
											}
											if (objectTool0.HasMember("offset") && objectTool0["offset"].IsDouble())
											{
												objectTool0["offset"].GetDouble();
											}
										}

										if (objectTemperature.HasMember("tool0") && objectTemperature["tool0"].IsObject())
										{
											const rapidjson::Value& objectTool0 = doc["tool0"];
											if (objectTool0.HasMember("actual") && objectTool0["actual"].IsDouble())
											{
												objectTool0["actual"].GetDouble();
											}
											if (objectTool0.HasMember("target") && objectTool0["target"].IsDouble())
											{
												objectTool0["target"].GetDouble();
											}
											if (objectTool0.HasMember("offset") && objectTool0["offset"].IsDouble())
											{
												objectTool0["offset"].GetDouble();
											}
										}

										if (objectTemperature.HasMember("tool1") && objectTemperature["tool1"].IsObject())
										{
											const rapidjson::Value& objectTool1 = doc["tool1"];
											if (objectTool1.HasMember("actual") && objectTool1["actual"].IsDouble())
											{
												objectTool1["actual"].GetDouble();
											}
											if (objectTool1.HasMember("target") && objectTool1["target"].IsDouble())
											{
												objectTool1["target"].GetDouble();
											}
											if (objectTool1.HasMember("offset") && objectTool1["offset"].IsDouble())
											{
												objectTool1["offset"].GetDouble();
											}
										}

										if (objectTemperature.HasMember("bed") && objectTemperature["bed"].IsObject())
										{
											const rapidjson::Value& objectBed = doc["bed"];
											if (objectBed.HasMember("actual") && objectBed["actual"].IsDouble())
											{
												objectBed["actual"].GetDouble();
											}
											if (objectBed.HasMember("target") && objectBed["target"].IsDouble())
											{
												objectBed["target"].GetDouble();
											}
											if (objectBed.HasMember("offset") && objectBed["offset"].IsDouble())
											{
												objectBed["offset"].GetDouble();
											}
										}
									}

									m_pfnGetPrinterInfoCb(printer);
								}

								session.connected = true;
								session.tmLastActive = tmNow;
								});
						}
							break;
						case RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER:
						{
							auto funcGetDeviceCallback = [=](const std::string& ret, RemotePrinterSession printerSession) {
								RemotePrinterSession session(printerSession);
								if (ret.size() > 0 && m_pfnGetPrinterInfoCb)
								{
									RemotePrinter printer;
									printer.ipAddress = session.ipAddress.c_str();
									printer.type = session.type;
									printer.fluiddPort = session.fluiddPort;
									printer.mainsailPort = session.mainsailPort;
									printer.printerId = session.printerId;
									printer.printMode = 1;//default

									rapidjson::Document doc;
									doc.Parse(ret.c_str());
									if (doc.HasParseError()) return;

									if (doc.HasMember("result") && doc["result"].IsObject())
									{
										const rapidjson::Value& objectResult = doc["result"];

										if (objectResult.HasMember("status") && objectResult["status"].IsObject())
										{
											const rapidjson::Value& objectStatus = objectResult["status"];
											if (objectStatus.HasMember("fan") && objectStatus["fan"].IsObject())
											{
												const rapidjson::Value& objectFan = objectStatus["fan"];
												if (objectFan.HasMember("speed") && objectFan["speed"].IsDouble())
												{
													printer.fanSwitchState = objectFan["speed"].GetDouble() > 0;
												}
											}

											if (objectStatus.HasMember("gcode_move") && objectStatus["gcode_move"].IsObject())
											{
												const rapidjson::Value& objectGcodeMove = objectStatus["gcode_move"];
												if (objectGcodeMove.HasMember("speed_factor") && objectGcodeMove["speed_factor"].IsDouble())
												{
													printer.printSpeed = objectGcodeMove["speed_factor"].GetDouble() * 100;
												}
											}

											if (objectStatus.HasMember("heater_bed") && objectStatus["heater_bed"].IsObject())
											{
												const rapidjson::Value& objectHeaterBed = objectStatus["heater_bed"];
												if (objectHeaterBed.HasMember("target") && objectHeaterBed["target"].IsDouble())
												{
													printer.bedTempTarget = objectHeaterBed["target"].GetDouble();
												}
												if (objectHeaterBed.HasMember("temperature") && objectHeaterBed["temperature"].IsDouble())
												{
													printer.bedTemp = objectHeaterBed["temperature"].GetDouble();
												}
											}

											if (objectStatus.HasMember("extruder") && objectStatus["extruder"].IsObject())
											{
												const rapidjson::Value& objectExtruder = objectStatus["extruder"];
												if (objectExtruder.HasMember("target") && objectExtruder["target"].IsDouble())
												{
													printer.nozzleTempTarget = objectExtruder["target"].GetDouble();
												}
												if (objectExtruder.HasMember("temperature") && objectExtruder["temperature"].IsDouble())
												{
													printer.nozzleTemp = objectExtruder["temperature"].GetDouble();
												}
											}

											if (objectStatus.HasMember("display_status") && objectStatus["display_status"].IsObject())
											{
												const rapidjson::Value& objectExtruder = objectStatus["display_status"];
												if (objectExtruder.HasMember("progress") && objectExtruder["progress"].IsDouble())
												{
													printer.printProgress = objectExtruder["progress"].GetDouble() * 100;
												}
											}

											if (objectStatus.HasMember("print_stats") && objectStatus["print_stats"].IsObject())
											{
												const rapidjson::Value& objectExtruder = objectStatus["print_stats"];
												if (objectExtruder.HasMember("state") && objectExtruder["state"].IsString())
												{
													std::string printState = objectExtruder["state"].GetString();
													if (printState == "standby")
													{
														printer.printState = 0;
													}
													else if (printState == "printing")
													{
														printer.printState = 1;
													}
													else if (printState == "complete")
													{
														printer.printState = 2;
													}
													else if (printState == "error")
													{
														printer.printState = 3;
													}
													else if (printState == "cancelled")
													{
														printer.printState = 4;
													}
													else if (printState == "paused")
													{
														printer.printState = 5;
													}
												}
												if (objectExtruder.HasMember("print_duration") && objectExtruder["print_duration"].IsDouble())
												{
													printer.printJobTime = objectExtruder["print_duration"].GetDouble();
													if (printer.printProgress > 0)
													{
														printer.printLeftTime = printer.printJobTime * 100 / printer.printProgress;
													}
												}
												if (objectExtruder.HasMember("filename") && objectExtruder["filename"].IsString())
												{
													printer.printFileName = objectExtruder["filename"].GetString();
												}
											}
										}
									}
									printer.printerStatus = 1;
									printer.macAddress = session.macAddress.c_str();
									printer.printerName = session.printerName.c_str();
									printer.moonrakerPort = session.port;

									m_pfnGetPrinterInfoCb(printer);
								}

								session.connected = true;
							};

							if (session.macAddress.empty())
							{
								m_pKlipperInterface->getSystemInfo(session.ipAddress, session.port, [=](const std::string& ret, RemotePrinterSession printerSession) {
									RemotePrinterSession session(printerSession);
									if (ret.size() > 0)
									{
										rapidjson::Document doc;
										doc.Parse(ret.c_str());
										if (doc.HasParseError()) return;

										if (doc.HasMember("result") && doc["result"].IsObject())
										{
											const rapidjson::Value& objectResult = doc["result"];
											if (objectResult.HasMember("system_info") && objectResult["system_info"].IsObject())
											{
												const rapidjson::Value& objectSystemInfo = objectResult["system_info"];
												if (objectSystemInfo.HasMember("network") && objectSystemInfo["network"].IsObject())
												{
													const rapidjson::Value& objectNetwork = objectSystemInfo["network"];
													if (objectNetwork.HasMember("wlan0") && objectNetwork["wlan0"].IsObject())
													{
														const rapidjson::Value& objectWlan0 = objectNetwork["wlan0"];
														if (objectWlan0.HasMember("mac_address") && objectWlan0["mac_address"].IsString())
														{
															session.macAddress = objectWlan0["mac_address"].GetString();
															session.macAddress = session.macAddress + "_" + std::to_string(session.printerId);
														}
													}
												}
											}
										}
										if (!session.macAddress.empty())
										{
											m_pKlipperInterface->getDeviceState(session.ipAddress, session.port, funcGetDeviceCallback, session);
										}
									}
								}, session);
							}
							else
							{
								m_pKlipperInterface->getDeviceState(session.ipAddress, session.port, funcGetDeviceCallback, session);
							}
							session.tmLastActive = tmNow;
						}
							break;
						default:
							break;
					}
				}
				if (session.tmLastGetFileList == 0 || abs(tmNow - session.tmLastGetFileList) > 5)
				{
					switch (session.type)
					{
						case RemotePrinerType::REMOTE_PRINTER_TYPE_LAN:
						{
							m_pLanPrinterInterface->getFileListFromDevice(session.ipAddress, [=](std::string filelist) {
								if (m_pfnGetGcodeListCb)
								{
									m_pfnGetGcodeListCb(session.macAddress, filelist, RemotePrinerType::REMOTE_PRINTER_TYPE_LAN);
								}
							});
							session.tmLastGetFileList = tmNow;
						}
							break;
						default:
							break;
					}
				}
			}
			m_mutex.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		m_condition.notify_all();
	}

	void RemotePrinterManager::addPrinter(const RemotePrinter& printer)
	{
		auto callback = std::function<void(cpr::Response)>(std::bind(&RemotePrinterManager::tryAddPrinter, this, std::placeholders::_1, printer));
		switch (printer.type)
		{
			case RemotePrinerType::REMOTE_PRINTER_TYPE_LAN:
			{
				std::string strUrl = "http://" + printer.ipAddress.toStdString() + ":" + std::to_string(81) + "/protocal.csp";
				cpr::GetCallback(callback, cpr::Url{ strUrl }, cpr::Url{ strUrl }, cpr::Timeout{ 2000 }, cpr::Parameters{ {"fname", "net"},{"opt","iot_conf"},{"function","set"},{"ReqPrinterPara","0"} });

				strUrl = "http://" + printer.ipAddress.toStdString() + ":" + std::to_string(80) + "/info";
				cpr::GetCallback(callback, cpr::Url{ strUrl }, cpr::Url{ strUrl }, cpr::Timeout{ 2000 }, cpr::Parameters{ });
			}
				break;
			case RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER:
				m_pKlipperInterface->getMultiMachine(printer.ipAddress.toStdString(), printer.moonrakerPort, [=](std::vector<RemotePrinter>& printers) {
					for (const auto& printer : printers)
					{
						doAddPrinter(printer);
					}
				}
			);
				break;
			case RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408:
				doAddPrinter(printer);
				break;
			default:
				break;
		}
	}

	void RemotePrinterManager::tryAddPrinter(cpr::Response r, RemotePrinter printer)
	{
		if (r.status_code == 200)
		{
			rapidjson::Document doc;
			doc.Parse(r.text.c_str());
			if (doc.HasParseError())
			{
				return;
			}
			if (doc.HasMember("opt"))
			{
				printer.type = RemotePrinerType::REMOTE_PRINTER_TYPE_LAN;
				std::string strUrl = "http://" + printer.ipAddress.toStdString() + ":" + std::to_string(81) + "/protocal.csp";

				auto response = cpr::Get( cpr::Url{ strUrl },
						cpr::Parameters{ {"fname", "Info"},{"opt","main"},{"function","get"} });
				rapidjson::Document d;
				d.Parse(response.text.c_str());

				if (d.HasParseError())
					return;

				if (d.HasMember("ssid") && d["ssid"].IsString())
				{
					QString ssid = d["ssid"].GetString();
					QStringList strList = ssid.split("-");
					if (strList.size() > 1)
					{
						printer.macAddress = strList[1];
					}
				}
				doAddPrinter(printer);
			}
			else
			{
				printer.type = RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408;
				printer.macAddress = doc["mac"].GetString();
				doAddPrinter(printer);
			}
		}
	}

	void RemotePrinterManager::addPrinter(const QList<RemotePrinter>& listPrinter)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		foreach(RemotePrinter printer, listPrinter)
		{
			switch (printer.type)
			{
				case RemotePrinerType::REMOTE_PRINTER_TYPE_LAN:
					doAddPrinter(printer, false);
					break;
				case RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER:
					m_pKlipperInterface->getMultiMachine(printer.ipAddress.toStdString(), printer.moonrakerPort, [=](std::vector<RemotePrinter> & printers) {
							for (const auto& printer : printers)
							{
								doAddPrinter(printer, false);
							}
						}
					);
					break;
				case RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408:
					doAddPrinter(printer, false);
					break;
				default:
					break;
			}
		}
	}

	void RemotePrinterManager::doAddPrinter(const RemotePrinter& printer, bool lockEnabled)
	{
		if (printer.type == RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408)
		{
			auto infoCallback = [=](const RemotePrinter& printer) {
				if (m_pfnGetPrinterInfoCb)
				{
					if (printer.type != RemotePrinerType::REMOTE_PRINTER_TYPE_NONE)
					{
						m_pfnGetPrinterInfoCb(printer);
					}
				}
			};
			auto fileCallback = [=](const std::string& ipAddrClient, const std::string& fileList) {
				if (m_pfnGetGcodeListCb)
				{
					m_pfnGetGcodeListCb(ipAddrClient, fileList, RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408);
				}
			};

			auto historyCallback = [=](std::string ipAddrClient, std::string fileList) {
				if (m_pfnGetHistoryListCb)
				{
					m_pfnGetHistoryListCb(ipAddrClient, fileList, RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408);
				}
			};

			auto videoCallback = [=](const std::string& ipAddrClient, const std::string& fileList) {
				if (m_pfnGetVideoListCb)
				{
					m_pfnGetVideoListCb(ipAddrClient, fileList, RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408);
				}
			};

			m_pKlipper4408Interface->addClient(printer.ipAddress.toStdString(), printer.macAddress.toStdString(), 9999, infoCallback, fileCallback, historyCallback, videoCallback);
			return;
		}
		auto doAddPrinterFunc = [=]() {
			auto key = printer.uniqueId.toStdString();
			if (m_mapID2Printer.find(key) == m_mapID2Printer.end())
			{
				RemotePrinterSession session;
				session.type = printer.type;
				session.ipAddress = printer.ipAddress.toStdString();
				session.macAddress = printer.macAddress.toStdString();
				session.printerName = printer.printerName.toStdString();
				session.uniqueId = session.ipAddress.empty() ? session.macAddress : session.ipAddress;
				
				if (session.type == RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER)
				{
					session.port = printer.moonrakerPort;
					session.printerId = printer.printerId;
					session.fluiddPort = printer.fluiddPort;
					session.mainsailPort = printer.mainsailPort;
				}

				//if (m_pfnGetPrinterInfoCb) m_pfnGetPrinterInfoCb(printer);
				m_mapID2Printer[key] = session;
			}
		};

		//Diff interface
		if (lockEnabled)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			doAddPrinterFunc();
		}
		else {
			doAddPrinterFunc();
		}
	}

	std::string RemotePrinterManager::deletePrinter(const RemotePrinter& printer)
	{
		if (printer.type == RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408)
		{
			m_pKlipper4408Interface->removeClient(printer.uniqueId.toStdString());
			return printer.macAddress.toStdString();
		}
		std::lock_guard<std::mutex> lock(m_mutex);
		std::string key = printer.uniqueId.toStdString();
		std::string mac = printer.macAddress.toStdString();
		if (m_mapID2Printer.find(key) != m_mapID2Printer.end())
		{
			m_mapID2Printer.erase(key);
		}

		return mac;
	}

	void RemotePrinterManager::getPrinterInfo(RemotePrinter& printer)
	{

	}

	void RemotePrinterManager::pushFile(const RemotePrinter& printer, const std::string& fileName, const std::string& filePath, std::function<void(std::string, std::string, float)> callback, std::function<void(std::string, std::string, int)> errorCallback)
	{
		switch (printer.type)
		{
			case RemotePrinerType::REMOTE_PRINTER_TYPE_LAN:
			{
				QByteArray ba = printer.ipAddress.toLocal8Bit();
				m_pLanPrinterInterface->sendFileToDevice(ba.data(), fileName, filePath,
					[=](float progress) {
						if (callback)
						{
							callback(printer.macAddress.toStdString(), std::string(), progress);
						}
						if (qFuzzyCompare(progress, 1.0f))
						{
							const auto& id = printer.ipAddress.toStdString();
							m_mutex.lock();
							if (m_mapID2Printer.find(id) != m_mapID2Printer.cend())
							{
								m_mapID2Printer[id].tmLastGetFileList = 0;
							}
							m_mutex.unlock();
						}
					},
					[=](int errCode) {
						if (errorCallback)
						{
							errorCallback(printer.macAddress.toStdString(), std::string(), errCode);
						}
					}
				);
			}
				break;
			case RemotePrinerType::REMOTE_PRINTER_TYPE_OCTOPRINT:
			{
				m_pOctoPrinterInterface->sendFileToDevice(printer.ipAddress.toStdString(), printer.token.toStdString(), filePath);
			}
				break;
			case RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER:
			{
				m_pKlipperInterface->sendFileToDevice(printer.ipAddress.toStdString(), printer.moonrakerPort, fileName, filePath, 
					[=](float progress) {
						if (callback)
						{
							callback(printer.macAddress.toStdString(), std::string(), progress);
						}
					},
					[=](int errCode) {
						if (errorCallback)
						{
							errorCallback(printer.macAddress.toStdString(), std::string(), errCode);
						}
					}
				);
			}
				break;
			case RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408:
			{
				m_pKlipper4408Interface->sendFileToDevice(printer.ipAddress.toStdString(), 80, fileName, filePath,
					[=](float progress) {
						if (callback)
						{
							callback(printer.macAddress.toStdString(), fileName, progress);
						}
					},
					[=](int errCode) {
						if (errorCallback)
						{
							errorCallback(printer.macAddress.toStdString(), fileName, errCode);
						}
					}
				);
			}
				break;
			default:
				break;
		}
	}

	void RemotePrinterManager::getPreviewImg(const RemotePrinter& printer, const std::string& filePath)
	{
		switch (printer.type)
		{
		case RemotePrinerType::REMOTE_PRINTER_TYPE_LAN:
		{
			QByteArray ba = printer.ipAddress.toLocal8Bit();
			m_pLanPrinterInterface->fetchHead(ba.data(), filePath, [=](std::string type, std::string imgData) {
				if (m_pfnGetPreviewCb)
				{
					m_pfnGetPreviewCb(printer.macAddress.toStdString(), type, imgData);
				}
				});
		}
		break;
		case RemotePrinerType::REMOTE_PRINTER_TYPE_OCTOPRINT:
		{
			m_pOctoPrinterInterface->sendFileToDevice(printer.ipAddress.toStdString(), printer.token.toStdString(), filePath);
		}
		break;
		default:
			break;
		}
	}

	void RemotePrinterManager::deleteFile(const std::string& strIp, const std::string& value, RemotePrinerType printerType, const OpPrinerFileType& fileType)
	{
		switch (printerType)
		{
			case RemotePrinerType::REMOTE_PRINTER_TYPE_LAN:
			{
				m_pLanPrinterInterface->deleteFile(strIp, value);
			}
				break;
			case RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408:
			{
				switch (fileType)
				{
				case OpPrinerFileType::GCODE_FILE:
					m_pKlipper4408Interface->deleteGcodeFile(strIp, value);
					break;
				case OpPrinerFileType::HISTORY_FILE:
					m_pKlipper4408Interface->deleteHistoryFile(strIp, std::stoi(value));
					break;
				case OpPrinerFileType::VIDEO_FILE:
					m_pKlipper4408Interface->deleteVideoFile(strIp, value);
					break;
				default:
					break;
				}
			}
				break;
			default:
				break;
		}
	}

	void RemotePrinterManager::renameFile(const std::string& strIp, const std::string& value, const std::string& targetName, RemotePrinerType printerType, const OpPrinerFileType& fileType)
	{
		switch (printerType)
		{
		case RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408:
		{
			switch (fileType)
			{
			case OpPrinerFileType::VIDEO_FILE:
				m_pKlipper4408Interface->renameVideoFile(strIp, value, targetName);
				break;
			case OpPrinerFileType::GCODE_FILE:
				m_pKlipper4408Interface->renameGcodeFile(strIp, value, targetName);
				break;
			default:
				break;
			}
		}
		break;
		default:
			break;
		}
	}

	void RemotePrinterManager::controlPrinter(const RemotePrinter& printer, const PrintControlType& cmdType, const std::string& value)
	{
			switch (printer.type)
			{
			case RemotePrinerType::REMOTE_PRINTER_TYPE_LAN:
			{
				m_pLanPrinterInterface->controlPrinter(printer.ipAddress.toStdString(), cmdType, value, std::bind(&RemotePrinterManager::updateSessitonActive, this, printer.uniqueId.toStdString(),0));
			}
				break;
			case RemotePrinerType::REMOTE_PRINTER_TYPE_OCTOPRINT:
			{
				m_pOctoPrinterInterface->controlPrinter(printer.ipAddress.toStdString(), printer.token.toStdString(), cmdType, value);
			}
			break;
			case RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER:
			{
				m_pKlipperInterface->controlPrinter(printer.ipAddress.toStdString(), printer.moonrakerPort, cmdType, value);
			}
			break;
			case RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408:
			{
				m_pKlipper4408Interface->controlPrinter(printer.ipAddress.toStdString(), printer.moonrakerPort, cmdType, value);
			}
			break;
			default:
				break;
			}
			//updateSessitonActive(printer.uniqueId.toStdString());
	}

	void RemotePrinterManager::transparentCommand(const RemotePrinter& printer, const std::string& value)
	{
			switch (printer.type)
			{
			case RemotePrinerType::REMOTE_PRINTER_TYPE_LAN:
			{
				m_pLanPrinterInterface->transparentCommand(printer.ipAddress.toStdString(), value, std::bind(&RemotePrinterManager::updateSessitonActive, this, printer.uniqueId.toStdString(), 0));
			}
			break;
			case RemotePrinerType::REMOTE_PRINTER_TYPE_OCTOPRINT:
			{
				m_pOctoPrinterInterface->transparentCommand(printer.ipAddress.toStdString(), printer.token.toStdString(), value);
			}
			break;
			default:
				break;
			}
	}

	void RemotePrinterManager::downloadFile(const std::string& ipAddress, const std::string& filePath, std::function<void(const float&)> callback)
	{
		m_mtxDownload.lock();
		m_listDownloads.push_back(std::make_tuple(ipAddress, filePath, callback));
		m_mtxDownload.unlock();
	}

	void creative_kernel::RemotePrinterManager::downloadThread()
	{
		while (!m_bExit)
		{
			m_mtxDownload.lock();
			if (m_listDownloads.empty())
			{
				m_mtxDownload.unlock();
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}
			auto task = m_listDownloads.front();
			m_listDownloads.pop_front();
			m_mtxDownload.unlock();
			auto url = std::get<0>(task);
			auto filePath = std::get<1>(task);
			auto call_back = std::get<2>(task);

			cpr::Download(
				filePath,
				cpr::Url{ url },
				cpr::VerifySsl{ false },
				cpr::ProgressCallback{ [=](cpr::cpr_off_t download_total,
												cpr::cpr_off_t download_current,
												cpr::cpr_off_t upload_total,
												cpr::cpr_off_t upload_current,
												intptr_t userdata) -> bool {
						if (call_back && download_total) {
							call_back(download_current*1.0 / download_total);
}
						return true;
				} });
		}
	}

	void RemotePrinterManager::updateSessitonActive(const std::string& uuid, const time_t& tmActive)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_mapID2Printer.find(uuid) != m_mapID2Printer.end())
		{
			m_mapID2Printer[uuid].tmLastActive = tmActive;
		}
	}
}