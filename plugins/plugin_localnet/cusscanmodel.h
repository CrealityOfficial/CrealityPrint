#ifndef CUSSCANMODEL_H
#define CUSSCANMODEL_H

#include <QString>
#include <QAbstractListModel>
#include <QtConcurrent/QtConcurrent>

#include "localnetworkinterface/RemotePrinterManager.h"

class ScanModelItem;
class CusScanModel : public QAbstractListModel
{
	Q_OBJECT

public:
	explicit CusScanModel(QObject* parent = nullptr);
	~CusScanModel();

	Q_INVOKABLE void searchDevice();
	Q_INVOKABLE void searchDeviceList();
	Q_INVOKABLE void selectAllDevice(bool select);
	Q_INVOKABLE void addSearchDevice();
	void onGetSearchSuccess(const RemotePrinter& printer);

protected:
	int rowCount(const QModelIndex& parent = QModelIndex()) const;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	QHash<int, QByteArray> roleNames() const;

private:
	int m_bFirstSearch;
	int getItemRowIndex(const QString& macAddress);
	QList<QPair<QString, ScanModelItem*>> m_ListScanItem;

signals:
	void sigGetSearchSuccess(const RemotePrinter& printer);

private slots:
	void slotGetSearchSuccess(const RemotePrinter& printer);
};

class ScanModelItem : public QObject
{
	Q_OBJECT

public:
	enum DataType
	{
		E_HasSelect,
		E_PrinterType,
		E_PrinterID,
		E_DeviceName,
		E_PrinterModel,
		E_IpAddr,
	};

	Q_PROPERTY(int pcHasSelect READ pcHasSelect WRITE setPcHasSelect NOTIFY pcHasSelectChanged)
	Q_PROPERTY(int printerType READ printerType CONSTANT)
	Q_PROPERTY(QString pcPrinterID READ pcPrinterID CONSTANT)
	Q_PROPERTY(QString pcDeviceName READ pcDeviceName CONSTANT)
	Q_PROPERTY(QString pcPrinterModel READ pcPrinterModel CONSTANT)
	Q_PROPERTY(QString pcIpAddr READ pcIpAddr CONSTANT)

	explicit ScanModelItem(QObject* parent = nullptr);
	ScanModelItem(const ScanModelItem& scanModelItem);
	~ScanModelItem();

	int pcHasSelect() const;
	void setPcHasSelect(int hasSelect);

	int printerType() const;

	int moonrakerPort() const;

	const QString& pcPrinterID() const;

	const QString& pcDeviceName() const;

	const QString& pcPrinterModel() const;

	const QString& pcIpAddr() const;

	friend class CusScanModel;

signals:
	void pcHasSelectChanged();

private:
	int m_pcHasSelect;
	int m_printerType;
	int m_moonrakerPort;
	QString m_pcPrinterID;
	QString m_pcDeviceName;
	QString m_pcPrinterModel;
	QString m_pcIpAddr;
};
Q_DECLARE_METATYPE(ScanModelItem)

#endif
