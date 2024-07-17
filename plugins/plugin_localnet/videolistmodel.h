#ifndef VIDEOLISTMODEL_H
#define VIDEOLISTMODEL_H

#include <QAbstractListModel>
#include "localnetworkinterface/RemotePrinterManager.h"

class VideoModelItem;
class VideoListModel: public QAbstractListModel
{
	Q_OBJECT

public:
	Q_PROPERTY(int printerType READ printerType NOTIFY printerTypeChanged)
	explicit VideoListModel(QObject* parent = nullptr);
	~VideoListModel();

	Q_INVOKABLE void findVideoFileList();
	Q_INVOKABLE void deleteVideoFile(const QString& ipAddress, const QString& filePath, int remoteType);
	Q_INVOKABLE void renameVideoFile(const QString& ipAddress, const QString& filePath, const QString& targetName, int remoteType);
	Q_INVOKABLE void switchDataSource(const QString& macAddr, RemotePrinerType remoteType, bool init = true);
	Q_INVOKABLE void downloadVideoFile(const QString& macAddr, const QString& ipAddress, const QString& fileName, const QString& localPath, const int& remoteType);
	void onGetExportProgress(const QString& ipAddress, const QString& fileName, const float& progress);

	int printerType() const;
	void onGetVideoFileList(const std::string& std_macAddr, const std::string& std_list, RemotePrinerType remoteType);

protected:
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;

signals:
	void printerTypeChanged();
	void sigGetVideoFileList(const QString& from_std_macAddr, const QString& from_std_list, RemotePrinerType remoteType);

private slots:
	void slotGetVideoFileList(const QString& from_std_macAddr, const QString& from_std_list, RemotePrinerType remoteType);

private:
	int m_printerType;
	int m_bFirstConnect;
	QRegExp m_timeRegExp;
	QRegExp m_webInfoRegExp;
	QList<VideoModelItem*> m_videoItemList;
	QMap<std::string, std::string> m_videoInfoMap;
	QMap<QString, QList<VideoModelItem*>> m_videoItemMap;

	int getItemRowIndex(const QString& fileName, const QList<VideoModelItem*>& itemList);
};

class VideoModelItem : public QObject
{
	Q_OBJECT

public:
	enum GCodeFieldType
	{
		E_GCodeFileName,
		E_VideoFileSize,
		E_VideoFileName,
		E_StartTime,
		E_PrintTime,
		E_VideoCover,
		E_ExportProgress,
		E_VideoFileShowName,
		E_VideoFilePath,
	};

	Q_PROPERTY(QString gcodeFileName	    READ gcodeFileName		  WRITE setGcodeFileName		NOTIFY gcodeFileNameChanged)
	Q_PROPERTY(QString videoFileName	    READ videoFileName		  WRITE setVideoFileName		NOTIFY videoFileNameChanged)
	Q_PROPERTY(QString videoFileSize	    READ videoFileSize		  WRITE setVideoFileSize		NOTIFY videoFileSizeChanged)
	Q_PROPERTY(QString startTime	    READ startTime		  WRITE setStartTime		NOTIFY startTimeChanged)
	Q_PROPERTY(QString printTime	    READ printTime		  WRITE setPrintTime		NOTIFY printTimeChanged)
	Q_PROPERTY(QString videoCover	    READ videoCover		  WRITE setVideoCover		NOTIFY videoCoverChanged)
	Q_PROPERTY(float   exportProgress  READ exportProgress  WRITE setExportProgress  NOTIFY exportProgressChanged)
	Q_PROPERTY(QString videoFileShowName	    READ videoFileShowName		  WRITE setVideoFileShowName		NOTIFY videoFileShowNameChanged)
	Q_PROPERTY(QString videoFilPath	    READ videoFilePath		  WRITE setVideoFilePath		NOTIFY videoFilePathChanged)
	explicit VideoModelItem(QObject* parent = nullptr);
	VideoModelItem(const VideoModelItem& VideoModelItem);
	~VideoModelItem();

	const QString& gcodeFileName() const;
	void setGcodeFileName(const QString& fileName);

	const QString& videoFileSize() const;
	void setVideoFileSize(const QString& fileSize);

	const QString& videoFileName() const;
	void setVideoFileName(const QString& fileName);

	const QString& videoFilePath() const;
	void setVideoFilePath(const QString& path);

	const QString& startTime() const;
	void setStartTime(const QString& startTime);

	const QString& printTime() const;
	void setPrintTime(const QString& printTime);

	const QString& videoCover() const;
	void setVideoCover(const QString& cover);

	float exportProgress() const;
	void setExportProgress(float progress);

	const QString& videoFileShowName() const;
	void setVideoFileShowName(const QString& fileName);

	friend class VideoListModel;

signals:
	void gcodeFileNameChanged();
	void videoFileNameChanged();
	void videoFileSizeChanged();
	void startTimeChanged();
	void printTimeChanged();
	void videoCoverChanged();
	void exportProgressChanged();
	void videoFileShowNameChanged();
	void videoFilePathChanged();

private:
	QString m_gCodeFileName;
	QString m_videoFileSize;
	QString m_videoFileName;
	QString m_videoFilePath;
	QString m_startTime;
	QString m_printTime;
	QString m_videoCover;
	float   m_exportProgress = 0.0;
	QString m_videoFileShowName;
};
Q_DECLARE_METATYPE(VideoModelItem)

#endif