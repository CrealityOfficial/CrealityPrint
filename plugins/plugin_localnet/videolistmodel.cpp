#include <QUrl>
#include <QDebug>
#include <QDateTime>
#include <QFileInfo>

#include "videolistmodel.h"
#include "cxkernel/interface/iointerface.h"
#include <QtCore/qfuture.h>
#include <QtConcurrent/qtconcurrentrun.h>

using namespace creative_kernel;

/******************** VideoListModel ********************/
VideoListModel::VideoListModel(QObject *parent) : QAbstractListModel(parent)
{
	m_printerType = 0;
	m_bFirstConnect = false;
	m_webInfoRegExp = QRegExp("([\\d]+)[:](/.+)[:]([\\d]+)[:](/.+)[:]([\\d]+)[:](/.+)[:]([\\d]+)[:]([\\d]+)[:](.+)");
}

VideoListModel::~VideoListModel()
{
	getRemotePrinterManager()->setGetVideoListCb(nullptr);
	this->disconnect();
}

int VideoListModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent)
	return m_videoItemList.count();
}

QVariant VideoListModel::data(const QModelIndex& index, int role) const
{
	if (index.row() < 0 || index.row() >= rowCount()) return QVariant();

	QVariant value;
	VideoModelItem* modelItem = m_videoItemList.at(index.row());

	switch (role)
	{
		case VideoModelItem::E_GCodeFileName:
			value = modelItem->gcodeFileName();
			break;
		case VideoModelItem::E_VideoFileSize:
			value = modelItem->videoFileSize();
			break;
		case VideoModelItem::E_VideoFileName:
			value = modelItem->videoFileName();
			break;
		case VideoModelItem::E_StartTime:
			value = modelItem->startTime();
			break;
		case VideoModelItem::E_PrintTime:
			value = modelItem->printTime();
			break;
		case VideoModelItem::E_VideoCover:
			value = modelItem->videoCover();
			break;
		case VideoModelItem::E_ExportProgress:
			value = modelItem->exportProgress();
			break;
		case VideoModelItem::E_VideoFileShowName:
			value = modelItem->videoFileShowName();
			break;
		default:
			break;
	}
	return value;
}

QHash<int, QByteArray> VideoListModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[VideoModelItem::E_GCodeFileName]		 = "gcodeFileName";
	roles[VideoModelItem::E_VideoFileSize]		 = "videoFileSize";
	roles[VideoModelItem::E_VideoFileName]		 = "videoFileName";
	roles[VideoModelItem::E_StartTime]	 = "startTime";
	roles[VideoModelItem::E_PrintTime] = "printTime";
	roles[VideoModelItem::E_VideoCover] = "videoCover";
	roles[VideoModelItem::E_ExportProgress] = "exportProgress";
	roles[VideoModelItem::E_VideoFileShowName] = "videoFileShowName";
	return roles;
}

void VideoListModel::findVideoFileList()
{
	if (!m_bFirstConnect)
	{
		m_bFirstConnect = true;
		connect(this, &VideoListModel::sigGetVideoFileList, this, &VideoListModel::slotGetVideoFileList, Qt::ConnectionType::QueuedConnection);
		getRemotePrinterManager()->setGetVideoListCb(std::bind(&VideoListModel::onGetVideoFileList, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	}
}

void VideoListModel::deleteVideoFile(const QString& ipAddress, const QString& filePath, int remoteType)
{
	getRemotePrinterManager()->deleteFile(ipAddress.toStdString(), filePath.toStdString(), RemotePrinerType(remoteType), OpPrinerFileType::VIDEO_FILE);
}

void VideoListModel::renameVideoFile(const QString& ipAddress, const QString& filePath, const QString& targetName, int remoteType)
{
	getRemotePrinterManager()->renameFile(ipAddress.toStdString(), filePath.toStdString(), targetName.toStdString(),RemotePrinerType(remoteType), OpPrinerFileType::VIDEO_FILE);
}

void VideoListModel::switchDataSource(const QString& macAddr, RemotePrinerType remoteType, bool init)//@true: init @false: update
{
	static QString curMacAddr = QString();
	if (init) curMacAddr = macAddr;

	if (curMacAddr.compare(macAddr) == 0 && m_videoItemMap.contains(curMacAddr))
	{
		m_printerType = int(remoteType);
		emit printerTypeChanged();

		beginResetModel();
		m_videoItemList = m_videoItemMap[curMacAddr];
		endResetModel();
	}
}

void VideoListModel::downloadVideoFile(const QString& ipAddress, const QString& fileName, const QString& localPath, const int& remoteType)
{
	cpr::Session session;
	std::string url = "http://" + ipAddress.toStdString() + "/downloads/video/";
	url += session.GetCurlHolder()->urlEncode(fileName.toStdString());
	getRemotePrinterManager()->downloadFile(url, QUrl(localPath).toLocalFile().toStdString(), std::bind(&VideoListModel::onGetExportProgress, this, ipAddress, fileName, std::placeholders::_1));
}

void VideoListModel::onGetExportProgress(const QString& ipAddress, const QString& fileName, const float& progress)
{
	QString macAddr = ipAddress;

	if (!m_videoItemMap.contains(macAddr)) return;

	auto itemList = m_videoItemMap[macAddr];

	int rowIndex = getItemRowIndex(fileName, itemList);

	if (rowIndex != -1)
	{
		bool success = qFuzzyCompare(progress, 1.0f);
		itemList[rowIndex]->setExportProgress(progress);
		if (success) itemList[rowIndex]->setExportProgress(0);//Reset value

		QModelIndex modelIndex = createIndex(rowIndex, 0);
		emit dataChanged(modelIndex, modelIndex, QVector<int>() << VideoModelItem::E_ExportProgress);
	}
}

int VideoListModel::printerType() const
{
	return m_printerType;
}

void VideoListModel::onGetVideoFileList(const std::string& std_macAddr, const std::string& std_list, RemotePrinerType remoteType)
{
	if (std_macAddr.empty()) return;
	if (!m_videoInfoMap.contains(std_macAddr) || m_videoInfoMap[std_macAddr].compare(std_list) != 0)
	{
		m_videoInfoMap[std_macAddr] = std_list;
		QString from_std_list = QString::fromStdString(std_list);
		QString from_std_macAddr = QString::fromStdString(std_macAddr);
		emit sigGetVideoFileList(from_std_macAddr, from_std_list, remoteType);
	}
}

void VideoListModel::slotGetVideoFileList(const QString& from_std_macAddr, const QString& from_std_list, RemotePrinerType remoteType)
{
	QList<VideoModelItem*> itemList;

	switch (remoteType)
	{
		case RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408:
		{
			QStringList splitList = from_std_list.split(";", QString::SkipEmptyParts);
			foreach(QString fullInfo, splitList)
			{
				if (m_webInfoRegExp.exactMatch(fullInfo))
				{
					VideoModelItem* modelItem = new VideoModelItem(this);

					QString gcodeFileName  = m_webInfoRegExp.cap(2).split("/", QString::SkipEmptyParts).last();
					QString videoFileSize  = QString::number(m_webInfoRegExp.cap(3).toDouble() / 1024.0 / 1024.0, 'f', 2);
					QString videoFileName = m_webInfoRegExp.cap(4).split("/", QString::SkipEmptyParts).last();
					QString duration = m_webInfoRegExp.cap(5);
					QString cover = "/downloads/video/" + m_webInfoRegExp.cap(6);
					QString starttime = QDateTime::fromSecsSinceEpoch(m_webInfoRegExp.cap(7).toLongLong()).toString("yyyy-MM-dd hh:mm:ss");
					QString printtime = m_webInfoRegExp.cap(8);
					QString videoFileShowName = m_webInfoRegExp.cap(9);

					modelItem->setGcodeFileName(gcodeFileName);
					modelItem->setVideoFileSize(videoFileSize);
					modelItem->setVideoFileName(videoFileName);
					modelItem->setStartTime(starttime);
					modelItem->setPrintTime(printtime);
					modelItem->setVideoCover(cover);
					modelItem->setVideoFileShowName(videoFileShowName);

					itemList.append(modelItem);
				}
			}
		}
			break;
		default:
			break;
	}

	m_videoItemMap[from_std_macAddr] = itemList;
	switchDataSource(from_std_macAddr, remoteType, false);
}

int VideoListModel::getItemRowIndex(const QString& fileName, const QList<VideoModelItem*>& itemList)
{
	for (int i = 0; i < itemList.count(); ++i)
	{
		const auto& item = itemList[i];
		if (item->videoFileName() == fileName)
		{
			return i;
		}
	}
	return -1;
}


/******************** VideoModelItem ********************/
VideoModelItem::VideoModelItem(QObject* parent) : QObject(parent)
{
}

VideoModelItem::VideoModelItem(const VideoModelItem& VideoModelItem) : QObject(nullptr)
{
	Q_UNUSED(VideoModelItem)
}

VideoModelItem::~VideoModelItem()
{

}

const QString& VideoModelItem::gcodeFileName() const
{
	return m_gCodeFileName;
}

void VideoModelItem::setGcodeFileName(const QString& fileName)
{
	if (m_gCodeFileName == fileName)
	{
		return;
	}
	m_gCodeFileName = fileName;
	emit gcodeFileNameChanged();
}

const QString& VideoModelItem::videoFileSize() const
{
	return m_videoFileSize;
}

void VideoModelItem::setVideoFileSize(const QString& fileSize)
{
	if (m_videoFileSize == fileSize)
	{
		return;
	}
	m_videoFileSize = fileSize;
	emit videoFileSizeChanged();
}

const QString& VideoModelItem::videoFileName() const
{
	return m_videoFileName;
}

void VideoModelItem::setVideoFileName(const QString& fileName)
{
	if (m_videoFileName == fileName)
	{
		return;
	}
	m_videoFileName = fileName;
	emit videoFileNameChanged();
}

const QString& VideoModelItem::startTime() const
{
	return m_startTime;
}

void VideoModelItem::setStartTime(const QString& startTime)
{
	if (m_startTime == startTime)
	{
		return;
	}
	m_startTime = startTime;
	emit startTimeChanged();
}

const QString& VideoModelItem::printTime() const
{
	return m_printTime;
}

void VideoModelItem::setPrintTime(const QString& printTime)
{
	if (m_printTime == printTime)
	{
		return;
	}
	m_printTime = printTime;
	emit printTimeChanged();
}

const QString& VideoModelItem::videoCover() const
{
	return m_videoCover;
}

void VideoModelItem::setVideoCover(const QString& cover)
{
	if (m_videoCover == cover)
	{
		return;
	}
	m_videoCover = cover;
	emit videoCoverChanged();
}

float VideoModelItem::exportProgress() const
{
	return m_exportProgress;
}

void VideoModelItem::setExportProgress(float progress)
{
	if (qFuzzyCompare(m_exportProgress, progress))
		return;

	m_exportProgress = progress;
	emit exportProgressChanged();
}

const QString& VideoModelItem::videoFileShowName() const
{
	return m_videoFileShowName;
}

void VideoModelItem::setVideoFileShowName(const QString& fileName)
{
	if (m_videoFileShowName == fileName)
	{
		return;
	}
	m_videoFileShowName = fileName;
	emit videoFileShowNameChanged();
}
