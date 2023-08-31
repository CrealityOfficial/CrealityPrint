#include <QUrl>
#include <QDebug>
#include <QDateTime>
#include <QFileInfo>

#include "gcodelistmodel.h"

using namespace creative_kernel;

/******************** GcodeListModel ********************/
GcodeListModel::GcodeListModel(QObject *parent) : QAbstractListModel(parent)
{
	m_printerType = 0;
	m_bFirstConnect = false;
	m_timeRegExp = QRegExp("[a-zA-z]{3}[\\s][\\d]{2}[\\s][\\d]{2}:[\\d]{2}");
	m_webInfoRegExp = QRegExp(".+[:](.+)[:]([\\d]+)[:](.+)[:]([\\d]+)[:](.+)[:]/[\\w]+/[\\w]+/[\\w]+(/.+)");
	m_boxInfoRegExp = QRegExp("-rwxrwxrwx[\\s]+[\\d][\\s]+[\\d][\\s]+[\\d][\\s]+([\\d]+)[\\s]([a-zA-z]{3}[\\s][\\d]{2}[\\s](?:[\\d]{2}:[\\d]{2}|[\\s][\\d]{4}))[\\s](.+.gcode)");
}

GcodeListModel::~GcodeListModel()
{
	getRemotePrinterManager()->setGetGcodeListCb(nullptr);
	this->disconnect();
}

int GcodeListModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent)
	return m_GcodeItemList.count();
}

QVariant GcodeListModel::data(const QModelIndex& index, int role) const
{
	if (index.row() < 0 || index.row() >= rowCount()) return QVariant();

	QVariant value;
	GcodeModelItem* modelItem = m_GcodeItemList.at(index.row());

	switch (role)
	{
		case GcodeModelItem::E_GCodeFileName:
			value = modelItem->gCodeFileName();
			break;
		case GcodeModelItem::E_GCodeFileSize:
			value = modelItem->gCodeFileSize();
			break;
		case GcodeModelItem::E_GCodeFileTime:
			value = modelItem->gCodeFileTime();
			break;
		case GcodeModelItem::E_GCodeLayerHeight:
			value = modelItem->gCodeLayerHeight();
			break;
		case GcodeModelItem::E_GCodeMaterialLength:
			value = modelItem->gCodeMaterialLength();
			break;
		case GcodeModelItem::E_GCodeThumbnailImage:
			value = modelItem->gCodeThumbnailImage();
			break;
		case GcodeModelItem::E_GCodeImportProgress:
			value = modelItem->gCodeImportProgress();
			break;
		case GcodeModelItem::E_GCodeExportProgress:
			value = modelItem->gCodeExportProgress();
			break;
		default:
			break;
	}
	return value;
}

QHash<int, QByteArray> GcodeListModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[GcodeModelItem::E_GCodeFileName]		 = "gcodeFileName";
	roles[GcodeModelItem::E_GCodeFileSize]		 = "gcodeFileSize";
	roles[GcodeModelItem::E_GCodeFileTime]		 = "gcodeFileTime";
	roles[GcodeModelItem::E_GCodeLayerHeight]	 = "gcodeLayerHeight";
	roles[GcodeModelItem::E_GCodeMaterialLength] = "gcodeMaterialLength";
	roles[GcodeModelItem::E_GCodeThumbnailImage] = "gCodeThumbnailImage";
	roles[GcodeModelItem::E_GCodeImportProgress] = "gCodeImportProgress";
	roles[GcodeModelItem::E_GCodeExportProgress] = "gCodeExportProgress";
	return roles;
}

void GcodeListModel::findGcodeFileList()
{
	if (!m_bFirstConnect)
	{
		m_bFirstConnect = true;
		connect(this, &GcodeListModel::sigGetGcodeFileList, this, &GcodeListModel::slotGetGcodeFileList, Qt::ConnectionType::QueuedConnection);
		getRemotePrinterManager()->setGetGcodeListCb(std::bind(&GcodeListModel::onGetGcodeFileList, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	}
}

bool GcodeListModel::isGcodeFileExist(const QString& macAddr, const QString& fileName)
{
	std::string std_macAddr = macAddr.toStdString();
	std::string std_fileName = fileName.toStdString();

	return (m_GcodeInfoMap[std_macAddr].find(std_fileName) != std::string::npos);
}

void GcodeListModel::deleteGcodeFile(const QString& ipAddress, const QString& filePath, int remoteType)
{
	getRemotePrinterManager()->deleteFile(ipAddress.toStdString(), filePath.toStdString(), RemotePrinerType(remoteType), OpPrinerFileType::GCODE_FILE);
}

void GcodeListModel::renameGcodeFile(const QString& ipAddress, const QString& filePath, const QString& targetName, int remoteType)
{
	getRemotePrinterManager()->renameFile(ipAddress.toStdString(), filePath.toStdString(), targetName.toStdString(), RemotePrinerType(remoteType), OpPrinerFileType::GCODE_FILE);
}

void GcodeListModel::switchDataSource(const QString& macAddr, RemotePrinerType remoteType, bool init)//@true: init @false: update
{
	static QString curMacAddr = QString();
	if (init) curMacAddr = macAddr;

	if (curMacAddr.compare(macAddr) == 0 && m_GcodeItemMap.contains(curMacAddr))
	{
		m_printerType = int(remoteType);
		emit printerTypeChanged();

		beginResetModel();
		m_GcodeItemList = m_GcodeItemMap[curMacAddr];
		endResetModel();
	}
}

void GcodeListModel::downloadGcodeFile(const QString& ipAddress, const QString& fileName, const QString& localPath, const int& remoteType)
{
	cpr::Session session;
	std::string url = "http://" + ipAddress.toStdString() + "/downloads/gcode/";
	url += session.GetCurlHolder()->urlEncode(fileName.toStdString());
	getRemotePrinterManager()->downloadFile(url, QUrl(localPath).toLocalFile().toStdString(), std::bind(&GcodeListModel::onGetExportProgress, this, ipAddress, fileName, std::placeholders::_1));
}

void GcodeListModel::onGetExportProgress(const QString& ipAddress, const QString& fileName, const float& progress)
{
	QString macAddr = ipAddress;

	if (!m_GcodeItemMap.contains(macAddr)) return;

	QList<GcodeModelItem*> itemList = m_GcodeItemMap[macAddr];

	int rowIndex = getItemRowIndex(fileName, itemList);

	if (rowIndex != -1)
	{
		bool success = qFuzzyCompare(progress, 1.0f);
		itemList[rowIndex]->setGCodeExportProgress(progress);
		if (success) itemList[rowIndex]->setGCodeExportProgress(0);//Reset value

		QModelIndex modelIndex = createIndex(rowIndex, 0);
		emit dataChanged(modelIndex, modelIndex, QVector<int>() << GcodeModelItem::E_GCodeExportProgress);
	}
}

void GcodeListModel::importMultiFile(const QString& macAddr, const QString& ipAddress, const QList<QUrl>& fileUrls)
{
	foreach(QUrl fileUrl, fileUrls)
	{
		RemotePrinter printer;
		printer.macAddress = macAddr;
		printer.ipAddress = ipAddress;
		printer.type = RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408;

		QString fileName = fileUrl.fileName();
		QString filePath = fileUrl.toLocalFile();

		getRemotePrinterManager()->pushFile(printer, fileName.toStdString(), filePath.toStdString(),
		std::bind(&GcodeListModel::ImportMultiFileProgress, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&GcodeListModel::ImportMultiFileMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	}
}

int GcodeListModel::printerType() const
{
	return m_printerType;
}

void GcodeListModel::onGetGcodeFileList(const std::string& std_macAddr, const std::string& std_list, RemotePrinerType remoteType)
{
	if (std_macAddr.empty()) return;
	if (!m_GcodeInfoMap.contains(std_macAddr) || m_GcodeInfoMap[std_macAddr].compare(std_list) != 0)
	{
		m_GcodeInfoMap[std_macAddr] = std_list;
		QString from_std_list = QString::fromStdString(std_list);
		QString from_std_macAddr = QString::fromStdString(std_macAddr);
		emit sigGetGcodeFileList(from_std_macAddr, from_std_list, remoteType);
	}
}

void GcodeListModel::slotGetGcodeFileList(const QString& from_std_macAddr, const QString& from_std_list, RemotePrinerType remoteType)
{
	QList<GcodeModelItem*> itemList;

	switch (remoteType)
	{
		case RemotePrinerType::REMOTE_PRINTER_TYPE_LAN:
		{
			QStringList splitList = from_std_list.split("\r\n", QString::SkipEmptyParts);
			foreach(QString fullInfo, splitList)
			{
				if (m_boxInfoRegExp.exactMatch(fullInfo))
				{
					GcodeModelItem* modelItem = new GcodeModelItem(this);
					QString fileSize = QString::number(m_boxInfoRegExp.cap(1).toDouble() / 1024.0 / 1024.0, 'f', 2);
					QString fileTime = m_boxInfoRegExp.cap(2);
					QString fileName = m_boxInfoRegExp.cap(3);

					//within half a year
					QDateTime fileDateTime;
					if (m_timeRegExp.exactMatch(fileTime))
					{
						fileTime += QDate::currentDate().toString(" yyyy");
						fileDateTime = QLocale(QLocale::English).toDateTime(fileTime, "MMM dd HH:mm yyyy");
						if (fileDateTime > QDateTime::currentDateTime()) fileDateTime = fileDateTime.addYears(-1);
					}
					else 
					{
						fileDateTime = QLocale(QLocale::English).toDateTime(fileTime, "MMM dd HH:mm yyyy");
					}

					modelItem->setGCodeFileSize(fileSize);
					modelItem->setGCodeFileTime(fileDateTime.toString("yyyy-MM-dd hh:mm:ss"));
					modelItem->setGCodeFileName(fileName);

					itemList.append(modelItem);
				}
			}
		}	
			break;
		case RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408:
		{
			QStringList splitList = from_std_list.split(";", QString::SkipEmptyParts);
			foreach(QString fullInfo, splitList)
			{
				if (m_webInfoRegExp.exactMatch(fullInfo))
				{
					GcodeModelItem* modelItem = new GcodeModelItem(this);

					QString gcodeFileName  = m_webInfoRegExp.cap(1);
					QString gcodeFileSize  = QString::number(m_webInfoRegExp.cap(2).toDouble() / 1024.0 / 1024.0, 'f', 2);
					QString layerHeight	   = QString::number(m_webInfoRegExp.cap(3).toDouble(), 'f', 2);
					QString gcodeFileTime  = QDateTime::fromSecsSinceEpoch(m_webInfoRegExp.cap(4).toLongLong()).toString("yyyy-MM-dd hh:mm:ss");
					QString materialLength = QString::number(m_webInfoRegExp.cap(5).toDouble() / 1000.0, 'f', 3);
					QString thumbnailImage = ":80/downloads" + m_webInfoRegExp.cap(6);

					modelItem->setGCodeFileName(gcodeFileName);
					modelItem->setGCodeFileSize(gcodeFileSize);
					modelItem->setGCodeFileTime(gcodeFileTime);
					modelItem->setGCodeLayerHeight(layerHeight);
					modelItem->setGCodeMaterialLength(materialLength);
					modelItem->setGCodeThumbnailImage(thumbnailImage);

					itemList.append(modelItem);
				}
			}
		}
			break;
		default:
			break;
	}

	m_GcodeItemMap[from_std_macAddr] = itemList;
	switchDataSource(from_std_macAddr, remoteType, false);
}

void GcodeListModel::ImportMultiFileProgress(const std::string& std_macAddr, const std::string& std_fileName, const float& progress)
{
	QString macAddr = QString::fromStdString(std_macAddr);
	QString fileName = QString::fromStdString(std_fileName);

	if (!m_GcodeItemMap.contains(macAddr)) return;

	QList<GcodeModelItem*> itemList = m_GcodeItemMap[macAddr];

	int rowIndex = getItemRowIndex(fileName, itemList);

	if (rowIndex != -1)
	{
		bool success = qFuzzyCompare(progress, 1.0f);
		itemList[rowIndex]->setGCodeImportProgress(success ? 100.0f : qRound(progress * 100.0f));
		if (success) itemList[rowIndex]->setGCodeImportProgress(0);//Reset value
	}
}

void GcodeListModel::ImportMultiFileMessage(const std::string& std_macAddr, const std::string& std_fileName, int errorCode)
{
	Q_UNUSED(errorCode)
	QString macAddr = QString::fromStdString(std_macAddr);
	QString fileName = QString::fromStdString(std_fileName);

	if (!m_GcodeItemMap.contains(macAddr)) return;

	QList<GcodeModelItem*> itemList = m_GcodeItemMap[macAddr];

	int rowIndex = getItemRowIndex(fileName, itemList);

	if (rowIndex != -1)
	{
		itemList[rowIndex]->setGCodeImportProgress(-1);
	}
}

int GcodeListModel::getItemRowIndex(const QString& fileName, const QList<GcodeModelItem*>& itemList)
{
	for (int i = 0; i < itemList.count(); ++i)
	{
		const auto& item = itemList[i];
		if (item->gCodeFileName() == fileName)
		{
			return i;
		}
	}
	return -1;
}


/******************** GcodeModelItem ********************/
GcodeModelItem::GcodeModelItem(QObject* parent) : QObject(parent)
{
	m_gCodeFileName        = QString();
	m_gCodeFileSize        = QString();
	m_gCodeFileTime	       = QString();
	m_gCodeLayerHeight	   = QString();
	m_gCodeMaterialLength  = QString();
	m_gCodeThumbnailImage  = QString();
	m_gCodeImportProgress = 0;
	m_gCodeExportProgress  = 0;
}

GcodeModelItem::GcodeModelItem(const GcodeModelItem& GcodeModelItem) : QObject(nullptr)
{
	Q_UNUSED(GcodeModelItem)
}

GcodeModelItem::~GcodeModelItem()
{

}

const QString& GcodeModelItem::gCodeFileName() const
{
	return m_gCodeFileName;
}

void GcodeModelItem::setGCodeFileName(const QString& fileName)
{
	m_gCodeFileName = fileName;
	emit gCodeFileNameChanged();
}

const QString& GcodeModelItem::gCodeFileSize() const
{
	return m_gCodeFileSize;
}

void GcodeModelItem::setGCodeFileSize(const QString& fileSize)
{
	m_gCodeFileSize = fileSize;
	emit gCodeFileSizeChanged();
}

const QString& GcodeModelItem::gCodeFileTime() const
{
	return m_gCodeFileTime;
}

void GcodeModelItem::setGCodeFileTime(const QString& fileTime)
{
	m_gCodeFileTime = fileTime;
	emit gCodeFileTimeChanged();
}

const QString& GcodeModelItem::gCodeLayerHeight() const
{
	return m_gCodeLayerHeight;
}

void GcodeModelItem::setGCodeLayerHeight(const QString& height)
{
	m_gCodeLayerHeight = height;
	emit gCodeLayerHeightChanged();
}

const QString& GcodeModelItem::gCodeMaterialLength() const
{
	return m_gCodeMaterialLength;
}

void GcodeModelItem::setGCodeMaterialLength(const QString& length)
{
	m_gCodeMaterialLength = length;
	emit gCodeMaterialLengthChanged();
}

const QString& GcodeModelItem::gCodeThumbnailImage() const
{
	return m_gCodeThumbnailImage;
}

void GcodeModelItem::setGCodeThumbnailImage(const QString& image)
{
	m_gCodeThumbnailImage = image;
	emit gCodeThumbnailImageChanged();
}

float GcodeModelItem::gCodeImportProgress() const
{
	return m_gCodeImportProgress;
}

void GcodeModelItem::setGCodeImportProgress(float progress)
{
	if (qFuzzyCompare(m_gCodeImportProgress, progress))
		return;

	m_gCodeImportProgress = progress;
	emit gCodeImportProgressChanged();
}

float GcodeModelItem::gCodeExportProgress() const
{
	return m_gCodeExportProgress;
}

void GcodeModelItem::setGCodeExportProgress(float progress)
{
	if (qFuzzyCompare(m_gCodeExportProgress, progress))
		return;

	m_gCodeExportProgress = progress;
	emit gCodeExportProgressChanged();
}
