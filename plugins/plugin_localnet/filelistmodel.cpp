#include <QUrl>
#include <QDebug>
#include <QDateTime>
#include <QFileInfo>

#include "filelistmodel.h"
#include <qglobal.h>
#include <qlist.h>
#include <regex>

using namespace creative_kernel;
#include <QCryptographicHash>
/******************** FileListModel ********************/
FileListModel::FileListModel(QObject *parent) : QAbstractListModel(parent)
{
	m_printerType = 0;
	m_bFirstConnect = false;
	m_timeRegExp = QRegExp("[a-zA-z]{3}[\\s][\\d]{2}[\\s][\\d]{2}:[\\d]{2}");
	m_webInfoRegExp = QRegExp(".+[:](.+)[:]([\\d]+)[:](.+)[:]([\\d]+)[:](.+)[:]/[\\w]+/[\\w]+/[\\w]+(/.+)");
	m_boxInfoRegExp = QRegExp("-rwxrwxrwx[\\s]+[\\d][\\s]+[\\d][\\s]+[\\d][\\s]+([\\d]+)[\\s]([a-zA-z]{3}[\\s][\\d]{2}[\\s](?:[\\d]{2}:[\\d]{2}|[\\s][\\d]{4}))[\\s](.+.gcode)");
}
void FileListModel::switchDataSource(const QString& macAddr, RemotePrinerType remoteType, bool init)
{

}
FileListModel::~FileListModel()
{
	getRemotePrinterManager()->setGetGcodeListCb(nullptr);
	this->disconnect();
}

int FileListModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent)
	return m_GcodeItemList.count();
}

QVariant FileListModel::data(const QModelIndex& index, int role) const
{
	if (index.row() < 0 || index.row() >= rowCount()) return QVariant();

	QVariant value;
	FileModelItem* modelItem = m_GcodeItemList.at(index.row());
	QList<QString> filenames;
	QList<FileModelItem*> items;
	switch (role)
	{
		case FileModelItem::E_GCodeFileName:
			value = modelItem->gCodeFileName();
			break;
		case FileModelItem::E_GCodePrefixPath:
			value = modelItem->gCodePrefixPath();
			break;
		case FileModelItem::E_GCodeFileSize:
			value = modelItem->gCodeFileSize();
			break;
		case FileModelItem::E_GCodeFileTime:
			value = modelItem->gCodeFileTime();
			break;
		case FileModelItem::E_GCodeLayerHeight:
			value = modelItem->gCodeLayerHeight();
			break;
		case FileModelItem::E_GCodeMaterialLength:
			value = modelItem->gCodeMaterialLength();
			break;
		case FileModelItem::E_GCodeThumbnailImage:
			value = modelItem->gCodeThumbnailImage();
			break;
		case FileModelItem::E_GCodeImportProgress:
			value = modelItem->gCodeImportProgress();
			break;
		case FileModelItem::E_GCodeExportProgress:
			value = modelItem->gCodeExportProgress();
			break;
		case FileModelItem::E_GCodeSelected:
			value = QVariant::fromValue(modelItem);
			break;
		case FileModelItem::E_GCodeIsMultiColor:
			value = modelItem->gCodeIsMultiColor();
			break;
		case FileModelItem::E_FileType:
			value = QVariant::fromValue(modelItem->type());
			break;
		case FileModelItem::E_FileListModel:
			value = QVariant::fromValue(modelItem->getItemFiles());
			break;
		case FileModelItem::E_GCodeFileNames:
			items = modelItem->getItemFiles();
			
			filenames << modelItem->gCodeFileName();
			
			
			for(int i=0;i<items.size();i++)
			{
				if(items[i]->gCodeFileName()!="")
					filenames << items[i]->gCodeFileName();
			}
			
			value = QVariant::fromValue(filenames);
			break;
		default:
			break;
	}
	return value;
}

QHash<int, QByteArray> FileListModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[FileModelItem::E_GCodeFileName]		 = "gcodeFileName";
	roles[FileModelItem::E_GCodeFileSize]		 = "gcodeFileSize";
	roles[FileModelItem::E_GCodeFileTime]		 = "gcodeFileTime";
	roles[FileModelItem::E_GCodeLayerHeight]	 = "gcodeLayerHeight";
	roles[FileModelItem::E_GCodeMaterialLength] = "gcodeMaterialLength";
	roles[FileModelItem::E_GCodeThumbnailImage] = "gCodeThumbnailImage";
	roles[FileModelItem::E_GCodeImportProgress] = "gCodeImportProgress";
	roles[FileModelItem::E_GCodeExportProgress] = "gCodeExportProgress";
	roles[FileModelItem::E_GCodePrefixPath] = "gCodePrefixPath";

	roles[FileModelItem::E_GCodeIsMultiColor] = "gCodeIsMultiColor";
	roles[FileModelItem::E_GCodeSelected] = "mItem";
	roles[FileModelItem::E_FileType] = "type";
	roles[FileModelItem::E_FileListModel] = "fileListModel";
	roles[FileModelItem::E_GCodeFileNames]		 = "gcodeFileNames";
	return roles;
}

void FileListModel::findGcodeFileList()
{
	if (!m_bFirstConnect)
	{
		m_bFirstConnect = true;
		connect(this, &FileListModel::sigGetGcodeFileList, this, &FileListModel::slotGetGcodeFileList, Qt::ConnectionType::QueuedConnection);
		//getRemotePrinterManager()->setGetGcodeListCb(std::bind(&FileListModel::onGetGcodeFileList, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	}
}

bool FileListModel::isGcodeFileExist(const QString& macAddr, const QString& fileName)
{
    Q_FOREACH(auto item, m_GcodeItemList)
    {
        if(item->gCodeFileName()==fileName)
        {
            return true;
        }
    }
	return false;
}

void FileListModel::deleteGcodeFile(const QString& ipAddress, const QString& filePath, int remoteType)
{
	getRemotePrinterManager()->deleteFile(ipAddress.toStdString(), filePath.toStdString(), RemotePrinerType(remoteType), OpPrinerFileType::GCODE_FILE);
}

void FileListModel::deleteSelectedGcodeFile(const QString& ipAddress)
{
	for(auto item:m_GcodeItemList)
	{	
		QString path = item->gCodePrefixPath() + "/" + item->gCodeFileName();
		RemotePrinerType type = RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408;
		if(item->gCodeSelected())
			deleteGcodeFile(ipAddress,path, static_cast<int>(type));
	}
}

void FileListModel::renameGcodeFile(const QString& ipAddress, const QString& filePath, const QString& targetName, int remoteType)
{
	getRemotePrinterManager()->renameFile(ipAddress.toStdString(), filePath.toStdString(), targetName.toStdString(), RemotePrinerType(remoteType), OpPrinerFileType::GCODE_FILE);
}



void FileListModel::downloadGcodeFile(const QString& macAddr, const QString& ipAddress, const QString& fileName, const QString& localPath, const int& remoteType)
{
	cpr::Session session;
	std::string url = "http://" + ipAddress.toStdString() + "/downloads/gcode/";
	url += session.GetCurlHolder()->urlEncode(fileName.toStdString());
	getRemotePrinterManager()->downloadFile(url, QUrl(localPath).toLocalFile().toStdString(), std::bind(&FileListModel::onGetExportProgress, this, macAddr, fileName, std::placeholders::_1));
}


void FileListModel::downloadSelectedGcodeFile(const QString& macAddr,const QString& ipAddress,const QString& localPath)
{
	for(auto item:m_GcodeItemList)
	{	
		if(item->gCodeSelected())
		{
			RemotePrinerType type = RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408;
			QString fileName = item->gCodeFileName();
			QString path = localPath + "/" + fileName;
			downloadGcodeFile(macAddr,ipAddress,fileName, path, static_cast<int>(type));
		}
			
	}
}


void FileListModel::setGcodeFileSelected(bool select)
{
	for(auto item:m_GcodeItemList)
	{	
		item->setGCodeSelected(select);
	}
}

int FileListModel::gcodeFileSelectedCount()
{
	int index = 0;
	for(auto item:m_GcodeItemList)
	{	
		if(item->gCodeSelected()){
			index++;
		}
	}
	return index;
}


void FileListModel::onGetExportProgress(const QString& ipAddress, const QString& fileName, const float& progress)
{
	QString macAddr = ipAddress;

	

	int rowIndex = getItemRowIndex(fileName, m_GcodeItemList);

	if (rowIndex != -1)
	{
		bool success = qFuzzyCompare(progress, 1.0f);
		m_GcodeItemList[rowIndex]->setGCodeExportProgress(progress);
		if (success) m_GcodeItemList[rowIndex]->setGCodeExportProgress(0);//Reset value

		QModelIndex modelIndex = createIndex(rowIndex, 0);
		emit dataChanged(modelIndex, modelIndex, QVector<int>() << FileModelItem::E_GCodeExportProgress);
	}
}

void FileListModel::importMultiFile(const QString& macAddr, const QString& ipAddress, const QList<QUrl>& fileUrls)
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
		std::bind(&FileListModel::ImportMultiFileProgress, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
		std::bind(&FileListModel::ImportMultiFileMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	}
}

int FileListModel::printerType() const
{
	return m_printerType;
}

void FileListModel::onGetGcodeFileList(const std::string& std_macAddr, const std::string& std_list, RemotePrinerType remoteType)
{
	if (std_macAddr.empty()) return;
	
}

void FileListModel::slotGetGcodeFileList(const QString& from_std_macAddr, const QString& from_std_list, RemotePrinerType remoteType)
{
	
	
}
void FileListModel::onGetGcodeFileList(QList<GcodeFile> gcodefiles,time_t updateTime,int printerType)
{
    if(updateTime==m_updateTime)
    {
        return;
    }
	m_printerType = printerType;
	m_updateTime = updateTime;
	qWarning()<<"onGetGcodeFileList:"<<gcodefiles.size();
    beginResetModel();
    Q_FOREACH(auto item, m_GcodeItemList)
    {
		item->disconnect();
        delete item;
    }
    m_GcodeItemList.clear();
    Q_FOREACH(auto file, gcodefiles)
    {
        
        FileModelItem* modelItem = new FileModelItem(this);
        modelItem->setGCodeFileName(QString::fromStdString(file.name));
        modelItem->setType(file.custom_types);
		modelItem->setGCodePrefixPath(QString::fromStdString(file.path.substr(0,file.path.find_last_of("/"))));
		modelItem->setGCodeFileSize(QString::number(file.file_size/1.0/1024/1024,'f',2));
        QDateTime dateTime = QDateTime::fromSecsSinceEpoch(file.create_time);
        QString dateTimeString = dateTime.toString("yyyy-MM-dd HH:mm:ss");
		modelItem->setGCodeFileTime(dateTimeString);
		int thumb_index = QString::fromStdString(file.thumbnail).indexOf("humbnail");
		if(thumb_index>0)
		{
			modelItem->setGCodeThumbnailImage("/downloads/"+QString::fromStdString(file.thumbnail.substr(thumb_index)));
		}
		
		QList<FileModelItem *> fileinfos;
		bool IsMultiColor = false;
		Q_FOREACH(auto fileInfo,file.files)
		{
			modelItem->setGCodeLayerHeight(QString::number(fileInfo.floorHeight/1.0/100,'f',2));
			modelItem->setGCodeMaterialLength(QString::number(fileInfo.consumables/1.0/100,'f',2));
			FileModelItem* modelItemInfo = new FileModelItem(this);
			modelItemInfo->setGCodeFileName(QString::fromStdString(fileInfo.name));
			modelItemInfo->setMaterial(QString::fromStdString(fileInfo.material));
			modelItemInfo->setMaterialColors(QString::fromStdString(fileInfo.materialColors));
			modelItemInfo->setMaterialIds(QString::fromStdString(fileInfo.materialIds));
			QStringList materials = QString::fromStdString(fileInfo.materialColors).split(";");
			if (materials.length() > 1)
			{
				int visibleCount = 0;
				Q_FOREACH(auto material, materials)
				{
					if(material.isEmpty())
						continue;
					visibleCount++;
				}
				if (visibleCount > 1)
					IsMultiColor = true;
			}
			if (file.files.size() > 0)
			{
				if(!fileInfo.thumbnail.empty())
				{
					int thumb_index = QString::fromStdString(fileInfo.thumbnail).indexOf("humbnail");
					if(thumb_index>0)
					{
						modelItemInfo->setGCodeThumbnailImage("/downloads/" + QString::fromStdString(fileInfo.thumbnail.substr(thumb_index)));
					}
				}
				modelItemInfo->setGCodePrefixPath(QString::fromStdString(fileInfo.path.substr(0, fileInfo.path.find_last_of("/")+1)));
	
        		QString dateTimeString = QString("%1h%2m%3s").arg(fileInfo.timeCost / 3600).arg(fileInfo.timeCost / 60 % 60).arg(fileInfo.timeCost % 60);
				modelItemInfo->setGCodeFileTime(dateTimeString);
				modelItemInfo->setGCodeMaterialLength(QString::number(fileInfo.consumables/1.0/100,'f',2));
			}
			
			fileinfos.append(modelItemInfo);
		}
		modelItem->setItemFiles(fileinfos);
		modelItem->setGCodeIsMultiColor(IsMultiColor?"1":"0");
		connect(modelItem, &FileModelItem::gCodeSelectedChanged, this, &FileListModel::gcodeFileSelectedCountChanged);
        m_GcodeItemList.append(modelItem);
    }
    endResetModel();

}

void FileListModel::ImportMultiFileProgress(const std::string& std_macAddr, const std::string& std_fileName, const float& progress)
{
	QString macAddr = QString::fromStdString(std_macAddr);
	QString fileName = QString::fromStdString(std_fileName);


	int rowIndex = getItemRowIndex(fileName, m_GcodeItemList);

	if (rowIndex != -1)
	{
		bool success = qFuzzyCompare(progress, 1.0f);
		m_GcodeItemList[rowIndex]->setGCodeImportProgress(success ? 100.0f : qRound(progress * 100.0f));
		if (success) m_GcodeItemList[rowIndex]->setGCodeImportProgress(0);//Reset value
	}
}

void FileListModel::ImportMultiFileMessage(const std::string& std_macAddr, const std::string& std_fileName, int errorCode)
{
	Q_UNUSED(errorCode)
	QString macAddr = QString::fromStdString(std_macAddr);
	QString fileName = QString::fromStdString(std_fileName);



	int rowIndex = getItemRowIndex(fileName, m_GcodeItemList);

	if (rowIndex != -1)
	{
		m_GcodeItemList[rowIndex]->setGCodeImportProgress(-1);
	}
}

int FileListModel::getItemRowIndex(const QString& fileName, const QList<FileModelItem*>& itemList)
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
QVariant FileListModel::getItemFileByFileName(const QString& fileName)
{
	QList<FileModelItem*> itemFiles;
	for (int i = 0; i < m_GcodeItemList.count(); ++i)
	{
		const auto& item = m_GcodeItemList[i];
		QString path = item->gCodePrefixPath() + "/" + item->gCodeFileName();
		if (path == fileName)
		{
			itemFiles = item->getItemFiles();
			break;
		}
	}
	return  QVariant::fromValue(itemFiles);
}
QString FileListModel::getThumbByFileName(const QString& fileName)
{
	QString thumb;
		for (int i = 0; i < m_GcodeItemList.count(); ++i)
	{
		const auto& item = m_GcodeItemList[i];
		QString path = item->gCodePrefixPath() + "/" + item->gCodeFileName();
		if (path == fileName)
		{
			thumb = item->gCodeThumbnailImage();
			break;
		}
	}
	return thumb;
}

/******************** FileModelItem ********************/
FileModelItem::FileModelItem(QObject* parent) : QObject(parent)
{
	m_gCodeFileName        = QString();
	m_gCodeFileSize        = QString();
	m_gCodeFileTime	       = QString();
	m_gCodeLayerHeight	   = QString();
	m_gCodeMaterialLength  = QString();
	m_gCodeThumbnailImage  = QString();
	m_gCodeImportProgress = 0;
	m_gCodeExportProgress  = 0;
	m_gCodeSelected        = false;
}
QList<FileModelItem*> FileModelItem::getItemFiles()
{
	return m_itemFiles;
}
FileModelItem::FileModelItem(const FileModelItem& FileModelItem) : QObject(nullptr)
{
	Q_UNUSED(FileModelItem)
}

FileModelItem::~FileModelItem()
{

}
void FileModelItem::setMaterial(const QString& material)
{
	m_material = material;
}
void FileModelItem::setMaterialColors(const QString& colors)
{
	m_materialColors = colors;
}
void FileModelItem::setMaterialIds(const QString& ids)
{
	m_materialIds = ids;
}
const QString& FileModelItem::material() const
{
	return m_material;
}
const QString& FileModelItem::materialColors() const
{
		return m_materialColors;
	}
const QString& FileModelItem::materialIds() const
{
	return m_materialIds;
}
const QString& FileModelItem::gCodeFileName() const
{
	return m_gCodeFileName;
}

void FileModelItem::setGCodeFileName(const QString& fileName)
{
	m_gCodeFileName = fileName;
	emit gCodeFileNameChanged();
}

const QString& FileModelItem::gCodePrefixPath() const
{
	return m_gCodePrefixPath;
}
int FileModelItem::type() const
{
    return m_type;
}
void FileModelItem::setType(int type)
{
	m_type = type;
}

void FileModelItem::setGCodePrefixPath(const QString& path)
{
	m_gCodePrefixPath = path;
	emit gCodePrefixPathChanged();
}

const QString& FileModelItem::gCodeFileSize() const
{
	return m_gCodeFileSize;
}

void FileModelItem::setGCodeFileSize(const QString& fileSize)
{
	m_gCodeFileSize = fileSize;
	emit gCodeFileSizeChanged();
}

const QString& FileModelItem::gCodeFileTime() const
{
	return m_gCodeFileTime;
}

void FileModelItem::setGCodeFileTime(const QString& fileTime)
{
	m_gCodeFileTime = fileTime;
	emit gCodeFileTimeChanged();
}

const QString& FileModelItem::gCodeLayerHeight() const
{
	return m_gCodeLayerHeight;
}

void FileModelItem::setGCodeLayerHeight(const QString& height)
{
	m_gCodeLayerHeight = height;
	emit gCodeLayerHeightChanged();
}

const QString& FileModelItem::gCodeMaterialLength() const
{
	return m_gCodeMaterialLength;
}

void FileModelItem::setGCodeMaterialLength(const QString& length)
{
	m_gCodeMaterialLength = length;
	emit gCodeMaterialLengthChanged();
}

const QString& FileModelItem::gCodeThumbnailImage() const
{
	return m_gCodeThumbnailImage;
}

void FileModelItem::setGCodeThumbnailImage(const QString& image)
{
	m_gCodeThumbnailImage = image;
	emit gCodeThumbnailImageChanged();
}

float FileModelItem::gCodeImportProgress() const
{
	return m_gCodeImportProgress;
}

void FileModelItem::setGCodeImportProgress(float progress)
{
	if (qFuzzyCompare(m_gCodeImportProgress, progress))
		return;

	m_gCodeImportProgress = progress;
	emit gCodeImportProgressChanged();
}

float FileModelItem::gCodeExportProgress() const
{
	return m_gCodeExportProgress;
}

void FileModelItem::setGCodeExportProgress(float progress)
{
	if (qFuzzyCompare(m_gCodeExportProgress, progress))
		return;

	m_gCodeExportProgress = progress;
	emit gCodeExportProgressChanged();
}


bool FileModelItem::gCodeSelected() const
{
	return m_gCodeSelected;
}

void FileModelItem::setGCodeSelected(bool selected)
{
	if (m_gCodeSelected == selected)
		return;

	m_gCodeSelected = selected;
	emit gCodeSelectedChanged();
}

QString FileModelItem::gCodeIsMultiColor() const
{
	return m_gCodeIsMultiColor;
}

void FileModelItem::setGCodeIsMultiColor(const QString& isMulti)
{
	if (m_gCodeIsMultiColor == isMulti)
		return;

	m_gCodeIsMultiColor = isMulti;
	emit gCodeIsMultiColorChanged();
}

