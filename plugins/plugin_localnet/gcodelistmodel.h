#ifndef GCODELISTMODEL_H
#define GCODELISTMODEL_H

#include <QAbstractListModel>
#include "localnetworkinterface/RemotePrinterManager.h"

class GcodeModelItem;
class GcodeListModel: public QAbstractListModel
{
	Q_OBJECT

public:
	Q_PROPERTY(int printerType READ printerType NOTIFY printerTypeChanged)
	explicit GcodeListModel(QObject* parent = nullptr);
	~GcodeListModel();

	Q_INVOKABLE void findGcodeFileList();
	Q_INVOKABLE void deleteSelectedGcodeFile(const QString& ipAddress);
	Q_INVOKABLE void downloadSelectedGcodeFile(const QString& macAddr,const QString& ipAddress,const QString& localPath);
	Q_INVOKABLE void setGcodeFileSelected(bool select);
	//Q_INVOKABLE int gcodeFileSelectedCount();
	Q_INVOKABLE bool isGcodeFileExist(const QString& macAddr, const QString& fileName);
	Q_INVOKABLE void deleteGcodeFile(const QString& ipAddress, const QString& filePath, int remoteType);
	Q_INVOKABLE void renameGcodeFile(const QString& ipAddress, const QString& filePath, const QString& targetName, int remoteType);
	Q_INVOKABLE void switchDataSource(const QString& macAddr, RemotePrinerType remoteType, bool init = true);
	Q_INVOKABLE void importMultiFile(const QString& macAddr, const QString& ipAddress, const QList<QUrl>& fileUrls);
	Q_INVOKABLE void downloadGcodeFile(const QString& macAddr, const QString& ipAddress, const QString& fileName, const QString& localPath, const int& remoteType);
	Q_INVOKABLE void setCurrentMacAddr(QString macAddr) { m_currentMacAddr = macAddr;}
	void onGetExportProgress(const QString& ipAddress, const QString& fileName, const float& progress);

	int printerType() const;
	void onGetGcodeFileList(const std::string& std_macAddr, const std::string& std_list, RemotePrinerType remoteType);

	Q_PROPERTY(int gcodeFileSelectedCount READ gcodeFileSelectedCount NOTIFY gcodeFileSelectedCountChanged);

	int gcodeFileSelectedCount();


protected:
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;

signals:
	void gcodeFileSelectedCountChanged();
	void printerTypeChanged();
	void sigGetGcodeFileList(const QString& from_std_macAddr, const QString& from_std_list, RemotePrinerType remoteType);

private slots:
	void slotGetGcodeFileList(const QString& from_std_macAddr, const QString& from_std_list, RemotePrinerType remoteType);

private:
	int m_printerType;
	int m_bFirstConnect;
	int m_gcodeItemSelectedCount = 0;
	QRegExp m_timeRegExp;
	QRegExp m_webInfoRegExp;
	QRegExp m_boxInfoRegExp;
	QList<GcodeModelItem*> m_GcodeItemList;
	QMap<std::string, std::string> m_GcodeInfoMap;
	QMap<QString, QList<GcodeModelItem*>> m_GcodeItemMap;
	QString m_currentMacAddr;

	void ImportMultiFileProgress(const std::string& std_macAddr, const std::string& std_fileName, const float& progress);
	void ImportMultiFileMessage(const std::string& std_macAddr, const std::string& std_fileName, int errorCode);

	int getItemRowIndex(const QString& fileName, const QList<GcodeModelItem*>& itemList);
};

class GcodeModelItem : public QObject
{
	Q_OBJECT

public:
	enum GCodeFieldType
	{
		E_GCodeFileName,
		E_GCodePrefixPath,
		E_GCodeFileSize,
		E_GCodeFileTime,
		E_GCodeLayerHeight,
		E_GCodeMaterialLength,
		E_GCodeThumbnailImage,
		E_GCodeImportProgress,
		E_GCodeExportProgress,
		E_GCodeSelected,
		E_GCodeIsMultiColor,
		E_FileType,
		E_GCodeFileNames
	};

	Q_PROPERTY(QString gCodeFileName	    READ gCodeFileName		  WRITE setGCodeFileName		NOTIFY gCodeFileNameChanged)
	Q_PROPERTY(QString gCodePrefixPath	    READ gCodePrefixPath		  WRITE setGCodePrefixPath		NOTIFY gCodePrefixPathChanged)
	Q_PROPERTY(QString gCodeFileSize	    READ gCodeFileSize		  WRITE setGCodeFileSize		NOTIFY gCodeFileSizeChanged)
	Q_PROPERTY(QString gCodeFileTime	    READ gCodeFileTime		  WRITE setGCodeFileTime		NOTIFY gCodeFileTimeChanged)
	Q_PROPERTY(QString gCodeLayerHeight	    READ gCodeLayerHeight     WRITE setGCodeLayerHeight     NOTIFY gCodeLayerHeightChanged)
	Q_PROPERTY(QString gCodeMaterialLength  READ gCodeMaterialLength  WRITE setGCodeMaterialLength  NOTIFY gCodeMaterialLengthChanged)
	Q_PROPERTY(QString gCodeThumbnailImage  READ gCodeThumbnailImage  WRITE setGCodeThumbnailImage  NOTIFY gCodeThumbnailImageChanged)
	Q_PROPERTY(float   gCodeImportProgress  READ gCodeImportProgress  WRITE setGCodeImportProgress  NOTIFY gCodeImportProgressChanged)
	Q_PROPERTY(float   gCodeExportProgress  READ gCodeExportProgress  WRITE setGCodeExportProgress  NOTIFY gCodeExportProgressChanged)
	Q_PROPERTY(bool gCodeSelected	    READ gCodeSelected		  WRITE setGCodeSelected		NOTIFY gCodeSelectedChanged)
	Q_PROPERTY(QString gCodeIsMultiColor	    READ gCodeIsMultiColor		  WRITE setGCodeIsMultiColor		NOTIFY gCodeIsMultiColorChanged)

	explicit GcodeModelItem(QObject* parent = nullptr);
	GcodeModelItem(const GcodeModelItem& GcodeModelItem);
	~GcodeModelItem();

	const QString& gCodeFileName() const;
	void setGCodeFileName(const QString& fileName);

	const QString& gCodePrefixPath() const;
	void setGCodePrefixPath(const QString& path);

	const QString& gCodeFileSize() const;
	void setGCodeFileSize(const QString& fileSize);

	const QString& gCodeFileTime() const;
	void setGCodeFileTime(const QString& fileTime);

	const QString& gCodeLayerHeight() const;
	void setGCodeLayerHeight(const QString& height);

	const QString& gCodeMaterialLength() const;
	void setGCodeMaterialLength(const QString& length);

	const QString& gCodeThumbnailImage() const;
	void setGCodeThumbnailImage(const QString& image);

	float gCodeImportProgress() const;
	void setGCodeImportProgress(float progress);

	float gCodeExportProgress() const;
	void setGCodeExportProgress(float progress);

	bool gCodeSelected() const;
	void setGCodeSelected(bool selected);

	QString gCodeIsMultiColor() const;
	void setGCodeIsMultiColor(const QString& isMulti);

	friend class GcodeListModel;

signals:
	void gCodeFileNameChanged();
	void gCodePrefixPathChanged();
	void gCodeFileSizeChanged();
	void gCodeFileTimeChanged();
	void gCodeLayerHeightChanged();
	void gCodeMaterialLengthChanged();
	void gCodeThumbnailImageChanged();
	void gCodeImportProgressChanged();
	void gCodeExportProgressChanged();
	void gCodeSelectedChanged();
	void gCodeIsMultiColorChanged();
	

private:
	QString m_gCodeFileName;
	QString m_gCodePrefixPath;
	QString m_gCodeFileSize;
	QString m_gCodeFileTime;
	QString m_gCodeLayerHeight;
	QString m_gCodeMaterialLength;
	QString m_gCodeThumbnailImage;
	float   m_gCodeImportProgress;
	float   m_gCodeExportProgress;
	bool 	m_gCodeSelected;
	QString m_gCodeIsMultiColor;
};
Q_DECLARE_METATYPE(GcodeModelItem)

#endif