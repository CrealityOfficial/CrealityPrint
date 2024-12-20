#ifndef MATERIALBOXMODELLIST_H
#define MATERIALBOXMODELLIST_H

#include <memory>

#include <QAbstractListModel>
#include <QColor>

#include "basickernelexport.h"
#include "localnetworkinterface/RemotePrinterManager.h"
#include "localnetworkinterface/autorefillinfolistmodel.h"

class MaterialBoxModelItem;
class BASIC_KERNEL_API MaterialBoxListModel :public QAbstractListModel
{
	Q_OBJECT
public:
	Q_PROPERTY(int printerType READ printerType NOTIFY printerTypeChanged)
	explicit MaterialBoxListModel(QObject* parent = nullptr);
	~MaterialBoxListModel();








	struct ColorMap {
		std::string type;
		std::string color;
		int boxId;
		int materialId;
	};
	struct ColorMapInfo {
		std::vector<ColorMap> ColorMapList;
	};

	enum {
		E_AutoRefill,
		E_CAutoFeed,
		E_CSelfTest,
		E_CAutoUpdateFilament,
	};




	Q_INVOKABLE void findMaterialBoxList();
	Q_INVOKABLE void switchDataSource(const QString& macAddress, RemotePrinerType remoteType, bool init = true);
    Q_INVOKABLE QString getMaterialBoxId(const QString& macAddress, int index);
	Q_INVOKABLE void getMaterialSelected(int boxId, int materialId);
	Q_INVOKABLE QString getMaterialType(const QString& macAddress, int index);
	Q_INVOKABLE QString getMaterialName(const QString& macAddress, int index);
	Q_INVOKABLE QString getMaterialId(const QString& macAddress, int index);
	Q_INVOKABLE QString getMaterialState(const QString& macAddress, int index);
	Q_INVOKABLE QColor getMaterialColor(const QString& macAddress, int index);
	Q_INVOKABLE QList<QColor> getMaterialColorList(const QString& macAddress);
	Q_INVOKABLE QString getRfid(const QString& macAddress, int index);
	Q_INVOKABLE void materialColorMap(const QString& ip, QString path,const QList<QVariantMap>& colorMap);
	Q_INVOKABLE void feedInOrOutMaterial(const QString& ip, const QString& boxId, const QString& materialId, const QString& isFeed);
	Q_INVOKABLE void refreshBox(const QString& ip, const QString& boxId, const QString& materialId);
	Q_INVOKABLE void toMultiColorGuide();
	Q_INVOKABLE void modifyMaterial(const QString& ip, const QVariantMap& materialParams);
	Q_INVOKABLE bool isRemoteModifyMaterial(const QString& ip, const QVariantMap& materialParams);
	Q_INVOKABLE void boxConfig(const QString& ip, const QString& autoRefill, const QString& cAutoFeed, const QString& cSelfTest);
	Q_INVOKABLE void getColorMap(const QString& ip, const QString& filePath);
	Q_INVOKABLE bool isOnlyRackMaterial(const QString& ip);
	Q_INVOKABLE QString getCloseColorFromPrinter(const QString& hexColor, const QStringList hexColorList);
	Q_INVOKABLE int getCloseColorFromPrinter(const QString& hexColor, const QString& type);
	Q_INVOKABLE void GenCloseGCodeColors(const QStringList colors, const QStringList types,QStringList usedIndexs);
	Q_INVOKABLE void GenCloseGCodeColors2(const QString colors, const QString types);
	Q_INVOKABLE int getCloseColorByIndex(int colorIndex);
	Q_INVOKABLE int getCurrentMaterialState();
	int printerType() const;
	void onGetMaterialBoxList(const std::string& std_macAddr, const BoxsInfo& std_list, RemotePrinerType remoteType);
	void onGetMaterialState(const QString& macAddr,  MaterialState state, RemotePrinerType remoteType);
	void onGetBoxConfig(const std::string& std_macAddr,  BoxConfig state, RemotePrinerType remoteType);
	void onGetColorMap(const std::string& std_macAddr, ColorMapInfo state, RemotePrinerType remoteType);
	void onGetGCodeColors(const std::string& std_macAddr, ColorMapInfo state, RemotePrinerType remoteType);

	Q_PROPERTY(QString autoRefill		READ autoRefill		WRITE setAutoRefill		NOTIFY autoRefillChanged)
	Q_PROPERTY(QString cAutoFeed		READ cAutoFeed		WRITE setCAutoFeed		NOTIFY cAutoFeedChanged)
	Q_PROPERTY(QString cSelfTest		READ cSelfTest		WRITE setCSelfTest		NOTIFY cSelfTestChanged)
	Q_PROPERTY(QString humidity		READ humidity		WRITE setHumidity		NOTIFY humidityChanged)


	// �ϼܵĺĲ���Ϣ
	Q_PROPERTY(bool rack			READ rack		WRITE setRack		NOTIFY rackChanged)
	Q_PROPERTY(QString rfid			READ rfid		WRITE setRfid		NOTIFY rfidChanged)
	Q_PROPERTY(QString vendor		READ vendor		WRITE setVendor		NOTIFY vendorChanged)
	Q_PROPERTY(QString type			READ type		WRITE setType		NOTIFY typeChanged)
	Q_PROPERTY(QColor color		READ color		WRITE setColor		NOTIFY colorChanged)
	Q_PROPERTY(QString name			READ name		WRITE setName		NOTIFY nameChanged)
	Q_PROPERTY(QString minTemp		READ minTemp		WRITE setMinTemp		NOTIFY minTempChanged)
	Q_PROPERTY(QString maxTemp		READ maxTemp		WRITE setMaxTemp		NOTIFY maxTempChanged)
	Q_PROPERTY(QString editStatus		READ editStatus		WRITE setEditStatus		NOTIFY editStatusChanged)
	Q_PROPERTY(bool used			READ used		WRITE setUsed		NOTIFY usedChanged)
	Q_PROPERTY(QObject* autoRefillListModel READ getAutoRefillListModel CONSTANT)
	Q_PROPERTY(int materialState	READ getCurrentMaterialState  NOTIFY materialSelected)
	
	QString autoRefill() const;
	void setAutoRefill(QString autoRefill);

	QString cAutoFeed() const;
	void setCAutoFeed(QString cAutoFeed);

	QString cSelfTest() const;
	void setCSelfTest(QString cSelfTest);

	QString humidity() const;
	void setHumidity(QString humidity);
	QString editStatus() const;
	void setEditStatus(QString editStatus);

	bool rack() const;
	void setRack(bool rack);
	QString vendor() const;
	void setVendor(QString vendor);
	QString rfid() const;
	void setRfid(QString rfid);
	bool used() const;
	void setUsed(bool used);
	QString type() const;
	void setType(QString type);
	QColor color() const;
	void setColor(QColor color);
	QString name() const;
	void setName(QString name);
	QString minTemp() const;
	void setMinTemp(QString minTemp);
	QString maxTemp() const;
	void setMaxTemp(QString maxTemp);
	void initRack();

	QObject* getAutoRefillListModel();


	/*QString cAutoUpdateFilament() const;
	void setCAutoUpdateFilament(QString cAutoUpdateFilament);*/
protected:
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;
signals:
	void printerTypeChanged();
	void sigGetMaterialBoxList(const QString& from_std_macAddr, const QString& from_std_list, RemotePrinerType remoteType);
	void autoRefillChanged();
	void cAutoFeedChanged();
	void cSelfTestChanged();
	void vendorChanged();
	void typeChanged();
	void colorChanged();
	void nameChanged();
	void minTempChanged();
	void maxTempChanged();
	void rackChanged();
	void humidityChanged();
	void editStatusChanged();
	void rfidChanged();
	void usedChanged();
	void materialSelected();
private slots:
	void slotGetMaterialBoxList(const QString& from_std_macAddr, const QString& from_std_list, RemotePrinerType remoteType);

private:
	int m_printerType;
	int m_bFirstConnect;
	QRegExp m_webInfoRegExp;
	QRegExp m_boxInfoRegExp;
	QList<MaterialBoxModelItem*> m_materialBoxItemList;
	QMap<std::string, BoxsInfo> m_materialBoxMap;
	QMap<QString, QList<MaterialBoxModelItem*>> m_MaterialBoxItemMap;
	QString m_autoRefill;
	QString m_cAutoFeed;
	QString m_cSelfTest;
	QString m_humidity = "60";
	QString m_editStatus = "";
	bool m_rack;
	QString m_cAutoUpdateFilament;
	QString m_vendor;
	QString m_type;
	QString m_rfid;
	QColor m_color;
	QString m_name;
	QString m_minTemp;
	QString m_maxTemp;
	QString m_sameMaterial;
	bool m_used;
	long long m_boxUpdateTime;
	long long m_boxConfigUpdateTime;
	long long m_boxMaterialUpdateTime;
	std::unique_ptr<creative_kernel::AutoRefillInfoListModel> m_autoRefillInfoListModel = nullptr;
	QMap<int,int> m_closestIndexs; 
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
		E_Name,
		E_Vendor,
		E_MinTemp,
		E_MaxTemp,
		E_Pressure,
		E_Selected,
		E_State,
		E_Percent,
		E_Used,
		E_MapColor,
		E_MapType,
		E_EditStatus
	};
	Q_PROPERTY(QString materialBoxId		READ materialBoxId		WRITE setMaterialBoxId		NOTIFY materialBoxIdChanged)
	Q_PROPERTY(QString materialBoxState		READ materialBoxState		WRITE setMaterialBoxState		NOTIFY materialBoxStateChanged)
	Q_PROPERTY(QString materialBoxFilament		READ materialBoxFilament		WRITE setMaterialBoxFilament		NOTIFY materialBoxFilamentChanged)
	Q_PROPERTY(QString materialId		READ materialId		WRITE setMaterialId		NOTIFY materialIdChanged)
	Q_PROPERTY(QColor materialColor	READ materialColor		WRITE setMaterialColor		NOTIFY materialColorChanged)
	Q_PROPERTY(QString materialType		READ materialType		WRITE setMaterialType		NOTIFY materialTypeChanged)
	Q_PROPERTY(QString rfid		READ rfid		WRITE setRfid		NOTIFY rfidChanged)
	Q_PROPERTY(QString name		READ name		WRITE setName		NOTIFY nameChanged)
	Q_PROPERTY(QString vendor		READ vendor		WRITE setVendor		NOTIFY vendorChanged)
	Q_PROPERTY(QString minTemp		READ minTemp		WRITE setMinTemp		NOTIFY minTempChanged)
	Q_PROPERTY(QString maxTemp		READ maxTemp		WRITE setMaxTemp	NOTIFY maxTempChanged)
	Q_PROPERTY(QString pressure		READ pressure		WRITE setPressure		NOTIFY pressureChanged)
	Q_PROPERTY(bool selected		READ selected		WRITE setSelected		NOTIFY selectedChanged)
	Q_PROPERTY(QString state		READ state		WRITE setState		NOTIFY stateChanged)
	Q_PROPERTY(QString percent		READ percent		WRITE setPercent		NOTIFY percentChanged)
	Q_PROPERTY(bool used		READ used		WRITE setUsed		NOTIFY usedChanged)
	Q_PROPERTY(QColor mapColor		READ mapColor		WRITE setMapColor		NOTIFY mapColorChanged)
	Q_PROPERTY(QString mapType		READ mapType		WRITE setMapType		NOTIFY mapTypeChanged)
	Q_PROPERTY(QString editStatus		READ editStatus		WRITE setEditStatus		NOTIFY editStatusChanged)

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

	QColor materialColor() const;
	void setMaterialColor(const QColor& color);

	QString materialType() const;
	void setMaterialType(const QString& type);

	const QString rfid() const;
	void setRfid(const QString& id);

	const QString name() const;
	void setName(const QString& name);

	const QString vendor() const;
	void setVendor(const QString& vendor);

	const QString minTemp() const;
	void setMinTemp(const QString& minTemp);

	const QString maxTemp() const;
	void setMaxTemp(const QString& maxTemp);

	const QString pressure() const;
	void setPressure(const QString& pressure);

	bool selected() const;
	void setSelected(bool select);

	const QString state() const;
	void setState(const QString& state);

	const QString percent() const;
	void setPercent(const QString& percent);

	bool used() const;
	void setUsed(bool used);

	const QColor mapColor() const;
	void setMapColor(const QColor& color);

	const QString mapType() const;
	void setMapType(const QString& type);

	const QString editStatus() const;
	void setEditStatus(const QString& status);

	//Q_INVOKABLE QString setMaterialSelected(int boxId, int materialId);


signals:
	void materialBoxIdChanged();
	void materialBoxStateChanged();
	void materialBoxFilamentChanged();
	void materialIdChanged();
	void materialColorChanged();
	void materialTypeChanged();
	void rfidChanged();
	void pressureChanged();
	void nameChanged();
	void vendorChanged();
	void minTempChanged();
	void maxTempChanged();
	void selectedChanged();
	void stateChanged();
	void percentChanged();
	void usedChanged();
	void mapColorChanged();
	void mapTypeChanged();
	void editStatusChanged();

private:
	QString m_materialBoxId;
	QString m_materialBoxState;
	QString m_materialBoxFilament;
	QString m_materialId;
	QColor m_materialColor;
	QString m_materialType;
	QString m_rfid;
	QString m_name;
	QString m_vendor;
	QString m_minTemp;
	QString m_maxTemp;
	QString m_pressure;
	bool m_selected = false;
	QString m_state;
	QString m_percent;
	QColor m_mapColor;
	QString m_mapType;
	bool m_used = false;
	QString m_editStatus;
};
Q_DECLARE_METATYPE(MaterialBoxModelItem)
#endif // !MATERIALBOXMODELLIST_H
