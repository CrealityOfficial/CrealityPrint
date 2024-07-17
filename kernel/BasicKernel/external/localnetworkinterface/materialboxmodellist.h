#ifndef MATERIALBOXMODELLIST_H
#define MATERIALBOXMODELLIST_H

#include <QAbstractListModel>
#include "basickernelexport.h"
#include "localnetworkinterface/RemotePrinterManager.h"

class MaterialBoxModelItem;
class BASIC_KERNEL_API MaterialBoxListModel :public QAbstractListModel
{
	Q_OBJECT
public:
	Q_PROPERTY(int printerType READ printerType NOTIFY printerTypeChanged)
	explicit MaterialBoxListModel(QObject* parent = nullptr);
	~MaterialBoxListModel();




	Q_INVOKABLE void findMaterialBoxList();
	Q_INVOKABLE void switchDataSource(const QString& macAddress, RemotePrinerType remoteType, bool init = true);
    Q_INVOKABLE QString getMaterialBoxId(const QString& macAddress, int index);
	Q_INVOKABLE QString getMaterialType(const QString& macAddress, int index);
	Q_INVOKABLE QString getMaterialId(const QString& macAddress, int index);
	Q_INVOKABLE QString getMaterialColor(const QString& macAddress, int index);
	Q_INVOKABLE QStringList getMaterialColorList(const QString& macAddress);
	Q_INVOKABLE QString getRfid(const QString& macAddress, int index);
	Q_INVOKABLE void materialColorMap(const QString& ip, QString path,const QList<QVariantMap>& colorMap);

	Q_INVOKABLE QString getCloseColorFromPrinter(const QString& hexColor, const QStringList hexColorList);
	Q_INVOKABLE int getCloseColorFromPrinter(const QString& hexColor);
	int printerType() const;
	void onGetMaterialBoxList(const std::string& std_macAddr, const std::string& std_list, RemotePrinerType remoteType);
protected:
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;
signals:
	void printerTypeChanged();
	void sigGetMaterialBoxList(const QString& from_std_macAddr, const QString& from_std_list, RemotePrinerType remoteType);
private slots:
	void slotGetMaterialBoxList(const QString& from_std_macAddr, const QString& from_std_list, RemotePrinerType remoteType);

private:
	int m_printerType;
	int m_bFirstConnect;
	QRegExp m_webInfoRegExp;
	QRegExp m_boxInfoRegExp;
	QList<MaterialBoxModelItem*> m_materialBoxItemList;
	QMap<std::string, std::string> m_materialBoxMap;
	QMap<QString, QList<MaterialBoxModelItem*>> m_MaterialBoxItemMap;
};
class MaterialBoxModelItem :public QObject
{
	Q_OBJECT
public:
	enum {
		E_MaterialBoxId,
		E_MaterialBoxState,
		E_MaterialBoxFilament,
		E_MaterialId,
		E_MaterialColor,
		E_MaterialType,
		E_Rfid,
	};
	Q_PROPERTY(QString materialBoxId		READ materialBoxId		WRITE setMaterialBoxId		NOTIFY materialBoxIdChanged)
	Q_PROPERTY(QString materialBoxState		READ materialBoxState		WRITE setMaterialBoxState		NOTIFY materialBoxStateChanged)
	Q_PROPERTY(QString materialBoxFilament		READ materialBoxFilament		WRITE setMaterialBoxFilament		NOTIFY materialBoxFilamentChanged)
	Q_PROPERTY(QString materialId		READ materialId		WRITE setMaterialId		NOTIFY materialIdChanged)
	Q_PROPERTY(QString materialColor	READ materialColor		WRITE setMaterialColor		NOTIFY materialColorChanged)
	Q_PROPERTY(QString materialType		READ materialType		WRITE setMaterialType		NOTIFY materialTypeChanged)
	Q_PROPERTY(QString rfid		READ rfid		WRITE setRfid		NOTIFY rfidChanged)

	explicit MaterialBoxModelItem(QObject* parent = nullptr);
	MaterialBoxModelItem(const MaterialBoxModelItem& MaterialBoxModelItem);
	~MaterialBoxModelItem();

	const QString materialBoxId() const;
	void setMaterialBoxId(const QString& materialBox);

	const QString materialBoxState() const;
	void setMaterialBoxState(const QString& materialBoxState);

	const QString materialBoxFilament() const;
	void setMaterialBoxFilament(const QString& materialBoxFilament);

	const QString materialId() const;
	void setMaterialId(const QString& rfid);

	const QString materialColor() const;
	void setMaterialColor(const QString& color);

	const QString materialType() const;
	void setMaterialType(const QString& type);

	const QString rfid() const;
	void setRfid(const QString& id);
signals:
	void materialBoxIdChanged();
	void materialBoxStateChanged();
	void materialBoxFilamentChanged();
	void materialIdChanged();
	void materialColorChanged();
	void materialTypeChanged();
	void rfidChanged();
private:
	QString m_materialBoxId;
	QString m_materialBoxState;
	QString m_materialBoxFilament;
	QString m_materialId;
	QString m_materialColor;
	QString m_materialType;
	QString m_rfid;

};
Q_DECLARE_METATYPE(MaterialBoxModelItem)
#endif // !MATERIALBOXMODELLIST_H
