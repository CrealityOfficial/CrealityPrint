#include <QDebug>
#include <QDateTime>
#include <QUrl>

#include "historylistmodel.h"
#include <regex>
#include <QFileInfo>
using namespace creative_kernel;

/******************** HistoryListModel ********************/
HistoryListModel::HistoryListModel(QObject* parent) : QAbstractListModel(parent)
{
	m_printerType = 0;
	m_bFirstConnect = false;
	m_webInfoRegExp = QRegExp("([\\d]+)[:](/.+)[:]([\\d]+)[:]([\\d]+)[:]([\\d]+)[:](.+)[:](.+)[:]([\\d])");
}

HistoryListModel::~HistoryListModel()
{
	getRemotePrinterManager()->setGetHistoryListCb(nullptr);
	this->disconnect();
}

int HistoryListModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent)
	return m_HistoryItemList.count();
}

QVariant HistoryListModel::data(const QModelIndex& index, int role) const
{
	if (index.row() < 0 || index.row() >= rowCount()) return QVariant();

	QVariant value;
	HistoryModelItem * modelItem = m_HistoryItemList.at(index.row());

	switch (role)
	{
		case HistoryModelItem::E_IsPrintFinished:
			value = modelItem->isPrintFinished();
			break;
		case HistoryModelItem::E_GCodePrefixPath:
			value = modelItem->gCodePrefixPath();
			break;
		case HistoryModelItem::E_HistoryFileNumb:
			value = modelItem->historyFileNumb();
			break;
		case HistoryModelItem::E_HistoryFileName:
			value = modelItem->historyFileName();
			break;
		case HistoryModelItem::E_HistoryFileSize:
			value = modelItem->historyFileSize();
			break;
		case HistoryModelItem::E_HistoryFileTime:
			value = modelItem->historyFileTime();
			break;
		case HistoryModelItem::E_HistoryInterval:
			value = modelItem->historyInterval();
			break;
		case HistoryModelItem::E_HistoryMaterialUsage:
			value = modelItem->historyMaterialUsage();
			break;
		case HistoryModelItem::E_HistoryThumbnailImage:
			value = modelItem->historyThumbnailImage();
			break;
		case HistoryModelItem::E_GCodeExportProgress:
			value = modelItem->gCodeExportProgress();
			break;

		case HistoryModelItem::E_HistoryIsMultiColor:
			value = modelItem->historyIsMultiColor();
			break;
		case HistoryModelItem::E_HistorySelected:
			value = QVariant::fromValue(modelItem);
			break;
		case HistoryModelItem::E_TYPE:
			QFileInfo(modelItem->historyFileName()).suffix().toLower() == "gcode" ? value = 1 : value = 3;
			break;
		default:
			break;
	}
	return value;
}

QHash<int, QByteArray> HistoryListModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[HistoryModelItem::E_IsPrintFinished]	     = "isPrintFinished";
	roles[HistoryModelItem::E_HistoryFileNumb]		 = "historyFileNumb";
	roles[HistoryModelItem::E_HistoryFileName]	     = "historyFileName";
	roles[HistoryModelItem::E_HistoryFileSize]	     = "historyFileSize";
	roles[HistoryModelItem::E_HistoryFileTime]	     = "historyFileTime";
	roles[HistoryModelItem::E_HistoryInterval]	     = "historyInterval";
	roles[HistoryModelItem::E_HistoryMaterialUsage]  = "historyMaterialUsage";
	roles[HistoryModelItem::E_HistoryThumbnailImage] = "historyThumbnailImage";
	roles[HistoryModelItem::E_GCodeExportProgress] = "gCodeExportProgress";
	roles[HistoryModelItem::E_GCodePrefixPath] = "gCodePrefixPath";

	roles[HistoryModelItem::E_HistoryIsMultiColor] = "historyIsMultiColor";
	roles[HistoryModelItem::E_HistorySelected] = "mItem";
	roles[HistoryModelItem::E_TYPE] = "type";
	
	return roles;
}

void HistoryListModel::findHistoryFileList()
{
	if (!m_bFirstConnect)
	{
		m_bFirstConnect = true;
		connect(this, &HistoryListModel::sigGetHistoryFileList, this, &HistoryListModel::slotGetHistoryFileList, Qt::ConnectionType::QueuedConnection);
		getRemotePrinterManager()->setGetHistoryListCb(std::bind(&HistoryListModel::onGetHistoryFileList, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	}
}

void HistoryListModel::deleteHistoryFile(const QString& ipAddress, const QString& id, int remoteType)
{
	getRemotePrinterManager()->deleteFile(ipAddress.toStdString(), id.toStdString(), RemotePrinerType(remoteType), OpPrinerFileType::HISTORY_FILE);
}

void HistoryListModel::deleteSelectedHistoryFile(const QString& ipAddress)
{
	for(auto item:m_HistoryItemList)
	{	
		RemotePrinerType type = RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408;
		if(item->historySelected())
			deleteHistoryFile(ipAddress,item->historyFileNumb(), static_cast<int>(type));
	}
}

void HistoryListModel::switchDataSource(const QString& macAddress, RemotePrinerType remoteType, bool init)//@true: init @false: update
{

	if (m_HistoryItemMap.contains(macAddress))
	{
		m_printerType = int(remoteType);
		beginResetModel();
		m_HistoryItemList = m_HistoryItemMap[macAddress];
		endResetModel();
		emit printerTypeChanged();
		emit layoutChanged();
		if(&HistoryListModel::historyFileSelectedCountChanged) historyFileSelectedCountChanged();
	}
	
}

void HistoryListModel::downloadGcodeFile(const QString& macAddress, const QString& ipAddress, const QString& fileName, const QString& localPath, const int& remoteType, const QString& historyFileNumb)
{
	cpr::Session session;
	std::string url = "http://" + ipAddress.toStdString() + "/downloads/gcode/";
	url += session.GetCurlHolder()->urlEncode(fileName.toStdString());
	getRemotePrinterManager()->downloadFile(url, QUrl(localPath).toLocalFile().toStdString(), std::bind(&HistoryListModel::onGetExportProgress, this, macAddress, historyFileNumb, std::placeholders::_1));
}



void HistoryListModel::downloadSelectedHistoryFile(const QString& macAddr,const QString& ipAddress,const QString& localPath)
{
	for(auto item:m_HistoryItemList)
	{	
		if(item->historySelected())
		{
			RemotePrinerType type = RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408;
			QString fileName = item->historyFileName();
			QString path = localPath + "/" + fileName;
			downloadGcodeFile(macAddr,ipAddress,fileName, path, static_cast<int>(type),item->historyFileNumb());
		}
			
	}
}
int HistoryListModel::historyFileSelectedCount()
{
	int index = 0;
	for (auto item : m_HistoryItemList)
	{
		if (item->historySelected()) {
			index++;
		}
	}
	return index;
}

QStringList HistoryListModel::historyFileSelectedList()
{
	QStringList checkedList;
	for (auto item : m_HistoryItemList)
	{
		if (item->historySelected()) {
			checkedList << item->historyFileName();
		}
	}
	return checkedList;
}

void HistoryListModel::setHistoryFileSelected(bool select)
{
	for(auto item:m_HistoryItemList)
	{	
		item->setHistorySelected(select);
	}
}


int HistoryListModel::printerType() const
{
	return m_printerType;
}

void HistoryListModel::onGetHistoryFileList(const std::string& std_macAddr, const std::string& std_list, RemotePrinerType remoteType)
{
	if (std_macAddr.empty()) return;
	if (!m_HistoryInfoMap.contains(std_macAddr) || m_HistoryInfoMap[std_macAddr].compare(std_list) != 0)
	{
		m_HistoryInfoMap[std_macAddr] = std_list;
		QString from_std_list = QString::fromStdString(std_list);
		QString from_std_macAddr = QString::fromStdString(std_macAddr);
		emit sigGetHistoryFileList(from_std_macAddr, from_std_list, remoteType);
	}
}

void HistoryListModel::onGetExportProgress(const QString& ipAddress, const QString& fileName, const float& progress)
{
	QString macAddr = ipAddress;

	if (!m_HistoryItemMap.contains(macAddr)) return;

	QList<HistoryModelItem*> itemList = m_HistoryItemMap[macAddr];

	int rowIndex = getItemRowIndex(fileName, itemList);

	if (rowIndex != -1)
	{
		bool success = qFuzzyCompare(progress, 1.0f);
		itemList[rowIndex]->setGCodeExportProgress(progress);
		if (success) itemList[rowIndex]->setGCodeExportProgress(0);//Reset value

		QModelIndex modelIndex = createIndex(rowIndex, 0);
		emit dataChanged(modelIndex, modelIndex, QVector<int>() << HistoryModelItem::E_GCodeExportProgress);
	}
}

int HistoryListModel::getItemRowIndex(const QString& fileName, const QList<HistoryModelItem*>& itemList)
{
	for (int i = 0; i < itemList.count(); ++i)
	{
		const auto& item = itemList[i];
		if (item->historyFileNumb() == fileName)
		{
			return i;
		}
	}
	return -1;
}

void HistoryListModel::slotGetHistoryFileList(const QString& from_std_macAddr, const QString& from_std_list, RemotePrinerType remoteType)
{
	QList<HistoryModelItem*> itemList;

	switch (remoteType)
	{
		case RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408:
		{
			QStringList splitList = from_std_list.split(";", QString::SkipEmptyParts);
			foreach(QString fullInfo, splitList)
			{
				if (m_webInfoRegExp.exactMatch(fullInfo))
				{
					HistoryModelItem* modelItem = new HistoryModelItem(this);

					int lastPathIndex = m_webInfoRegExp.cap(2).lastIndexOf('/');
					QString gcodePrefixPath = m_webInfoRegExp.cap(2).left(lastPathIndex);
					QString historyFileNumb = m_webInfoRegExp.cap(1);
					QString historyFileName = m_webInfoRegExp.cap(2).split("/", QString::SkipEmptyParts).last();
					QString historyFileSize = QString::number(m_webInfoRegExp.cap(3).toDouble() / 1024.0 / 1024.0, 'f', 2);
					QString historyInterval = QDateTime::fromSecsSinceEpoch(m_webInfoRegExp.cap(4).toLongLong()).toString("yyyy-MM-dd hh:mm:ss");
					QString historyFileTime = m_webInfoRegExp.cap(5);
					QString materialUsage   = QString::number(m_webInfoRegExp.cap(6).toDouble() / 1000.0, 'f', 3);
					//QString thumbnailImage  = m_webInfoRegExp.cap(7);
					int		isPrintFinished = m_webInfoRegExp.cap(8).toInt();
					QString isMultiColor = "2";
					QStringList listItem = fullInfo.split(":", QString::SkipEmptyParts);
					if (listItem.size() > 9)  isMultiColor = listItem[9];


					modelItem->setHistoryFileNumb(historyFileNumb);
					modelItem->setGCodePrefixPath(gcodePrefixPath);
					modelItem->setHistoryFileName(historyFileName);
					modelItem->setHistoryFileSize(historyFileSize);
					modelItem->setHistoryInterval(historyInterval);
					modelItem->setHistoryFileTime(historyFileTime);
					modelItem->setHistoryMaterialUsage(materialUsage);
					modelItem->setIsPrintFinished(isPrintFinished);

					modelItem->setHistoryIsMultiColor(isMultiColor);
					connect(modelItem, &HistoryModelItem::historySelectedChanged, this, &HistoryListModel::historyFileSelectedCountChanged);


					itemList.append(modelItem);
				}
			}
		}
			break;
		default:
			break;
	}

	m_HistoryItemMap[from_std_macAddr] = itemList;
	if(m_currentMacAddr==from_std_macAddr)
	{
		switchDataSource(from_std_macAddr, remoteType, false);
	}
}


/******************** HistoryModelItem ********************/
HistoryModelItem::HistoryModelItem(QObject* parent) : QObject(parent)
{
	m_isPrintFinished	   = false;
	m_historyFileNumb	   = QString();
	m_historyFileName	   = QString();
	m_historyFileSize	   = QString();
	m_historyFileTime	   = QString();
	m_historyInterval	   = QString();
	m_historyMaterialUsage = QString();
	m_historySelected        = false;
}

HistoryModelItem::HistoryModelItem(const HistoryModelItem& HistoryModelItem) : QObject(nullptr)
{
	Q_UNUSED(HistoryModelItem)
}

HistoryModelItem::~HistoryModelItem()
{

}

int HistoryModelItem::isPrintFinished() const
{
	return m_isPrintFinished;
}

void HistoryModelItem::setIsPrintFinished(int finished)
{
	m_isPrintFinished = finished;
	emit isPrintFinishedChanged();
}

const QString& HistoryModelItem::gCodePrefixPath() const
{
	return m_gCodePrefixPath;
}

void HistoryModelItem::setGCodePrefixPath(const QString& path)
{
	m_gCodePrefixPath = path;
	emit gCodePrefixPathChanged();
}

const QString& HistoryModelItem::historyFileNumb() const
{
	return m_historyFileNumb;
}

void HistoryModelItem::setHistoryFileNumb(const QString& fileNumb)
{
	m_historyFileNumb = fileNumb;
	emit historyFileNumbChanged();
}

const QString& HistoryModelItem::historyFileName() const
{
	return m_historyFileName;
}

void HistoryModelItem::setHistoryFileName(const QString& fileName)
{
	m_historyFileName = fileName;
	emit historyFileNameChanged();
}

const QString& HistoryModelItem::historyFileSize() const
{
	return m_historyFileSize;
}

void HistoryModelItem::setHistoryFileSize(const QString& fileSize)
{
	m_historyFileSize = fileSize;
	emit historyFileSizeChanged();
}

const QString& HistoryModelItem::historyFileTime() const
{
	return m_historyFileTime;
}

void HistoryModelItem::setHistoryFileTime(const QString& fileTime)
{
	m_historyFileTime = fileTime;
	emit historyFileTimeChanged();
}

const QString& HistoryModelItem::historyInterval() const
{
	return m_historyInterval;
}

void HistoryModelItem::setHistoryInterval(const QString& interval)
{
	m_historyInterval = interval;
	emit historyIntervalChanged();
}

const QString& HistoryModelItem::historyMaterialUsage() const
{
	return m_historyMaterialUsage;
}

void HistoryModelItem::setHistoryMaterialUsage(const QString& usage)
{
	m_historyMaterialUsage = usage;
	emit historyMaterialUsageChanged();
}

const QString& HistoryModelItem::historyThumbnailImage() const
{
	return m_historyThumbnailImage;
}

void HistoryModelItem::setHistoryThumbnailImage(const QString& image)
{
	m_historyThumbnailImage = image;
	emit historyThumbnailImageChanged();
}

float HistoryModelItem::gCodeExportProgress() const
{
	return m_gCodeExportProgress;
}

void HistoryModelItem::setGCodeExportProgress(float progress)
{
	if (qFuzzyCompare(m_gCodeExportProgress, progress))
		return;

	m_gCodeExportProgress = progress;
	emit gCodeExportProgressChanged();
}


bool HistoryModelItem::historySelected() const
{
	return m_historySelected;
}

void HistoryModelItem::setHistorySelected(bool selected)
{
	if (m_historySelected == selected)
		return;

	m_historySelected = selected;
	emit historySelectedChanged();
}

QString HistoryModelItem::historyIsMultiColor() const
{
	return m_historyIsMultiColor;
}

void HistoryModelItem::setHistoryIsMultiColor(const QString& isMulti)
{
	if (m_historyIsMultiColor == isMulti)
		return;

	m_historyIsMultiColor = isMulti;
	emit historyIsMultiColorChanged();
}


