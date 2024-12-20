#include <QDebug>
#include <QFileInfo>
#include <QFileDialog>
#include <QStringList>

#include "cuslistmodel.h"
#include "kernel/kernel.h"
#include "slice/sliceflow.h"
#include "interface/uiinterface.h"
#include "qtusercore/string/resourcesfinder.h"
#include "interface/machineinterface.h"
#include "interface/appsettinginterface.h"
#include "filelistmodel.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "localnetworkinterface/materialboxmodellist.h"
#include "sortproxymodel.h"
#include "localnetplugin.h"
using namespace creative_kernel;

/******************** CusListModel ********************/
CusListModel::CusListModel(QObject *parent) 
	: QAbstractListModel(parent)
	, m_previewProvider(new ImgProvider(this))
{
	m_ListPrinterItem.clear();
	m_deviceGroupNames << "Group1";
	m_appDataPath = qtuser_core::getOrCreateAppDataLocation(QStringLiteral("../"));//QProcessEnvironment::systemEnvironment().value("appdata");

	m_posRegExp = QRegExp("X:([\\-]?[\\d]+(?:[\\.][\\d]+)?)?[\\s]Y:([\\-]?[\\d]+(?:[\\.][\\d]+)?)[\\s]Z:([\\-]?[\\d]+(?:[\\.][\\d]+)?)");
	m_ipv4RegExp = QRegExp("((2(5[0-5]|[0-4][\\d]))|[0-1]?[\\d]{1,2})([\\.]((2(5[0-5]|[0-4][\\d]))|[0-1]?[\\d]{1,2})){3}");//ipv4

	readConfig();
}

CusListModel::~CusListModel()
{
	getRemotePrinterManager()->setGetPrinterInfoCb(nullptr);
	getRemotePrinterManager()->setGetPreviewCb(nullptr);

	this->disconnect();
	//writeConfig();
}

void CusListModel::initialize()
{
	registerImageProvider(QLatin1String("preview"), m_previewProvider);
}

void CusListModel::uninitialize()
{
	removeImageProvider(QLatin1String("preview"));
}

void CusListModel::onGetInfoSuccess(const RemotePrinter& stateInfo)
{
	emit sigGetInfoSuccess(stateInfo);
}

void CusListModel::onGetPreviewImage(const std::string& std_macAddr, const std::string& std_imgType, const std::string& std_imgData)
{
	if (std_macAddr.empty() || std_imgType.empty() || std_imgData.empty()) return;
	QString from_std_macAddr = QString::fromStdString(std_macAddr);
	//QString from_std_imgType = QString::fromStdString(std_imgType);
	QString from_std_imgData = QString::fromStdString(std_imgData);
	from_std_imgData.remove(QRegExp("[;]|[\\s]"));

	//qDebug() << "from_std_imgData" << from_std_imgData;
	QByteArray from_base64_imgData = QByteArray::fromBase64(from_std_imgData.toUtf8());
	QImage previewImage = QImage::fromData(from_base64_imgData, std_imgType.c_str());
	
	if (!previewImage.isNull()) emit sigGetPreviewImage(from_std_macAddr, previewImage);
}

void CusListModel::findDeviceList()
{
	//qDebug() << "findDeviceList:";
	if (m_firstRun) {
		connect(this, &CusListModel::sigGetInfoSuccess, this, &CusListModel::slotGetInfoSuccess, Qt::ConnectionType::QueuedConnection);
		connect(this, &CusListModel::sigGetPreviewImage, this, &CusListModel::slotGetPreviewImage, Qt::ConnectionType::QueuedConnection);

		getRemotePrinterManager()->setGetPrinterInfoCb(std::bind(&CusListModel::onGetInfoSuccess, this, std::placeholders::_1));
		getRemotePrinterManager()->setGetPreviewCb(std::bind(&CusListModel::onGetPreviewImage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		if (!m_MapDeviceInfo.isEmpty())
		{
			addLocalDevice();
		}
		m_firstRun = false;
	}
	//getRemotePrinterManager()->searchDevices();
}

void CusListModel::deleteConnect(const QString& deviceMacAddr)
{
	//qDebug() << "删除设备:" << deviceIpAddr;
	ListModelItem* item = getDeviceItem(deviceMacAddr);
	if (item) 
	{
		RemotePrinter printer;
		printer.uniqueId = item->pcIpAddr();
		printer.macAddress = item->pcPrinterID();
		printer.type = (RemotePrinerType)item->printerType();
		if (printer.type == RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER) printer.uniqueId += QString("_%1").arg(item->multiNumber());

		item->setPcHasSelect(false);//unchecked
		auto watcher = new QFutureWatcher<std::string>();
		connect(watcher, &QFutureWatcher<void>::finished, this, &CusListModel::slotDeleteFinished, Qt::ConnectionType::QueuedConnection);

		//start thread
		QFuture<std::string> future = QtConcurrent::run([=]() {return getRemotePrinterManager()->deletePrinter(printer); });
		watcher->setFuture(future);
	}
}

void CusListModel::createConnect(const QString & deviceIpAddr, const int& type)
{
	m_bIsConnecting = true;
	m_curConnectingIpAddr = deviceIpAddr;
	m_isConnectingType = (RemotePrinerType)type;
	//qDebug() << "添加设备:" << deviceIpAddr;

	QFuture<void> future = QtConcurrent::run([=]() {
		RemotePrinter printer;
		printer.uniqueId = deviceIpAddr;
		printer.ipAddress = deviceIpAddr;
		printer.type = (RemotePrinerType)type;
		getRemotePrinterManager()->addPrinter(printer);
	});
}

void CusListModel::sendUIControlCmdToDevice(QString deviceIpAddr, int nCmdType, QString value, int printerType, int moonrakerPort)
{
	RemotePrinter printer;
	printer.type = (RemotePrinerType)printerType;
	printer.uniqueId = deviceIpAddr;
	printer.ipAddress = deviceIpAddr;
	printer.moonrakerPort = moonrakerPort;

	getRemotePrinterManager()->controlPrinter(printer, (PrintControlType)nCmdType, value.toUtf8().toStdString());
	//qDebug() << "sendUIControlCmdToDevice" << deviceIpAddr << nCmdType << value;
}

void CusListModel::printDeviceGCode(const QString&strMac, const QString& filePath, const QList<QVariantMap>& colorMap)
{
	PrintControlType cmdType;
	auto path = filePath;
	QString macAddress = strMac;
	ListModelItem* item = getDeviceItem(macAddress);
	if(colorMap.size() > 0){
		cmdType = item->enableSelfTest() ? PrintControlType::CONTROL_CMD_MULTICOLORPRINT_WITH_CALIBRATION :PrintControlType::CONTROL_CMD_MULTICOLORPRINT;
		getRemotePrinterManager()->materialColorMap(item->pcIpAddr(),path,colorMap);
	} else {
		cmdType = item->enableSelfTest() ? PrintControlType::PRINT_WITH_CALIBRATION : PrintControlType::PRINT_START;
	}
				
	sendUIControlCmdToDevice(item->pcIpAddr(), (int)cmdType, path, (item->printerType()),item->moonrakerPort());//Auto Start
			
}
void CusListModel::batch_reset()
{
	foreach(auto pair, m_ListPrinterItem)
	{
		auto item = pair.second;
		item->setGCodeTransProgress(0);
	}
}

bool CusListModel::checkOnePrint()
{
	bool hasPrinting = false;
	foreach(auto pair, m_ListPrinterItem)
	{
		auto item = pair.second;
		if (item->pcHasSelect() && item->pcPrinterState() == 1)
		{
			hasPrinting = true;
			break;
		}
	}
	return hasPrinting;
}

bool CusListModel::hasDeviceOnline()
{
	bool hasOnline = false;
	foreach(auto pair, m_ListPrinterItem)
	{
		auto item = pair.second;
		if (item->pcPrinterStatus() == 1)
		{
			hasOnline = true;
			break;
		}
	}
	return hasOnline;
}




void CusListModel::checkAutoPrint(bool auto_check)
{
	QSettings settings;
	settings.beginGroup(QStringLiteral("localnet"));
	settings.setValue(QStringLiteral("auto_check_one_print"), auto_check);
	settings.endGroup();
}

bool CusListModel::showAutoPrintTip()
{
	QSettings settings;
	settings.beginGroup(QStringLiteral("localnet"));
	auto auto_check = settings.value(QStringLiteral("auto_check_one_print"), false).toBool();
	settings.endGroup();
	return !auto_check;
}

void CusListModel::setOneSelected(QString ip)
{
	foreach(auto pair, m_ListPrinterItem)
	{
		auto item = pair.second;
		item->setPcHasSelect( (item->pcIpAddr() == ip) ? 1 : 0);
	}
}

QString CusListModel::getOneProgress(QString ip)
{
	QString progress = "";
	foreach(auto pair, m_ListPrinterItem)
	{
		auto item = pair.second;
		if (item->pcIpAddr() == ip) {
			return QString::number(item->gCodeTransProgress());
		}
	}
	return "";
}

QObject* CusListModel::getDeviceItemByIp(const QString& ip)
{
	foreach(auto pair, m_ListPrinterItem)
	{
		auto item = pair.second;
		if (item->pcIpAddr() == ip) {
			return item;
		}
	}
	return nullptr;
}
QString CusListModel::getPinterErrorCode(QString ip)
{
	QString progress = "";
	foreach(auto pair, m_ListPrinterItem)
	{
		auto item = pair.second;
		if (item->pcIpAddr() == ip) {
			return QString::number(item->errorCode());
		}
	}
	return "";
}
QString CusListModel::getPinterErrorMsg(QString ip)
{
	QString progress = "";
	foreach(auto pair, m_ListPrinterItem)
	{
		auto item = pair.second;
		if (item->pcIpAddr() == ip) {
			return item->errorMessage();
		}
	}
	return "";
}

QString CusListModel::getPinterErrorKey(QString ip)
{
	QString progress = "";
	foreach(auto pair, m_ListPrinterItem)
	{
		auto item = pair.second;
		if (item->pcIpAddr() == ip) {
			return QString::number(item->errorKey());
		}
	}
	return "";
}

QString CusListModel::getPrinterType(QString ip)
{
	QString progress = "";
	foreach(auto pair, m_ListPrinterItem)
	{
		auto item = pair.second;
		if (item->pcIpAddr() == ip) {
			return QString::number(item->printerType());
		}
	}
	return "";
}

int CusListModel::getPinterStatus(QString ip)
{
	foreach(auto pair, m_ListPrinterItem)
	{
		auto item = pair.second;
		if (item->pcIpAddr() == ip) {
			return item->pcPrinterStatus();
		}
	}
	return 0;
}

int CusListModel::getPinterState(QString ip)
{
	foreach(auto pair, m_ListPrinterItem)
	{
		auto item = pair.second;
		if (item->pcIpAddr() == ip) {
			return item->pcPrinterState();
		}
	}
	return 0;
}

QString CusListModel::getPinterName(QString ip)
{
	foreach(auto pair, m_ListPrinterItem)
	{
		auto item = pair.second;
		if (item->pcIpAddr() == ip) {
			return item->printerModelName();
		}
	}
	return "";
}

void CusListModel::setSyncPrinter(QString ip)
{
	foreach(auto pair, m_ListPrinterItem)
	{
		auto item = pair.second;
		if (item->pcIpAddr() == ip) {
			item->setSync(true);
		}
		else {
			item->setSync(false);
		}
	}
	m_syncIp = ip;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, 0), { ListModelItem::E_Sync });

}


int CusListModel::getSyncPrinter()
{
	int currentIndex = 0;
	for (auto i = 0; i < m_ListPrinterItem.size(); i++)
	{
		auto item = m_ListPrinterItem.at(i).second;
		if (item->pcIpAddr() == m_syncIp) {
			currentIndex = i;
			break;
		}

	}
	return currentIndex;
}




void CusListModel::batch_command(const QString& fileName, const QString& filePath, int plate, const QList<QVariantMap>& colorMap)
{
	m_sendOnly = false;
	m_curGcodeFileName = fileName;
	m_multiColor = colorMap;
	//QString filePath = getKernel()->sliceFlow()->currentGCodeImageFileName();
	//qDebug() << "m_curGcodeFileName" << m_curGcodeFileName << "filePath" << filePath;
	//plate_1
	m_printPath = QString(".%1/%2_plate_%3.gcode").arg(fileName).arg(fileName.mid(0, fileName.size()-4)).arg(plate);
	foreach(auto pair, m_ListPrinterItem)
	{
		auto item = pair.second;
		if (item->pcHasSelect())
		{
			//if (item->sentFileName() == fileName)
			//{
			//	if (!sendOnly)
			//	{
			//		PrintControlType cmdType = item->enableSelfTest() ? PrintControlType::PRINT_WITH_CALIBRATION : PrintControlType::PRINT_START;
			//		sendUIControlCmdToDevice(item->pcIpAddr(), (int)cmdType, fileName, (item->printerType()), item->moonrakerPort());
			//	}
			//	continue;
			//}
			RemotePrinter printer;
			item->setPcHasSelect(false);
			printer.ipAddress = item->pcIpAddr();
			printer.macAddress = item->pcPrinterID();
			printer.moonrakerPort = item->moonrakerPort();
			printer.type = RemotePrinerType(item->printerType());

			getRemotePrinterManager()->uploadFile(printer, m_curGcodeFileName.toStdString(), filePath.toStdString(),
			std::bind(&CusListModel::UploadProgressCallBack, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
			std::bind(&CusListModel::UploadMessageCallBack, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		}
	}
}

void CusListModel::batch_command(const QString& fileName, const QString& filePath, bool sendOnly, const QList<QVariantMap>& colorMap)
{
	m_sendOnly = sendOnly;
	m_curGcodeFileName = fileName;
	m_multiColor = colorMap;
	m_printPath = fileName;
	//QString filePath = getKernel()->sliceFlow()->currentGCodeImageFileName();
	//qDebug() << "m_curGcodeFileName" << m_curGcodeFileName << "filePath" << filePath;

	foreach(auto pair, m_ListPrinterItem)
	{
		auto item = pair.second;
		if (item->pcHasSelect())
		{
			//if (item->sentFileName() == fileName)
			//{
			//	if (!sendOnly)
			//	{
			//		PrintControlType cmdType = item->enableSelfTest() ? PrintControlType::PRINT_WITH_CALIBRATION : PrintControlType::PRINT_START;
			//		sendUIControlCmdToDevice(item->pcIpAddr(), (int)cmdType, fileName, (item->printerType()), item->moonrakerPort());
			//	}
			//	continue;
			//}
			RemotePrinter printer;
			item->setPcHasSelect(false);
			printer.ipAddress = item->pcIpAddr();
			printer.macAddress = item->pcPrinterID();
			printer.moonrakerPort = item->moonrakerPort();
			printer.type = RemotePrinerType(item->printerType());

			getRemotePrinterManager()->uploadFile(printer, m_curGcodeFileName.toStdString(), filePath.toStdString(),
			std::bind(&CusListModel::UploadProgressCallBack, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
			std::bind(&CusListModel::UploadMessageCallBack, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		}
	}
}

void CusListModel::reloadFile(const QString& macAddress)
{
	ListModelItem* item = getDeviceItem(macAddress);
	QString filePath = getKernel()->sliceFlow()->currentGCodeImageFileName();
	//qDebug() << "m_curGcodeFileName" << m_curGcodeFileName << "filePath" << filePath;
	
	if (item)
	{
		RemotePrinter printer;
		printer.macAddress = macAddress;
		printer.ipAddress = item->pcIpAddr();
		printer.moonrakerPort = item->moonrakerPort();
		printer.type = RemotePrinerType(item->printerType());

		getRemotePrinterManager()->uploadFile(printer, m_curGcodeFileName.toStdString(), filePath.toStdString(),
		std::bind(&CusListModel::UploadProgressCallBack, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&CusListModel::UploadMessageCallBack, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	}
}

void CusListModel::importFile(const QString& macAddress, const QList<QUrl>& fileUrls)
{
	ListModelItem* item = getDeviceItem(macAddress);
	if (item == nullptr || fileUrls.isEmpty()) return;

	RemotePrinter printer;
	printer.macAddress = macAddress;
	printer.ipAddress = item->pcIpAddr();
	printer.type = RemotePrinerType(item->printerType());

	for (const auto& filePath : fileUrls)
	{
		getRemotePrinterManager()->uploadFile(printer, std::string(), filePath.toLocalFile().toStdString(),
			std::bind(&CusListModel::ImportProgressCallBack, this, std::placeholders::_1, filePath.toLocalFile().toStdString(), std::placeholders::_3),
			std::bind(&CusListModel::ImportMessageCallBack, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	}
}
bool CusListModel::findDeviceError()
{
	bool find = false;
	foreach(auto pair, m_ListPrinterItem)
	{
		auto item = pair.second;
		if (item->errorCode() != 0)
		{
			find = true;
			break;
		}
	}
	return find;
}

QStringList CusListModel::getGroups()
{
    return m_deviceGroupNames;
}

QStringList CusListModel::pasteContent()
{
	QString clipText = QGuiApplication::clipboard()->text();
	if (m_ipv4RegExp.exactMatch(clipText)) 
	{
		QStringList ipAddrList = clipText.split(".");
		return ipAddrList;
	}

	return (QStringList() << clipText);
}

void CusListModel::copyContent(const QString& deviceIpAddr)
{
	QGuiApplication::clipboard()->setText(deviceIpAddr);
}

void CusListModel::addGroup(const QString& addGroupName)
{
	m_deviceGroupNames.append(addGroupName);
    //qDebug()<<"新建编组"<<addGroupName;
}

void CusListModel::deleteGroup(const QString& delGroupName)
{
    //删除分组
    int groupIndex = m_deviceGroupNames.indexOf(delGroupName);
	if (groupIndex < 0) return; m_deviceGroupNames.removeAt(groupIndex);

    //设备移入默认分组
	foreach(auto pair, m_ListPrinterItem)
	{
		auto item = pair.second;
		if (item->pcGroup() == (groupIndex + 1))
		{
			editDeviceGroup(item->pcPrinterID(), 1);
		}
	}
}

void CusListModel::editGroupName(int groupIndex, const QString& newGroupName)
{
	m_deviceGroupNames[groupIndex] = newGroupName;
    //qDebug()<<"修改组名"<<groupIndex<<newGroupName;
}

void CusListModel::editDeviceGroup(const QString& macAddress, int newGroupNo)
{
	if (m_MapDeviceInfo.contains(macAddress))
	{
		DeviceInfo deviceInfo = m_MapDeviceInfo[macAddress];
		deviceInfo.m_group = newGroupNo;//修改设备分组
		m_MapDeviceInfo[macAddress] = deviceInfo;
		writeConfig();
	}

	ListModelItem* item = getDeviceItem(macAddress);
	if (item)
	{
		item->setPcGroup(newGroupNo);
		if (item->pcHasSelect()) item->setPcHasSelect(0);//取消选中
	}

	QVector<int> roles = QVector<int>() << ListModelItem::E_GroupNo << ListModelItem::E_HasSelect;
	onDataChanged(macAddress, roles);//Update Data
}

void CusListModel::editDeviceName(const QString& macAddress, const QString& newDeviceName)
{
	if (m_MapDeviceInfo.contains(macAddress) && !newDeviceName.isEmpty())
	{
		DeviceInfo deviceInfo = m_MapDeviceInfo[macAddress];
		deviceInfo.m_deviceName = newDeviceName;//修改设备名称
		m_MapDeviceInfo[macAddress] = deviceInfo;
		writeConfig();
	}
}

qint64 CusListModel::getCurrentUtcTime()
{
	return QDateTime::currentMSecsSinceEpoch();
}

QDateTime CusListModel::getCurrentDateTime(bool min)
{
	QDateTime dateTime = QDateTime::currentDateTime();
	return (min ? dateTime : dateTime.addSecs(60));
}

ListModelItem *CusListModel::getDeviceItem(const QString &macAddress)
{
	int rowIndex = getItemRowIndex(macAddress);
	ListModelItem* item = (rowIndex != -1) ? m_ListPrinterItem[rowIndex].second : nullptr;
	return item;
}

QObject *CusListModel::getDeviceObject(const int index)
{
	if (index < 0 || index>=m_ListPrinterItem.length() )
	{
		if(m_ListPrinterItem.isEmpty())
			return nullptr;
		else
		 return m_ListPrinterItem.at(0).second;
	}
	
    auto item=m_ListPrinterItem.at(index).second;
	return item;
}


int CusListModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent)
	return m_ListPrinterItem.count();
}

QVariant CusListModel::data(const QModelIndex& index, int role) const
{
	if (index.row() < 0 || index.row() >= m_ListPrinterItem.count())
		return QVariant();

	QVariant res;
	auto pair = m_ListPrinterItem.at(index.row());
	ListModelItem* modelItem = pair.second;
	QString defaultDir = "";
	QString image_url = "";
	switch (role)
	{
		case ListModelItem::E_PrinterID:
			res = modelItem->pcPrinterID();
			break;
		case ListModelItem::E_ErrorKey:
			res = modelItem->errorKey();
			break;
		case ListModelItem::E_ErrorCode:
			res = modelItem->errorCode();
			break;
		case ListModelItem::E_FluiddPort:
			res = modelItem->fluiddPort();
			break;
		case ListModelItem::E_PrinterType:
			res = modelItem->printerType();
			break;
		case ListModelItem::E_MultiNumber:
			res = modelItem->multiNumber();
			break;
		case ListModelItem::E_MainsailPort:
			res = modelItem->mainsailPort();
			break;
		case ListModelItem::E_PrinterModel:
			res = modelItem->pcPrinterModel();
			break;
		case ListModelItem::E_PrinterModelName:
			res = modelItem->printerModelName();
			break;
		case ListModelItem::E_PrinterState:
			res = modelItem->pcPrinterState();
			break;
		case ListModelItem::E_PrinterStatus:
			res = modelItem->pcPrinterStatus();
			break;
		case ListModelItem::E_PrinterMethod:
			res = modelItem->pcPrinterMethod();
			break; 
		case ListModelItem::E_HasCamera:
			res = modelItem->pcHasCamera();
			break;
		case ListModelItem::E_IpAddr:
			res = modelItem->pcIpAddr();
			break;
		case ListModelItem::E_GCodeName:
			res = modelItem->pcGCodeName();
			break;
		case ListModelItem::E_TotalPrintTime:
			res = modelItem->pcTotalPrintTime();
			break;
		case ListModelItem::E_LeftTime:
			res = modelItem->pcLeftTime();
			break;
		case ListModelItem::E_CurPrintLayer:
			res = modelItem->pcCurPrintLayer();
			break;
		case ListModelItem::E_TotalPrintLayer:
			res = modelItem->pcTotalPrintLayer();
			break;
		case ListModelItem::E_CurPrintProgress:
			res = modelItem->pcCurPrintProgress();
			break;
		case ListModelItem::E_BedTemp:
			res = modelItem->pcBedTemp();
			break;
		case ListModelItem::E_NozzleTemp:
			res = modelItem->pcNozzleTemp();
			break;
		case ListModelItem::E_GroupNo:
			res = modelItem->pcGroup();
			break;
		case ListModelItem::E_SentFileName:
			res = modelItem->sentFileName();
			break;
		case ListModelItem::E_ErrorMessage:
			res = modelItem->errorMessage();
			break;
		case ListModelItem::E_TransErrorMessage:
			res = modelItem->transErrorMessage();
			break;

		case ListModelItem::E_SendingFileName:
			res = modelItem->sendingFileName();
			break;
		case ListModelItem::E_MateialBoxList:
			res = QVariant::fromValue(modelItem->materialBoxList());// modelItem->materialBoxList();
			break;
		case ListModelItem::E_FilePrefixPath:
			res = modelItem->filePrefixPath();
			break;
		case ListModelItem::E_Sync:
			res = modelItem->sync();
			break;
		case ListModelItem::E_WebrtcSupport:
			res = modelItem->webrtcSupport();
			break;
		case ListModelItem::E_FeedState:
			res = modelItem->feedState();
			break;
		case ListModelItem::E_CfsConnect:
			res = modelItem->cfsConnect();
			break;
		//需要双向交互的数据直接传递对象指针
		case ListModelItem::E_HasSelect:
		case ListModelItem::E_EnableSelfTest:
		case ListModelItem::E_DeviceName:
		case ListModelItem::E_PrintSpeed:
		case ListModelItem::E_BedDstTemp:
		case ListModelItem::E_NozzleDstTemp:
		//case ListModelItem::E_X:
		//case ListModelItem::E_Y:
		//case ListModelItem::E_Z:
		//case ListModelItem::E_XY_Autohome:
		//case ListModelItem::E_Z_Autohome:
		//case ListModelItem::E_FanOpened:
		//case ListModelItem::E_CavityFan:
		//case ListModelItem::E_AssistFan:
		//case ListModelItem::E_LedOpened:
		case ListModelItem::E_TransProgress:
			res = QVariant::fromValue(modelItem);
			break;
		case ListModelItem::E_PrinterImageUrl:
			defaultDir = qtuser_core::getOrCreateAppDataLocation(getEnginePathPrefix());
			image_url = QUrl::fromLocalFile(QString(QStringLiteral("%1/default/machineImages/%2.png")).arg(defaultDir).arg(modelItem->pcPrinterModel())).toString();
			res = image_url;
			break;
		case ListModelItem::E_RemoteMaterial:
			break;
		case ListModelItem::E_RemoteGcodeList:
			res = QVariant::fromValue(modelItem->filelistModel());
			break;
		default:
			break;
	}
	return res;
}

QHash<int, QByteArray> CusListModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[ListModelItem::E_PrinterID] = "printerID";
	roles[ListModelItem::E_PrinterModel] = "printerModel";
	roles[ListModelItem::E_ErrorKey] = "errorKey";
	roles[ListModelItem::E_ErrorCode] = "errorCode";
	roles[ListModelItem::E_FluiddPort] = "fluiddPort";
	roles[ListModelItem::E_PrinterType] = "printerType";
	roles[ListModelItem::E_MultiNumber] = "multiNumber";
	roles[ListModelItem::E_MainsailPort] = "mainsailPort";
	roles[ListModelItem::E_PrinterState] = "printerState";
	roles[ListModelItem::E_PrinterStatus] = "printerStatus"; 
	roles[ListModelItem::E_PrinterMethod] = "printerMethod";
	roles[ListModelItem::E_HasCamera] = "hasCamera";
	roles[ListModelItem::E_HasSelect] = "mItem"; //select
	roles[ListModelItem::E_EnableSelfTest] = "mItem"; //enableSelfTest
	roles[ListModelItem::E_DeviceName] = "mItem"; //deviceName
	roles[ListModelItem::E_IpAddr] = "ipAddr";
	roles[ListModelItem::E_GCodeName] = "gCodeName";
	roles[ListModelItem::E_TotalPrintTime] = "totalPrintTime";
	roles[ListModelItem::E_LeftTime] = "leftTime";
	roles[ListModelItem::E_CurPrintLayer] = "curPrintLayer";
	roles[ListModelItem::E_TotalPrintLayer] = "totalPrintLayer";
	roles[ListModelItem::E_CurPrintProgress] = "curPrintProgress";
	roles[ListModelItem::E_PrintSpeed] = "mItem"; //printSpeed
	roles[ListModelItem::E_BedDstTemp] = "mItem"; //bedDstTemp
	roles[ListModelItem::E_NozzleDstTemp] = "mItem"; //nozzleDstTemp
	roles[ListModelItem::E_BedTemp] = "bedTemp";
	roles[ListModelItem::E_NozzleTemp] = "nozzleTemp";
	roles[ListModelItem::E_GroupNo] = "groupNo";
	//roles[ListModelItem::E_X] = "mItem";
	//roles[ListModelItem::E_Y] = "mItem";
	//roles[ListModelItem::E_Z] = "mItem";
	//roles[ListModelItem::E_XY_Autohome] = "mItem";
	//roles[ListModelItem::E_Z_Autohome] = "mItem";
	//roles[ListModelItem::E_FanOpened] = "mItem";
	//roles[ListModelItem::E_CavityFan] = "mItem";
	//roles[ListModelItem::E_AssistFan] = "mItem";
	//roles[ListModelItem::E_LedOpened] = "mItem";
	roles[ListModelItem::E_TransProgress] = "mItem";//progress
	roles[ListModelItem::E_SentFileName] = "sentFileName";
	roles[ListModelItem::E_ErrorMessage] = "errorMessage";
	roles[ListModelItem::E_TransErrorMessage] = "transErrorMessage";
	roles[ListModelItem::E_SendingFileName] = "sendingFileName";
	roles[ListModelItem::E_PrinterImageUrl] = "printerImageUrl";
	roles[ListModelItem::E_MateialBoxList] = "materialList";
	roles[ListModelItem::E_FilePrefixPath] = "filePrefixPath";
	roles[ListModelItem::E_WebrtcSupport] = "webrtcSupport";
	roles[ListModelItem::E_FeedState] = "feedState";
	roles[ListModelItem::E_CfsConnect] = "cfsConnect";
	roles[ListModelItem::E_Sync] = "sync";
	roles[ListModelItem::E_RemoteMaterial] = "remoteMaterial";
	roles[ListModelItem::E_RemoteGcodeList] = "remoteGcodeList";
	return roles;
}

QString CusListModel::getErrorMsgFromCode(int errorCode)
{
	if (Cpr_ErrorMsg.contains(errorCode))
	{
		return Cpr_ErrorMsg.value(errorCode);
	}
	return QString();
}

QString CusListModel::getPrinterNameFromCode(QString codeName)
{
	return creative_kernel::getMachineCodeName(codeName);
}

QString CusListModel::getUniqueNameFromCode(QString codeName)
{
	return creative_kernel::getMachineUniqueName(codeName);
}

QString CusListModel::getErrorMsgFromCode(int errorCode, RemotePrinerType type)
{
	if (type == RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408 && Klipper4408_ErrorMsg.contains(errorCode))
	{
		return QCoreApplication::translate("Klipper4408_ErrorMsg", Klipper4408_ErrorMsg.value(errorCode).toLatin1().data());
	}
	return QString();
}

void CusListModel::slotDeleteFinished()
{
	auto watcher = static_cast<QFutureWatcher<std::string>*>(sender());
	QString from_std_macAddr = QString::fromStdString(watcher->result());
	int rowIndex = getItemRowIndex(from_std_macAddr);
	delete watcher;//free memory

	if (!from_std_macAddr.isEmpty() && rowIndex != -1)
	{
		beginRemoveRows(QModelIndex(), rowIndex, rowIndex);
		m_MapDeviceInfo.remove(from_std_macAddr);
		m_ListPrinterItem.removeAt(rowIndex);
		endRemoveRows();
	}
	writeConfig();
}

void CusListModel::slotGetInfoSuccess(const RemotePrinter& stateInfo)
{
	int connectType = int(stateInfo.type);
	int connectPort = stateInfo.moonrakerPort;

	QString ipAddress = stateInfo.ipAddress;
	QString macAddress = stateInfo.macAddress;
	QString printerModel = stateInfo.printerName.simplified();
	if (printerModel == "K1" || printerModel == "K1 Max")
	{
		printerModel = "CR-"+printerModel;
	}
	QString printerModelName = getPrinterNameFromCode(stateInfo.printerName.simplified());
	QString printerModelUniqueName = getUniqueNameFromCode(stateInfo.printerName.simplified());
	QString errorMessage = getErrorMsgFromCode(stateInfo.errorCode, stateInfo.type);

	int rowIndex = getItemRowIndex(macAddress);
	ListModelItem* item = (rowIndex < 0) ? new ListModelItem(this) : m_ListPrinterItem[rowIndex].second;

	//手动添加设备,type = 0 : 表示打印机。 2：表示音速屏
	int printerType = (stateInfo.type == RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER) ? 2 : 0;
	//这个处理是为了 区分音速屏IP。 音速屏的IP只能通过界面上添加音速屏才能添加成功
	if (m_bIsConnecting && ipAddress == m_curConnectingIpAddr  && (int)m_isConnectingType == printerType)
	{
		m_bIsConnecting = false;
		m_curConnectingIpAddr.clear();
		emit sigConnectedIpSuccessed();
	}

	//如果ip存在，更新mac地址
	if (m_MapDeviceInfo.contains(macAddress))
	{
		DeviceInfo& infoDevice = m_MapDeviceInfo[macAddress];
		if (infoDevice.m_ipAddress != ipAddress)
		{
			deleteConnect(macAddress);
			return;
		}
	}
	else
	{
		for(const auto& macAddress : m_MapDeviceInfo.keys())
		{
			DeviceInfo& infoDevice = m_MapDeviceInfo[macAddress];
			if (infoDevice.m_ipAddress == ipAddress)
			{
				deleteConnect(macAddress);
				return;
			}
		}
	}

	//记录设备信息
	if (!m_MapDeviceInfo.contains(macAddress))
	{
		DeviceInfo deviceInfo;
		deviceInfo.m_connectType = connectType;
		deviceInfo.m_moonrakerPort = connectPort;
		deviceInfo.m_ipAddress = ipAddress;
		deviceInfo.m_modelName = printerModel;
		deviceInfo.m_deviceName = printerModel;
		m_MapDeviceInfo[macAddress] = deviceInfo;
		if (!printerModel.isEmpty())
		{
			writeConfig();
		}
	}
	else
	{
		bool needWrite = false;
		DeviceInfo& deviceInfo = m_MapDeviceInfo[macAddress];
		if (!printerModel.isEmpty() && printerModel != deviceInfo.m_modelName || deviceInfo.m_deviceName.isEmpty())
		{
			deviceInfo.m_modelName = printerModel;
			deviceInfo.m_deviceName = printerModel;
			needWrite = true;
		}
		if (deviceInfo.m_ipAddress != ipAddress)
		{
			deviceInfo.m_ipAddress = ipAddress;
			needWrite = true;
		}
		if (needWrite)
		{
			writeConfig();
		}
	}
	const DeviceInfo& deviceInfo = m_MapDeviceInfo[macAddress];
	item->m_roles = QVector<int>();
	item->setPcIpAddr(ipAddress);
	item->setPcPrinterID(macAddress);
	item->setPrinterType(connectType);
	item->setPcPrinterModel(printerModel);
	item->setPrinterModelName(printerModelName);
	if (stateInfo.type == RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408)
	{
		item->setDeviceName(stateInfo.deviceName);
	}
	else
	{
		DeviceInfo& deviceInfo = m_MapDeviceInfo[macAddress];
		item->setDeviceName(deviceInfo.m_deviceName);
	}
	item->setErrorKey(stateInfo.errorKey);
	item->setErrorCode(stateInfo.errorCode);
	item->setFluiddPort(stateInfo.fluiddPort);
	item->setMultiNumber(stateInfo.printerId);
	item->setMainsailPort(stateInfo.mainsailPort);
	item->setMoonrakerPort(stateInfo.moonrakerPort);//No roles
	item->setPcPrinterState(stateInfo.printState);
	if (!(m_curTransingMac == macAddress && m_isTransing))
	{
		//is transing,no change status
		item->setPcPrinterStatus(stateInfo.printerStatus);
	}

	
	item->setPcPrinterMethod(stateInfo.printMode);
	item->setPcHasCamera(stateInfo.video);
	item->setPcGCodeName(stateInfo.printFileName);
	item->setPcTotalPrintTime(stateInfo.printJobTime);
	item->setPcLeftTime(stateInfo.printLeftTime);
	item->setPcCurPrintLayer(stateInfo.layer);
	item->setPcTotalPrintLayer(stateInfo.totalLayer);
	item->setPcCurPrintProgress(stateInfo.printProgress);
	item->setPcPrintSpeed(stateInfo.printSpeed);
	item->setPcBedDstTemp(stateInfo.bedTempTarget);
	item->setPcNozzleDstTemp(stateInfo.nozzleTempTarget);
	//离线的时候 不更新温度值。 
	if (stateInfo.bedTemp != 0)
		item->setPcBedTemp(stateInfo.bedTemp);
	if (stateInfo.nozzleTemp != 0)
		item->setPcNozzleTemp(stateInfo.nozzleTemp);

	item->setFanOpened(stateInfo.fanSwitchState);
	item->setCaseFan(stateInfo.caseFanSwitchState);
	item->setAuxiliaryFan(stateInfo.auxiliaryFanSwitchState);
	item->setLedOpened(stateInfo.ledState);

	item->setPcGroup(deviceInfo.m_group);
	item->setPcDeviceName(deviceInfo.m_deviceName);
	item->setErrorMessage(errorMessage);
	item->setNozzleCount(stateInfo.nozzleCount);
	item->setNozzle2Temp(stateInfo.nozzle2Temp);
	item->setChamberTemp(stateInfo.chamberTemp);
	item->setNozzle2DstTemp(stateInfo.nozzle2TempTarget);
	item->setChamberDstTemp(stateInfo.chamberTempTarget);

	item->setFanSpeed(stateInfo.fanSpeedState);
	item->setCaseFanSpeed(stateInfo.caseFanSpeedState);
	item->setAuxiliaryFanSpeed(stateInfo.auxiliaryFanSpeedState);
	item->setMachineHeight(stateInfo.machineHeight);
	item->setMachineWidth(stateInfo.machineWidth);
	item->setMachineDepth(stateInfo.machineDepth);
	item->setFilePrefixPath(stateInfo.filePrefixPath);
	
	item->setPrintObjects(stateInfo.printObjects);
	item->setExcludedObjects(stateInfo.excludedObjects);
	item->setCurrentObject(stateInfo.currentObject);
	item->setMaxNozzleTemp(stateInfo.maxNozzleTemp);
	item->setMaxBedTemp(stateInfo.maxBedTemp);
	item->setWebrtcSupport(stateInfo.webrtcSupport);
	item->setFeedState(stateInfo.feedState);
	item->setCfsConnect(stateInfo.cfsConnect);

	if(stateInfo.boxsInfo.exist)
	{
		if(item->materialBoxList()==nullptr)
		{
			MaterialBoxListModel* materialBoxListModel =  new MaterialBoxListModel(this);
			materialBoxListModel->initRack();
			//materialBoxListModel->onGetMaterialBoxList(stateInfo.macAddress.toStdString(),stateInfo.boxsInfo,stateInfo.type);
			QObject* materialboxQObject = reinterpret_cast<QObject*>(materialBoxListModel);
			item->setMaterialBoxList(materialboxQObject);
		}
		MaterialBoxListModel* materialBoxListModel = reinterpret_cast<MaterialBoxListModel*>(item->materialBoxList());
		materialBoxListModel->onGetMaterialBoxList(stateInfo.macAddress.toStdString(),stateInfo.boxsInfo,stateInfo.type);
		materialBoxListModel->onGetBoxConfig(stateInfo.macAddress.toStdString(), stateInfo.boxsInfo.config, stateInfo.type);
		materialBoxListModel->onGetMaterialState(stateInfo.macAddress, stateInfo.boxsInfo.materialState, stateInfo.type);
	}
	if(item->materialLinkageModel()==nullptr && stateInfo.remoteMaterial.size()>0)
	{
		item->setMaterialLinkageModel(new MaterialLinkageModel(stateInfo.remoteMaterial,this));
	}
	if(stateInfo.remoteFileList.size()>0)
	{
		if(item->filelistModel()==nullptr)
		{
			FileListModel* fileListModel =  new FileListModel(this);
			item->setFilelistModel(fileListModel);
		}
		FileListModel* filelistModel = reinterpret_cast<FileListModel*>(item->filelistModel());
		filelistModel->onGetGcodeFileList(stateInfo.remoteFileList,stateInfo.remoteFileUpdateTime,(int)stateInfo.type);
		
	}
	
	QSharedPointer<us::USettings>settings = createDefaultMachineSetting(printerModelUniqueName);
	QString chamberFan = settings->value(QStringLiteral("support_air_filtration"), "false");
	bool machineChamberFanExist = chamberFan == "true" || chamberFan == "1";
	QString cdsFan = settings->value(QStringLiteral("auxiliary_fan"), "false");
	bool machineCdsFanExist = cdsFan == "true" || cdsFan == "1";
	QString lEDLightExist = settings->value(QStringLiteral("machine_LED_light_exist"), "false");
	bool machineLEDLightExist = lEDLightExist == "true" || lEDLightExist == "1";
	QString platformMotion = settings->value(QStringLiteral("machine_platform_motion_enable"), "false");
	bool machinePlatformMotionEnable = platformMotion == "true" || platformMotion == "1";

	item->setMachineChamberFanExist(machineChamberFanExist);
	item->setMachineCdsFanExist(machineCdsFanExist);
	item->setMachineLEDLightExist(machineLEDLightExist);
	item->setMachinePlatformMotionEnable(machinePlatformMotionEnable);

	

	if (m_posRegExp.exactMatch(stateInfo.curPosition))
	{
		item->setPcX(m_posRegExp.cap(1).toFloat());
		item->setPcY(m_posRegExp.cap(2).toFloat());
		item->setPcZ(m_posRegExp.cap(3).toFloat());
	}

	if (m_posRegExp.exactMatch(stateInfo.autohome))
	{
		bool xyAutohome = m_posRegExp.cap(1).toInt() && m_posRegExp.cap(2).toInt();
		bool zAutohome = m_posRegExp.cap(3).toInt();

		item->setXyAutohome(xyAutohome);
		item->setZAutohome(zAutohome);
	}

	//Add item
	if (rowIndex < 0)
	{
		beginInsertRows(QModelIndex(), rowCount(), rowCount());
		m_ListPrinterItem.append(QPair<QString, ListModelItem*>(macAddress, item));
		endInsertRows();
	}
	else {
		onDataChanged(macAddress, item->m_roles);//Update Data
	}
}

bool  CusListModel::checkPrinterFromIp(QString ip)
{
	bool hasPrinter = false;
	foreach(auto pair, m_ListPrinterItem)
	{
		auto item = pair.second;
		if (item->pcIpAddr() == ip)
		{
			hasPrinter = true;
			break;
		}
	}
	return hasPrinter;
}
int CusListModel::deviceCount()
{
	return m_ListPrinterItem.count();
}
void CusListModel::slotGetPreviewImage(const QString& from_std_macAddr, const QImage& previewImage)
{
	if (getItemRowIndex(from_std_macAddr) < 0) return;

	m_previewProvider->setImage(previewImage, from_std_macAddr);
}

void CusListModel::readConfig()
{
	QFile file(m_appDataPath + "deviceInfo.json");
	if (!file.open(QIODevice::ReadOnly)) return;

	QByteArray byteArray = file.readAll(); file.close();
	QJsonDocument document = QJsonDocument::fromJson(byteArray);
	if (document.isNull() || !document.isObject()) return;
	
	//版本信息
	QJsonObject rootObj = document.object();                              
	int version = rootObj.value("configVersion").toInt();
	if (version < JSON_VERSION)
	{
		file.remove();
		return;
	}

	//分组名称
	QStringList groupNames;
	QJsonArray groupArray = rootObj.value("deviceGroupNames").toArray();
	for (int i = 0; i < groupArray.size(); i++) 
	{
		QString groupName = groupArray.at(i).toString();
		groupNames.append(groupName);
	}
	if (!groupNames.isEmpty()) m_deviceGroupNames = groupNames;

	//设备信息
	QJsonArray infoArray = rootObj.value("deviceInfomation").toArray();
	for (int i = 0; i < infoArray.size(); i++)
	{
		DeviceInfo infoDevice;
		QJsonObject infoObj = infoArray.at(i).toObject();

		infoDevice.m_group = infoObj.value("group").toInt();
		infoDevice.m_connectType = infoObj.value("connectType").toInt();
		infoDevice.m_moonrakerPort = infoObj.value("moonrakerPort").toInt();

		QString macAddress = infoObj.value("macAddress").toString();
		infoDevice.m_ipAddress = infoObj.value("ipAddress").toString();
		infoDevice.m_modelName = infoObj.value("modelName").toString();
		infoDevice.m_deviceName = infoObj.value("deviceName").toString();
		m_MapDeviceInfo.insert(macAddress, infoDevice);
	}
}

void CusListModel::writeConfig()
{
	QFile file(m_appDataPath + "deviceInfo.json");
	if (!file.open(QIODevice::WriteOnly)) return;

	//版本信息
	QJsonObject rootObj;
	rootObj.insert("configVersion", JSON_VERSION);

	//分组名称
	QJsonArray groupArray;
	foreach(QString groupName, m_deviceGroupNames)
	{
		groupArray.append(groupName);
	}
	rootObj.insert("deviceGroupNames", groupArray);

	//设备信息
	QJsonArray infoArray;
	foreach(QString macAddress, m_MapDeviceInfo.keys())
	{
		DeviceInfo infoDevice = m_MapDeviceInfo[macAddress];
		if (infoDevice.m_modelName.isEmpty()) continue;

		QJsonObject infoObj;
		infoObj.insert("group", infoDevice.m_group);
		infoObj.insert("connectType", infoDevice.m_connectType);
		infoObj.insert("moonrakerPort", infoDevice.m_moonrakerPort);

		infoObj.insert("macAddress", macAddress);
		infoObj.insert("ipAddress", infoDevice.m_ipAddress);
		infoObj.insert("modelName", infoDevice.m_modelName);
		infoObj.insert("deviceName", infoDevice.m_deviceName);
		infoArray.append(infoObj);
	}
	rootObj.insert("deviceInfomation", infoArray);

	QJsonDocument document(rootObj);
	QByteArray byteArray = document.toJson();
	file.write(byteArray);
	file.close();
}

void CusListModel::addLocalDevice()
{
	//添加本地设备
	QList<RemotePrinter> listPrinter;
	foreach(QString macAddress, m_MapDeviceInfo.keys())
	{		
		DeviceInfo deviceInfo = m_MapDeviceInfo[macAddress];

		RemotePrinter printer;
		printer.uniqueId = deviceInfo.m_ipAddress;
		printer.ipAddress = deviceInfo.m_ipAddress;
		printer.macAddress = macAddress;
		printer.moonrakerPort = deviceInfo.m_moonrakerPort;
		printer.type = (RemotePrinerType)deviceInfo.m_connectType;
		sigGetInfoSuccess(printer);
		if (printer.type == RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408)
		{
			getRemotePrinterManager()->addPrinter(printer);
			continue;
		}
		listPrinter.append(printer);
	}

	//start thread
	QFuture<void> future = QtConcurrent::run([=]() {
		getRemotePrinterManager()->addPrinter(listPrinter);
	});
}

void CusListModel::onDataChanged(const QString& macAddress, const QVector<int>& roles)
{
	int rowIndex = getItemRowIndex(macAddress);
	QModelIndex modelIndex = createIndex(rowIndex, 0);
	if(rowIndex != -1) emit dataChanged(modelIndex, modelIndex, roles);
}

void CusListModel::UploadProgressCallBack(const std::string& strMac, const std::string& fileName, const float& progress)
{
	Q_UNUSED(fileName)
	QString macAddress = QString::fromStdString(strMac);
	ListModelItem* item = getDeviceItem(macAddress);
	/*m_isTransing = true;
	m_curTransingMac = macAddress;*/
	if (item)
	{
		bool success = qFuzzyCompare(progress, 1.0f);
		item->setGCodeTransProgress(success ? 100.0f : qRound(progress * 100.0f));
		if (success)
		{
			//m_isTransing = false;
			//m_curTransingMac = "";
			if (m_sendOnly)
			{
				item->setSentFileName(m_curGcodeFileName);
			}
			else
			{
				PrintControlType cmdType;
				auto path = item->filePrefixPath() + "/" + m_printPath;
				if(m_multiColor.size() > 0){
					cmdType = item->enableSelfTest() ? PrintControlType::CONTROL_CMD_MULTICOLORPRINT_WITH_CALIBRATION :PrintControlType::CONTROL_CMD_MULTICOLORPRINT;
					getRemotePrinterManager()->materialColorMap(item->pcIpAddr(),path,m_multiColor);
				} else {
					cmdType = item->enableSelfTest() ? PrintControlType::PRINT_WITH_CALIBRATION : PrintControlType::PRINT_START;
				}
				
				sendUIControlCmdToDevice(item->pcIpAddr(), (int)cmdType, path, (item->printerType()),item->moonrakerPort());//Auto Start
			}
		}
	}
}

void CusListModel::UploadMessageCallBack(const std::string& strMac, const std::string& fileName, int errorCode)
{
	Q_UNUSED(fileName)
	QString macAddress = QString::fromStdString(strMac);
	QString errMessage = getErrorMsgFromCode(errorCode);
	ListModelItem* item = getDeviceItem(macAddress);
	m_isTransing = false;
	m_curTransingMac = "";
	if (item) 
	{
		item->setGCodeTransProgress(-1);
		item->setTransErrorMessage(errMessage);
		onDataChanged(macAddress, QVector<int>() << ListModelItem::E_TransErrorMessage);
	}
}

void CusListModel::ImportProgressCallBack(const std::string& strMac, const std::string& fileName, const float& progress)
{
	Q_UNUSED(fileName)
	QString macAddress = QString::fromStdString(strMac);
	ListModelItem* item = getDeviceItem(macAddress);

	if (item) 
	{
		bool success = qFuzzyCompare(progress, 1.0f);
		item->setGCodeImportProgress(success ? 100.0f : qRound(progress * 100.0f));
		item->setSendingFileName(fileName.c_str());
		if (success) item->setGCodeImportProgress(0);//Reset value
	}
}

void CusListModel::ImportMessageCallBack(const std::string& strMac, const std::string& fileName, int errorCode)
{
	Q_UNUSED(fileName)
	QString macAddress = QString::fromStdString(strMac);
	ListModelItem* item = getDeviceItem(macAddress);
	if (item) item->setGCodeImportProgress(-1);
}

int CusListModel::getItemRowIndex(const QString& macAddress)
{
	if (m_ListPrinterItem.isEmpty()) return -1;

	auto it = std::find_if(m_ListPrinterItem.begin(), m_ListPrinterItem.end(), [=](auto pair) {
		return (pair.first == macAddress);
	});

	return (it != m_ListPrinterItem.end()) ? (it - m_ListPrinterItem.begin()) : -1;
}

/******************** ListModelItem ********************/
ListModelItem::ListModelItem(QObject* parent) : QObject(parent), 
m_pcHasCamera(0), m_pcHasSelect(0), m_enableSelfTest(0), m_xyAutohome(false), m_zAutohome(false), m_fanOpened(false), m_caseFan(false), m_auxiliaryFan(false), m_ledOpened(false),
m_pcPrinterState(0), m_pcPrinterStatus(0), m_pcPrinterMethod(0), m_pcTotalPrintTime(0), m_pcLeftTime(0), m_errorKey(0), m_errorCode(0),
m_pcCurPrintLayer(0), m_pcTotalPrintLayer(0), m_pcCurPrintProgress(0), m_pcPrintSpeed(0), m_pcBedDstTemp(0), m_pcNozzleDstTemp(0),
m_pcBedTemp(0), m_pcNozzleTemp(0), m_pcGroup(1), m_pcX(0), m_pcY(0), m_pcZ(0), m_gCodeTransProgress(0), m_gCodeImportProgress(0)
//m_fluiddPort(0), m_printerType(0), m_multiNumber(0), m_mainsailPort(0), m_moonrakerPort(0)
{
	m_pcIpAddr = QString();
	m_pcGCodeName = QString();
	m_pcPrinterID = QString();
	m_pcDeviceName = QString();
	m_pcPrinterModel = QString();
	m_sentFileName = QString();
	m_errorMessage = QString();
	m_transErrorMessage = QString();
}

ListModelItem::ListModelItem(const ListModelItem &listModelItem) : QObject(nullptr)
{
    Q_UNUSED(listModelItem)
}

ListModelItem::~ListModelItem()
{

}

const QString &ListModelItem::pcPrinterID() const
{
    return m_pcPrinterID;
}

void ListModelItem::setPcPrinterID(const QString& printerID)
{
	if (m_pcPrinterID == printerID) 
		return;

	m_pcPrinterID = printerID;
	emit pcPrinterIDChanged();
	m_roles += ListModelItem::E_PrinterID;
}

const QString &ListModelItem::pcPrinterModel() const
{
    return m_pcPrinterModel;
}

void ListModelItem::setPcPrinterModel(const QString& printerModel)
{
	if (m_pcPrinterModel == printerModel) 
		return;

	m_pcPrinterModel = printerModel;
	//emit pcPrinterModelChanged();
	m_roles += ListModelItem::E_PrinterModel;
}

const QString& ListModelItem::printerModelName() const
{
	return m_printerModelName;
}

void ListModelItem::setPrinterModelName(const QString& printerModel)
{
	if (m_printerModelName == printerModel)
		return;

	m_printerModelName = printerModel;
	//emit pcPrinterModelChanged();
	m_roles += ListModelItem::E_PrinterModel;
}

const QString& ListModelItem::deviceName() const
{
	return m_deviceName;
}

void ListModelItem::setDeviceName(const QString& deviceName)
{
	if (m_printerModelName == deviceName)
		return;

	m_deviceName = deviceName;
	emit deviceNameChanged();
}

int ListModelItem::errorKey() const
{
	return m_errorKey;
}

void ListModelItem::setErrorKey(int key)
{
	if (m_errorKey == key)
		return;

	m_errorKey = key;
	emit errorKeyChanged();
	m_roles += ListModelItem::E_ErrorKey;
}

int ListModelItem::errorCode() const
{
	return m_errorCode;
}

void ListModelItem::setErrorCode(int code)
{
	if (m_errorCode == code)
		return;

	m_errorCode = code;
	emit errorCodeChanged();
	m_roles += ListModelItem::E_ErrorCode;
}

int ListModelItem::fluiddPort() const
{
	return m_fluiddPort;
}

void ListModelItem::setFluiddPort(int port)
{
	if (m_fluiddPort == port)
		return;

	m_fluiddPort = port;
	//emit fluiddPortChanged();
	m_roles += ListModelItem::E_FluiddPort;
}

int ListModelItem::printerType() const
{
	return m_printerType;
}

void ListModelItem::setPrinterType(int type)
{
	if (m_printerType == type)
		return;

	m_printerType = type;
	emit printerTypeChanged();
	m_roles += ListModelItem::E_PrinterType;
}

int ListModelItem::multiNumber() const
{
	return m_multiNumber;
}

void ListModelItem::setMultiNumber(int number)
{
	if (m_multiNumber == number)
		return;

	m_multiNumber = number;
	//emit multiNumberChanged();
	m_roles += ListModelItem::E_MultiNumber;
}

int ListModelItem::mainsailPort() const
{
	return m_mainsailPort;
}

void ListModelItem::setMainsailPort(int port)
{
	if (m_mainsailPort == port)
		return;

	m_mainsailPort = port;
	//emit mainsailPortChanged();
	m_roles += ListModelItem::E_MainsailPort;
}

int ListModelItem::moonrakerPort() const
{
	return m_moonrakerPort;
}

void ListModelItem::setMoonrakerPort(const int& port)
{
	if (m_moonrakerPort == port)
		return;

	m_moonrakerPort = port;
}

int ListModelItem::pcPrinterState() const
{
    return m_pcPrinterState;
}

void ListModelItem::setPcPrinterState(int state)
{
	if (m_pcPrinterState == state) 
		return;

	m_pcPrinterState = state;
	emit pcPrinterStateChanged();
	m_roles += ListModelItem::E_PrinterState;
}

int ListModelItem::pcPrinterStatus() const
{
	return m_pcPrinterStatus;
}

void ListModelItem::setPcPrinterStatus(int status)
{
	if (m_pcPrinterStatus == status) 
		return;

	m_pcPrinterStatus = status;
	emit pcPrinterStatusChanged();
	m_roles += ListModelItem::E_PrinterStatus;
}

int ListModelItem::pcPrinterMethod() const
{
	return m_pcPrinterMethod;
}

void ListModelItem::setPcPrinterMethod(int method)
{
	if (m_pcPrinterMethod == method)
		return;

	m_pcPrinterMethod = method;
	emit pcPrinterMethodChanged();
	m_roles += ListModelItem::E_PrinterMethod;
}

int ListModelItem::pcHasCamera() const
{
    return m_pcHasCamera;
}

void ListModelItem::setPcHasCamera(int hasCamera)
{
	if (m_pcHasCamera == hasCamera)
		return;

	m_pcHasCamera = hasCamera;
	emit pcHasCameraChanged();
	m_roles += ListModelItem::E_HasCamera;
}

int ListModelItem::pcHasSelect() const
{
	return m_pcHasSelect;
}

void ListModelItem::setPcHasSelect(int hasSelect)
{
	if (m_pcHasSelect == hasSelect)
		return;

	m_pcHasSelect = hasSelect;
	emit pcHasSelectChanged();
	//m_roles += ListModelItem::E_HasSelect;
}

int ListModelItem::enableSelfTest() const
{
	return m_enableSelfTest;
}

void ListModelItem::setEnableSelfTest(int enabled)
{
	if (m_enableSelfTest == enabled)
		return;

	m_enableSelfTest = enabled;
	emit enableSelfTestChanged();
	//m_roles += ListModelItem::E_EnableSelfTest;
}

const QString &ListModelItem::pcDeviceName() const
{
    return m_pcDeviceName;
}

void ListModelItem::setPcDeviceName(const QString& deviceName)
{
    if (m_pcDeviceName == deviceName)
        return;

    m_pcDeviceName = deviceName;
    //emit pcDeviceNameChanged();
	m_roles += ListModelItem::E_DeviceName;
}

const QString &ListModelItem::pcIpAddr() const
{
    return m_pcIpAddr;
}

void ListModelItem::setPcIpAddr(const QString& ipAddr)
{
	if (m_pcIpAddr == ipAddr)
		return;

	m_pcIpAddr = ipAddr;
	emit pcIpAddrChanged();
	m_roles += ListModelItem::E_IpAddr;
}

const QString &ListModelItem::pcGCodeName() const
{
    return m_pcGCodeName;
}

void ListModelItem::setPcGCodeName(const QString& gcodeName)
{
	if (m_pcGCodeName == gcodeName)
		return;

	m_pcGCodeName = gcodeName;
	emit pcGCodeNameChanged();
	m_roles += ListModelItem::E_GCodeName;
}

int ListModelItem::pcTotalPrintTime() const
{
    return m_pcTotalPrintTime;
}

void ListModelItem::setPcTotalPrintTime(int totalPrintTime)
{
	if (m_pcTotalPrintTime == totalPrintTime)
		return;

	m_pcTotalPrintTime = totalPrintTime;
	emit pcTotalPrintTimeChanged();
	m_roles += ListModelItem::E_TotalPrintTime;
}

int ListModelItem::pcLeftTime() const
{
    return m_pcLeftTime;
}

void ListModelItem::setPcLeftTime(int leftTime)
{
	if (m_pcLeftTime == leftTime)
		return;

	m_pcLeftTime = leftTime;
	emit pcLeftTimeChanged();
	m_roles += ListModelItem::E_LeftTime;
}

int ListModelItem::pcCurPrintLayer() const
{
	return m_pcCurPrintLayer;
}

void ListModelItem::setPcCurPrintLayer(int layer)
{
	if (m_pcCurPrintLayer == layer)
		return;

	m_pcCurPrintLayer = layer;
	emit pcCurPrintLayerChanged();
	m_roles += ListModelItem::E_CurPrintLayer;
}

int ListModelItem::pcTotalPrintLayer() const
{
	return m_pcTotalPrintLayer;
}

void ListModelItem::setPcTotalPrintLayer(int totalLayer)
{
	if (m_pcTotalPrintLayer == totalLayer)
		return;

	m_pcTotalPrintLayer = totalLayer;
	emit pcTotalPrintLayerChanged();
	m_roles += ListModelItem::E_TotalPrintLayer;
}

int ListModelItem::pcCurPrintProgress() const
{
    return m_pcCurPrintProgress;
}

void ListModelItem::setPcCurPrintProgress(int progress)
{
	if (m_pcCurPrintProgress == progress)
		return;

	m_pcCurPrintProgress = progress;
	emit pcCurPrintProgressChanged();
	m_roles += ListModelItem::E_CurPrintProgress;
}

int ListModelItem::pcPrintSpeed() const
{
    return m_pcPrintSpeed;
}

void ListModelItem::setPcPrintSpeed(int speed)
{
    if (m_pcPrintSpeed == speed)
        return;

    m_pcPrintSpeed = speed;
    emit pcPrintSpeedChanged();
	m_roles += ListModelItem::E_PrintSpeed;
}

int ListModelItem::pcBedDstTemp() const
{
    return m_pcBedDstTemp;
}

void ListModelItem::setPcBedDstTemp(int bedDstTemp)
{
    if (m_pcBedDstTemp == bedDstTemp)
        return;

    m_pcBedDstTemp = bedDstTemp;
    emit pcBedDstTempChanged();
	m_roles += ListModelItem::E_BedDstTemp;
}

int ListModelItem::pcNozzleDstTemp() const
{
    return m_pcNozzleDstTemp;
}

void ListModelItem::setPcNozzleDstTemp(int nozzleDstTemp)
{
    if (m_pcNozzleDstTemp == nozzleDstTemp)
        return;

    m_pcNozzleDstTemp = nozzleDstTemp;
    emit pcNozzleDstTempChanged();
	m_roles += ListModelItem::E_NozzleDstTemp;
}

float ListModelItem::pcBedTemp() const
{
    return m_pcBedTemp;
}

void ListModelItem::setPcBedTemp(float bedTemp)
{
	if (qFuzzyCompare(m_pcBedTemp, bedTemp))
		return;

	m_pcBedTemp = bedTemp;
	emit pcBedTempChanged();
	m_roles += ListModelItem::E_BedTemp;
}

float ListModelItem::pcNozzleTemp() const
{
    return m_pcNozzleTemp;
}

void ListModelItem::setPcNozzleTemp(float nozzleTemp)
{
	if (qFuzzyCompare(m_pcNozzleTemp, nozzleTemp))
		return;

	m_pcNozzleTemp = nozzleTemp;
	emit pcNozzleTempChanged();
	m_roles += ListModelItem::E_NozzleTemp;
}

int ListModelItem::pcGroup() const
{
    return m_pcGroup;
}

void ListModelItem::setPcGroup(int groupNo)
{
    if (m_pcGroup == groupNo)
        return;

    m_pcGroup = groupNo;
    emit pcGroupChanged();
	m_roles += ListModelItem::E_GroupNo;
}

float ListModelItem::pcX() const
{
    return m_pcX;
}

void ListModelItem::setPcX(float newPcX)
{
    if (qFuzzyCompare(m_pcX, newPcX))
        return;

    m_pcX = newPcX;
    emit pcXChanged();
}

float ListModelItem::pcY() const
{
    return m_pcY;
}

void ListModelItem::setPcY(float newPcY)
{
    if (qFuzzyCompare(m_pcY, newPcY))
        return;

    m_pcY = newPcY;
    emit pcYChanged();
}

float ListModelItem::pcZ() const
{
    return m_pcZ;
}

void ListModelItem::setPcZ(float newPcZ)
{
    if (qFuzzyCompare(m_pcZ, newPcZ))
        return;

    m_pcZ = newPcZ;
    emit pcZChanged();
}

bool ListModelItem::xyAutohome() const
{
	return m_xyAutohome;
}

void ListModelItem::setXyAutohome(bool home)
{
	if (m_xyAutohome == home)
		return;

	m_xyAutohome = home;
	emit xyAutohomeChanged();
}

bool ListModelItem::zAutohome() const
{
	return m_zAutohome;
}

void ListModelItem::setZAutohome(bool home)
{
	if (m_zAutohome == home)
		return;

	m_zAutohome = home;
	emit zAutohomeChanged();
}

bool ListModelItem::fanOpened() const
{
    return m_fanOpened;
}

void ListModelItem::setFanOpened(bool opened)
{
    if (m_fanOpened == opened)
        return;

    m_fanOpened = opened;
    emit fanOpenedChanged();
}

bool ListModelItem::caseFan() const
{
	return m_caseFan;
}

void ListModelItem::setCaseFan(bool opened)
{
	if (m_caseFan == opened)
		return;

	m_caseFan = opened;
	emit caseFanChanged();
}

bool ListModelItem::auxiliaryFan() const
{
	return m_auxiliaryFan;
}

void ListModelItem::setAuxiliaryFan(bool opened)
{
	if (m_auxiliaryFan == opened)
		return;

	m_auxiliaryFan = opened;
	emit auxiliaryFanChanged();
}

int ListModelItem::fanSpeed() const
{
	return m_fanSpeed;
}

void ListModelItem::setFanSpeed(int speed)
{
	if (m_fanSpeed == speed)
		return;

	m_fanSpeed = speed;
	emit fanSpeedChanged();
}
int ListModelItem::caseFanSpeed() const
{
	return m_caseFanSpeed;
}

void ListModelItem::setCaseFanSpeed(int speed)
{
	if (m_caseFanSpeed == speed)
		return;

	m_caseFanSpeed = speed;
	emit caseFanSpeedChanged();
}
int ListModelItem::auxiliaryFanSpeed() const
{
	return m_auxiliaryFanSpeed;
}
void ListModelItem::setMaterialLinkageModel(QObject* model)
{
	m_materialLinkageModel = model;
}
QObject* ListModelItem::materialLinkageModel()
{
	return m_materialLinkageModel;
}
QObject* ListModelItem::filelistModel()
{
	return m_filelistModel;
}
QObject* ListModelItem::gcodeFileListModel()
{
	if(m_gcodeFileListModel==nullptr)
	{
		LocalNetPlugin* plugin = qobject_cast<LocalNetPlugin*>(parent()->parent());
		return plugin->gcodeSortModel();
	}
	return m_gcodeFileListModel;
}

void ListModelItem::setFilelistModel(QObject* model)
{
	if(m_gcodeFileListModel!=nullptr)
	{
		m_gcodeFileListModel->deleteLater();
	}
	m_filelistModel = model;
	SortProxyModel* gcodeSortModel = new SortProxyModel(this);
	m_gcodeFileListModel = gcodeSortModel;
	gcodeSortModel->setSourceModel(qobject_cast<QAbstractItemModel*>(m_filelistModel));
	emit gcodeFileListChanged();
}
void ListModelItem::setAuxiliaryFanSpeed(int speed)
{
	if (m_auxiliaryFanSpeed == speed)
		return;

	m_auxiliaryFanSpeed = speed;
	emit auxiliaryFanSpeedChanged();
}

bool ListModelItem::ledOpened() const
{
    return m_ledOpened;
}

void ListModelItem::setLedOpened(bool opened)
{
    if (m_ledOpened == opened)
        return;

    m_ledOpened = opened;
    emit ledOpenedChanged();
}

QString ListModelItem::printObjects() const
{
	return m_printObjects;
}

void ListModelItem::setPrintObjects(const QString& printobject)
{
	if (m_printObjects == printobject)
		return;

	m_printObjects = printobject;
	emit printObjectsChanged();
}

QString ListModelItem::excludedObjects() const
{
	return m_excludedObjects;
}

void ListModelItem::setExcludedObjects(const QString& printobject)
{
	if (m_excludedObjects == printobject)
		return;

	m_excludedObjects = printobject;
	emit excludedObjectsChanged();
}

QString ListModelItem::currentObject() const
{
	return m_currentObject;
}

void ListModelItem::setCurrentObject(const QString& printobject)
{
	if (m_currentObject == printobject)
		return;

	m_currentObject = printobject;
	emit currentObjectChanged();
}

int ListModelItem::maxNozzleTemp() const
{
	return m_maxNozzleTemp;
}

void ListModelItem::setMaxNozzleTemp(const int temp)
{
	if (m_maxNozzleTemp == temp)
		return;

	m_maxNozzleTemp = temp;
	emit maxNozzleTempChanged();
}

int ListModelItem::maxBedTemp() const
{
	return m_maxBedTemp;
}

void ListModelItem::setMaxBedTemp(const int temp)
{
	if (m_maxBedTemp == temp)
		return;

	m_maxBedTemp = temp;
	emit maxBedTempChanged();
}

bool ListModelItem::machineChamberFanExist() const
{
	return m_machineChamberFanExist;
}

void ListModelItem::setMachineChamberFanExist(const bool& machineChamberFanExist)
{
	if (m_machineChamberFanExist == machineChamberFanExist)
		return;

	m_machineChamberFanExist = machineChamberFanExist;
	emit machineChamberFanExistChanged();
}

bool ListModelItem::machineCdsFanExist() const
{
	return m_machineCdsFanExist;
}
void ListModelItem::setMachineCdsFanExist(const bool& machineCdsFanExist)
{
	if (m_machineCdsFanExist == machineCdsFanExist)
		return;

	m_machineCdsFanExist = machineCdsFanExist;
	emit machineCdsFanExistChanged();
}

bool ListModelItem::machineLEDLightExist() const
{
	return m_machineLEDLightExist;
}
void ListModelItem::setMachineLEDLightExist(const bool& machineLEDLightExist)
{
	if (m_machineLEDLightExist == machineLEDLightExist)
		return;

	m_machineLEDLightExist = machineLEDLightExist;
	emit machineLEDLightExistChanged();
}

bool ListModelItem::machinePlatformMotionEnable() const
{
	return m_machinePlatformMotionEnable;
}
void ListModelItem::setMachinePlatformMotionEnable(const bool& machinePlatformMotionEnable)
{
	if (m_machinePlatformMotionEnable == machinePlatformMotionEnable)
		return;

	m_machinePlatformMotionEnable = machinePlatformMotionEnable;
	emit machinePlatformMotionEnableChanged();
}

float ListModelItem::gCodeTransProgress() const
{
    return m_gCodeTransProgress;
}

void ListModelItem::setGCodeTransProgress(float progress)
{
	if (qFuzzyCompare(m_gCodeTransProgress, progress))
		return;

	m_gCodeTransProgress = progress;
    emit gCodeTransProgressChanged();
	//m_roles += ListModelItem::E_TransProgress;
}

float ListModelItem::gCodeImportProgress() const
{
	return m_gCodeImportProgress;
}

void ListModelItem::setGCodeImportProgress(float progress)
{
	if (qFuzzyCompare(m_gCodeImportProgress, progress))
		return;

	m_gCodeImportProgress = progress;
	emit gCodeImportProgressChanged();
}

QString ListModelItem::sentFileName() const
{
	return m_sentFileName;
}

void ListModelItem::setSentFileName(const QString& filename)
{
	if (m_sentFileName == filename)
		return;

	m_sentFileName = filename;
	emit sentFileNameChanged();
	//m_roles += ListModelItem::E_SentFileName;
}

QString ListModelItem::errorMessage() const
{
	return m_errorMessage;
}

void ListModelItem::setErrorMessage(const QString& message)
{
	if (m_errorMessage == message)
		return;

	m_errorMessage = message;
	emit errorMessageChanged();
	m_roles += ListModelItem::E_ErrorMessage;
}

QString ListModelItem::transErrorMessage() const
{
	return m_transErrorMessage;
}

void ListModelItem::setTransErrorMessage(const QString& message)
{
	if (m_transErrorMessage == message)
		return;

	m_transErrorMessage = message;
	//emit transErrorMessageChanged();
}

int ListModelItem::nozzleCount() const
{
	return m_nozzleCount;
}

void ListModelItem::setNozzleCount(int nozzleCount)
{
	if (m_nozzleCount == nozzleCount)
	{
		return;
	}
	m_nozzleCount = nozzleCount;
	emit nozzleCountChanged();
}

float ListModelItem::nozzle2Temp() const
{
	return m_nozzle2Temp;
}

void ListModelItem::setNozzle2Temp(float nozzle2Temp)
{
	if (qFuzzyCompare(m_nozzle2Temp, nozzle2Temp))
		return;

	m_nozzle2Temp = nozzle2Temp;
	emit nozzle2TempChanged();
}

float ListModelItem::chamberTemp() const
{
	return m_chamberTemp;
}

void ListModelItem::setChamberTemp(float chamberTemp)
{
	if (qFuzzyCompare(m_chamberTemp, chamberTemp))
		return;

	m_chamberTemp = chamberTemp;
	emit chamberTempChanged();
}

float ListModelItem::nozzle2DstTemp() const
{
	return m_nozzle2DstTemp;
}

void ListModelItem::setNozzle2DstTemp(float nozzle2Temp)
{
	if (qFuzzyCompare(m_nozzle2DstTemp, nozzle2Temp))
		return;

	m_nozzle2DstTemp = nozzle2Temp;
	emit nozzle2DstTempChanged();
}

float ListModelItem::chamberDstTemp() const
{
	return m_chamberDstTemp;
}

void ListModelItem::setChamberDstTemp(float chamberTemp)
{
	if (qFuzzyCompare(m_chamberDstTemp, chamberTemp))
		return;

	m_chamberDstTemp = chamberTemp;
	emit chamberTempChanged();
}

float ListModelItem::machineHeight() const
{
	return m_machineHeight;
}

void ListModelItem::setMachineHeight(float height)
{
	if (qFuzzyCompare(m_machineHeight, height))
		return;

	m_machineHeight = height;
}

float ListModelItem::machineWidth() const
{
	return m_machineWidth;
}

void ListModelItem::setMachineWidth(float width)
{
	if (qFuzzyCompare(m_machineWidth, width))
		return;

	m_machineWidth = width;
}

float ListModelItem::machineDepth() const
{
	return m_machineDepth;
}

void ListModelItem::setMachineDepth(float depth)
{
	if (qFuzzyCompare(m_machineDepth, depth))
		return;

	m_machineDepth = depth;
}

QString ListModelItem::filePrefixPath() const
{
	return m_filePrefixPath;
}

void ListModelItem::setFilePrefixPath(QString path)
{
	if (path == m_filePrefixPath)
	{
		return;
	}

	m_filePrefixPath = path;
	emit filePrefixPathChanged();
}

QString ListModelItem::sendingFileName() const
{
	return m_sendingFileName;
}

void ListModelItem::setSendingFileName(const QString& filename)
{
	if (filename == m_sendingFileName)
	{
		return;
	}
	m_sendingFileName = filename;
	emit sendingFileNameChanged();
}


QObject* ListModelItem::materialBoxList() const
{
	return m_materialBoxList;
}

void ListModelItem::setMaterialBoxList(QObject* box)
{
	if (box == m_materialBoxList)
	{
		return;
	}
	m_materialBoxList = box;
	emit materialBoxListChanged();
}

int ListModelItem::webrtcSupport() const
{
	return m_webrtcSupport;
}

void ListModelItem::setWebrtcSupport(int isSupport)
{
	if (isSupport == m_webrtcSupport)
	{
		return;
	}

	m_webrtcSupport = isSupport;
	emit webrtcSupportChanged();
}

int ListModelItem::feedState() const
{
	return m_feedState;
}

void ListModelItem::setFeedState(int feedState)
{
	if (feedState == m_feedState)
	{
		return;
	}

	m_feedState = feedState;
	emit feedStateChanged();
}

int ListModelItem::cfsConnect() const
{
	return m_cfsConnect;
}

void ListModelItem::setCfsConnect(int cfsConnect)
{
	if (cfsConnect == m_cfsConnect)
	{
		return;
	}

	m_cfsConnect = cfsConnect;
	emit cfsConnectChanged();
}
bool ListModelItem::sync() const
{
	return m_sync;
}

void ListModelItem::setSync(bool sync)
{
	if (m_sync == sync)
	{
		return;
	}
	m_sync = sync;
	emit syncChanged();
}