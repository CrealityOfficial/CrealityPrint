#ifndef HISTORYLISTMODEL_H
#define HISTORYLISTMODEL_H

#include <QAbstractListModel>
#include "localnetworkinterface/RemotePrinterManager.h"

class HistoryModelItem;
class HistoryListModel: public QAbstractListModel
{
	Q_OBJECT

public:
	Q_PROPERTY(int printerType READ printerType NOTIFY printerTypeChanged)
	explicit HistoryListModel(QObject* parent = nullptr);
	~HistoryListModel();

	Q_INVOKABLE void findHistoryFileList();
	Q_INVOKABLE void deleteHistoryFile(const QString& ipAddress, const QString& id, int remoteType);
	Q_INVOKABLE void switchDataSource(const QString& macAddress, RemotePrinerType remoteType, bool init = true);
	Q_INVOKABLE void downloadGcodeFile(const QString& macAddress, const QString& ipAddress, const QString& fileName, const QString& localPath, const int& remoteType, const QString& historyFileNumb);

	int printerType() const;
	void onGetHistoryFileList(const std::string& std_macAddr, const std::string& std_list, RemotePrinerType remoteType);
	void onGetExportProgress(const QString& ipAddress, const QString& fileName, const float& progress);
	int getItemRowIndex(const QString& fileName, const QList<HistoryModelItem*>& itemList);

protected:
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;

signals:
	void printerTypeChanged();
	void sigGetHistoryFileList(const QString& from_std_macAddr, const QString& from_std_list, RemotePrinerType remoteType);

private slots:
	void slotGetHistoryFileList(const QString& from_std_macAddr, const QString& from_std_list, RemotePrinerType remoteType);

private:
	int m_printerType;
	int m_bFirstConnect;
	QRegExp m_webInfoRegExp;
	QList<HistoryModelItem*> m_HistoryItemList;
	QMap<std::string, std::string> m_HistoryInfoMap;
	QMap<QString, QList<HistoryModelItem*>> m_HistoryItemMap;
};

class HistoryModelItem : public QObject
{
	Q_OBJECT

public:
	enum HistoryFieldType
	{
		E_IsPrintFinished,
		E_HistoryFileNumb,
		E_HistoryFileName,
		E_HistoryFileSize,
		E_HistoryFileTime,
		E_HistoryInterval,
		E_HistoryMaterialUsage,
		E_HistoryThumbnailImage,
		E_GCodeExportProgress,
		E_GCodePrefixPath,
	};

	Q_PROPERTY(int	   isPrintFinished		  READ isPrintFinished		  WRITE setIsPrintFinished		  NOTIFY isPrintFinishedChanged)
	Q_PROPERTY(QString gCodePrefixPath	    READ gCodePrefixPath		  WRITE setGCodePrefixPath		NOTIFY gCodePrefixPathChanged)
	Q_PROPERTY(QString historyFileNumb	      READ historyFileNumb		  WRITE setHistoryFileNumb		  NOTIFY historyFileNumbChanged)
	Q_PROPERTY(QString historyFileName	      READ historyFileName		  WRITE setHistoryFileName		  NOTIFY historyFileNameChanged)
	Q_PROPERTY(QString historyFileSize	      READ historyFileSize		  WRITE setHistoryFileSize		  NOTIFY historyFileSizeChanged)
	Q_PROPERTY(QString historyFileTime	      READ historyFileTime		  WRITE setHistoryFileTime		  NOTIFY historyFileTimeChanged)
	Q_PROPERTY(QString historyInterval	      READ historyInterval        WRITE setHistoryInterval		  NOTIFY historyIntervalChanged)
	Q_PROPERTY(QString historyMaterialUsage   READ historyMaterialUsage   WRITE setHistoryMaterialUsage	  NOTIFY historyMaterialUsageChanged)
	Q_PROPERTY(QString historyThumbnailImage  READ historyThumbnailImage  WRITE setHistoryThumbnailImage  NOTIFY historyThumbnailImageChanged)
	Q_PROPERTY(float   gCodeExportProgress  READ gCodeExportProgress  WRITE setGCodeExportProgress  NOTIFY gCodeExportProgressChanged)

	explicit HistoryModelItem(QObject* parent = nullptr);
	HistoryModelItem(const HistoryModelItem& HistoryModelItem);
	~HistoryModelItem();

	int isPrintFinished() const;
	void setIsPrintFinished(int finished);

	const QString& gCodePrefixPath() const;
	void setGCodePrefixPath(const QString& path);

	const QString& historyFileNumb() const;
	void setHistoryFileNumb(const QString& fileNumb);

	const QString& historyFileName() const;
	void setHistoryFileName(const QString& fileName);

	const QString& historyFileSize() const;
	void setHistoryFileSize(const QString& fileSize);

	const QString& historyFileTime() const;
	void setHistoryFileTime(const QString& fileTime);

	const QString& historyInterval() const;
	void setHistoryInterval(const QString& interval);

	const QString& historyMaterialUsage() const;
	void setHistoryMaterialUsage(const QString& usage);

	const QString& historyThumbnailImage() const;
	void setHistoryThumbnailImage(const QString& image);

	float gCodeExportProgress() const;
	void setGCodeExportProgress(float progress);

	friend class HistoryListModel;

signals:
	void isPrintFinishedChanged();
	void gCodePrefixPathChanged();
	void historyFileNumbChanged();
	void historyFileNameChanged();
	void historyFileSizeChanged();
	void historyFileTimeChanged();
	void historyIntervalChanged();
	void historyMaterialUsageChanged();
	void historyThumbnailImageChanged();
	void gCodeExportProgressChanged();

private:
	int m_isPrintFinished;
	QString m_gCodePrefixPath;
	QString m_historyFileNumb;
	QString m_historyFileName;
	QString m_historyFileSize;
	QString m_historyFileTime;
	QString m_historyInterval;
	QString m_historyMaterialUsage;
	QString m_historyThumbnailImage;
	float   m_gCodeExportProgress = 0.0;
};
Q_DECLARE_METATYPE(HistoryModelItem)

#endif