#include "cusscanmodel.h"

using namespace creative_kernel;

/******************** CusScanModel ********************/
CusScanModel::CusScanModel(QObject* parent) : QAbstractListModel(parent)
{
	m_bFirstSearch = false;
}

CusScanModel::~CusScanModel()
{
	getRemotePrinterManager()->setSearchCb(nullptr);
	this->disconnect();
}

void CusScanModel::searchDevice()
{
	getRemotePrinterManager()->searchDevices();
}

void CusScanModel::searchDeviceList()
{
	if (!m_bFirstSearch)
	{
		m_bFirstSearch = true;
		connect(this, &CusScanModel::sigGetSearchSuccess, this, &CusScanModel::slotGetSearchSuccess, Qt::ConnectionType::QueuedConnection);
		getRemotePrinterManager()->setSearchCb(std::bind(&CusScanModel::onGetSearchSuccess, this, std::placeholders::_1));
		//getRemotePrinterManager()->searchDevices();
	}
}

void CusScanModel::selectAllDevice(bool select)
{
	foreach(auto pair, m_ListScanItem)
	{
		auto item = pair.second;
		item->setPcHasSelect(select);
	}
}

void CusScanModel::addSearchDevice()
{
	QList<RemotePrinter> listPrinter;
	foreach(auto pair, m_ListScanItem)
	{
		auto item = pair.second;
		if (item->pcHasSelect())
		{
			RemotePrinter printer;
			item->setPcHasSelect(false);
			printer.uniqueId = item->pcIpAddr();
			printer.ipAddress = item->pcIpAddr();
			printer.macAddress = item->pcPrinterID();
			printer.moonrakerPort = item->moonrakerPort();
			printer.type = (RemotePrinerType)item->printerType();
			if (printer.type == RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408)
			{
				getRemotePrinterManager()->addPrinter(printer);
				continue;
			}
			listPrinter.append(printer);
		}
	}

	//start thread
	QFuture<void> future = QtConcurrent::run([=]() {
		getRemotePrinterManager()->addPrinter(listPrinter);
	});
}

void CusScanModel::onGetSearchSuccess(const RemotePrinter& printer)
{
	emit sigGetSearchSuccess(printer);
}

int CusScanModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent)
	return m_ListScanItem.count();
}

QVariant CusScanModel::data(const QModelIndex& index, int role) const
{
	if (index.row() < 0 || index.row() >= rowCount()) return QVariant();

	QVariant value;
	auto pair = m_ListScanItem.at(index.row());
	ScanModelItem* modelItem = pair.second;

	switch (role)
	{
		case ScanModelItem::E_HasSelect:
			value = QVariant::fromValue(modelItem);
			break;
		case ScanModelItem::E_PrinterType:
			value = modelItem->printerType();
			break;
		case ScanModelItem::E_PrinterID:
			value = modelItem->pcPrinterID();
			break;
		case ScanModelItem::E_DeviceName:
			value = modelItem->pcDeviceName();
			break;
		case ScanModelItem::E_PrinterModel:
			value = modelItem->pcPrinterModel();
			break;
		case ScanModelItem::E_IpAddr:
			value = modelItem->pcIpAddr();
			break;
		default:
			break;
	}
	return value;
}

QHash<int, QByteArray> CusScanModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[ScanModelItem::E_HasSelect] = "mItem";
	roles[ScanModelItem::E_PrinterType] = "printerType";
	roles[ScanModelItem::E_PrinterID] = "printerID";
	roles[ScanModelItem::E_DeviceName] = "deviceName";
	roles[ScanModelItem::E_PrinterModel] = "printerModel";
	roles[ScanModelItem::E_IpAddr] = "ipAddr";
	return roles;
}

int CusScanModel::getItemRowIndex(const QString& macAddress)
{
	if (m_ListScanItem.isEmpty()) return -1;

	auto it = std::find_if(m_ListScanItem.begin(), m_ListScanItem.end(), [=](auto pair) {
		return (pair.first == macAddress);
	});

	return (it != m_ListScanItem.end()) ? (it - m_ListScanItem.begin()) : -1;
}

void CusScanModel::slotGetSearchSuccess(const RemotePrinter& printer)
{
	QString macAddress = printer.macAddress;
	QString printerModel = printer.printerName.simplified();
	if (printerModel.isEmpty() && printer.type == RemotePrinerType::REMOTE_PRINTER_TYPE_LAN) return;

	int rowIndex = getItemRowIndex(macAddress);
	ScanModelItem* scanItem = (rowIndex < 0) ? new ScanModelItem(this) : m_ListScanItem[rowIndex].second;

	scanItem->m_printerType = int(printer.type);
	scanItem->m_moonrakerPort = printer.moonrakerPort;
	scanItem->m_pcPrinterID = macAddress;
	scanItem->m_pcDeviceName = printerModel;
	scanItem->m_pcPrinterModel = printerModel;
	scanItem->m_pcIpAddr = printer.ipAddress;

	//Add item
	if (rowIndex < 0)
	{
		beginInsertRows(QModelIndex(), rowCount(), rowCount());
		m_ListScanItem.append(QPair<QString, ScanModelItem*>(macAddress, scanItem));
		endInsertRows();
	}
	else {
		//Update data
		QModelIndex modelIndex = createIndex(rowIndex, 0);
		emit dataChanged(modelIndex, modelIndex);
	}
}

/******************** ScanModelItem ********************/
ScanModelItem::ScanModelItem(QObject* parent) : QObject(parent), m_pcHasSelect(0), m_printerType(0)
{
	m_pcIpAddr = QString();
	m_pcPrinterID = QString();
	m_pcDeviceName = QString();
	m_pcPrinterModel = QString();
}

ScanModelItem::ScanModelItem(const ScanModelItem& scanModelItem) : QObject(nullptr)
{
	Q_UNUSED(scanModelItem)
}

ScanModelItem::~ScanModelItem()
{

}

int ScanModelItem::pcHasSelect() const
{
	return m_pcHasSelect;
}

void ScanModelItem::setPcHasSelect(int hasSelect)
{
	if (m_pcHasSelect == hasSelect)
		return;

	m_pcHasSelect = hasSelect;

	emit pcHasSelectChanged();
}

int ScanModelItem::printerType() const
{
	return m_printerType;
}

int ScanModelItem::moonrakerPort() const
{
	return m_moonrakerPort;
}

const QString& ScanModelItem::pcPrinterID() const
{
	return m_pcPrinterID;
}

const QString& ScanModelItem::pcDeviceName() const
{
	return m_pcDeviceName;
}

const QString& ScanModelItem::pcPrinterModel() const
{
	return m_pcPrinterModel;
}

const QString& ScanModelItem::pcIpAddr() const
{
	return m_pcIpAddr;
}
