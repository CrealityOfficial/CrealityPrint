#ifndef CUSLISTMODEL_H
#define CUSLISTMODEL_H

#include <QMap>
#include <QTime>
#include <QPair>
#include <QTimer>
#include <QThread>
#include <QObject>
#include <QString>
#include <QSettings>
#include <QMetaEnum>
#include <QDateTime>
#include <QClipboard>
#include <QJsonArray>
#include <QJsonObject>
#include <QMapIterator>
#include <QMutexLocker>
#include <QGuiApplication>
#include <QAbstractListModel>
#include <QProcessEnvironment>
#include <QtConcurrent/QtConcurrent>

#include "localnetworkinterface/RemotePrinterManager.h"
#include "imgprovider.h"

constexpr auto JSON_VERSION = 100;

struct DeviceInfo
{
	int m_group;
	int m_connectType;
	int m_moonrakerPort;
	QString m_ipAddress;
	QString m_modelName;
	QString m_deviceName;

	DeviceInfo()
	{
		m_group = 1; m_connectType = 0; m_moonrakerPort = 0;
		m_ipAddress = QString(); m_modelName = QString(); m_deviceName = QString();
	}

	DeviceInfo(int group, int type, int port, const QString& ipAddress, const QString& modelName, const QString& deviceName)
	{
		m_group = group; m_connectType = type; m_moonrakerPort = port;
		m_ipAddress = ipAddress; m_modelName = modelName; m_deviceName = deviceName;
	}
};

class ListModelItem;
class CusListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit CusListModel(QObject *parent = nullptr);
	virtual ~CusListModel();
	
	void initialize();
	void uninitialize();

	void onGetInfoSuccess(const RemotePrinter& stateInfo);
	void onGetPreviewImage(const std::string& std_macAddr, const std::string& std_imgType, const std::string& std_imgData);

    Q_INVOKABLE void findDeviceList();
	//删除设备
	Q_INVOKABLE void deleteConnect(const QString& deviceMacAddr);
	//添加设备
	Q_INVOKABLE void createConnect(const QString& deviceIpAddr, const int& type);
	//控制命令
	Q_INVOKABLE void sendUIControlCmdToDevice(QString deviceIpAddr, int nCmdType, QString value, int printerType/* = RemotePrinerType::REMOTE_PRINTER_TYPE_LAN*/, int moonrakerPort = 0);
	//状态复位
	Q_INVOKABLE void batch_reset();
    //发送文件/一键打印
    Q_INVOKABLE void batch_command(const QString& fileName, bool sendOnly);
	//传输文件
	Q_INVOKABLE void reloadFile(const QString& macAddress);
	Q_INVOKABLE void importFile(const QString& macAddress, const QList<QUrl>& fileUrls);

    //获取分组
    Q_INVOKABLE QStringList getGroups();
	//粘贴内容
	Q_INVOKABLE QStringList pasteContent();
	//复制内容
	Q_INVOKABLE void copyContent(const QString& deviceIpAddr);
    //添加分组
    Q_INVOKABLE void addGroup(const QString& addGroupName);
	//删除分组
    Q_INVOKABLE void deleteGroup(const QString& delGroupName);
    //修改组名
    Q_INVOKABLE void editGroupName(int groupIndex, const QString& newGroupName);
	//修改设备分组
	Q_INVOKABLE void editDeviceGroup(const QString& macAddress, int newGroupNo);
	//修改设备名称
	Q_INVOKABLE void editDeviceName(const QString& macAddress, const QString& newDeviceName);

	//获取时间
	Q_INVOKABLE qint64 getCurrentUtcTime();
	Q_INVOKABLE QDateTime getCurrentDateTime(bool min);
    //获取Item
    Q_INVOKABLE ListModelItem* getDeviceItem(const QString& macAddress);

	Q_INVOKABLE bool checkOnePrint();


protected:
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int,QByteArray> roleNames() const;

protected:
	QString getErrorMsgFromCode(int errorCode);
	const QMap<int, QString> Cpr_ErrorMsg = QMap<int, QString>
	{
		{1, "Connection failure"},
		{2, "Empty reply from server"},
		{3, "Host resolution failure"},
		{4, "Internal error"},
		{5, "Invalid url format"},
		{6, "Network receive error"},
		{7, "Network send failure"},
		{8, "Operation timeout"},
		{9, "Proxy resolution failure"},
		{10, "SSL connect error"},
		{11, "SSL local certificate error"},
		{12, "SSL remote certificate error"},
		{13, "SSL CA certificate error"},
		{14, "Generic SSL error"},
		{15, "Unsupported protocol"},
		{16, "Request cancelled"},
		{17, "Too many redirects"},
		{1000, "Unknown error"}
	};

	QString getErrorMsgFromCode(int errorCode, RemotePrinerType type);
	const QMap<int, QString> Klipper4408_ErrorMsg = QMap<int, QString>
	{
		{1, "Abnormal motor drive"},
		{2, "Internal error"},
		{3, "Communication abnormality"},
		{4, "Not heating as expected"},
		{209, "The temperature of the hot bed is abnormal"},
		{6, "Abnormal extruder"},
		{7, "The coordinates of the print file are abnormal"},
		{101, "AI detection paused"},
		{100, "An unknown print error was detected"},
		{201, "Abnormal chamber temperature"},
		{9, "Abnormal vibration optimization sensor"},
		{10, "Abnormal fan"},
		{204, "Abnormal fan"},
		{205, "Network anomaly"},
		{206, "AI detects print quality issues"},
		{207, "z-Touch exception"},
		{208, "File exception"},
		{500, "Unknown exception"},
		{800, "Axis movement error"},
		{801, "Machine in preparation"},
		{2000, "Continue printing after power failure"},
		{2001, "Broken material"}
	};

signals:
	void sigConnectedIpSuccessed();
	void sigGetInfoSuccess(const RemotePrinter& stateInfo);
	void sigGetPreviewImage(const QString& from_std_macAddr, const QImage& previewImage);

private slots:
	void slotDeleteFinished();
	void slotGetInfoSuccess(const RemotePrinter& stateInfo);
	void slotGetPreviewImage(const QString& from_std_macAddr, const QImage& previewImage);

private:
	void readConfig();
	void writeConfig();
	void addLocalDevice();
	void onDataChanged(const QString& macAddress, const QVector<int>& roles = QVector<int>());

	void UploadProgressCallBack(const std::string& strMac, const std::string& fileName, const float& progress);
	void UploadMessageCallBack(const std::string& strMac, const std::string& fileName, int errorCode);//Upload

	void ImportProgressCallBack(const std::string& strMac, const std::string& fileName, const float& progress);
	void ImportMessageCallBack(const std::string& strMac, const std::string& fileName, int errorCode);//Import

	int getItemRowIndex(const QString& macAddress);

	bool m_sendOnly = false;
	bool m_bIsConnecting = false;
	ImgProvider* m_previewProvider;

	QRegExp m_posRegExp;
	QRegExp m_ipv4RegExp;
	QString m_appDataPath;
	QString m_curGcodeFileName;
	QString m_curConnectingIpAddr;
	QStringList m_deviceGroupNames;
	bool m_firstRun = true;

	QMap<QString, DeviceInfo> m_MapDeviceInfo;
	QList<QPair<QString, ListModelItem*>> m_ListPrinterItem;
};

class ListModelItem: public QObject
{
    Q_OBJECT

public:
    enum DataType 
	{
        E_PrinterID,
		E_ErrorKey,
		E_ErrorCode,
		E_FluiddPort,
		E_PrinterType,
		E_MultiNumber,
		E_MainsailPort,
		E_PrinterModel,
        E_PrinterState,
		E_PrinterStatus,
		E_PrinterMethod,
        E_HasCamera,
		E_HasSelect,
		E_EnableSelfTest,
        E_DeviceName,
        E_IpAddr,
        E_GCodeName,
        E_TotalPrintTime,
        E_LeftTime,
		E_CurPrintLayer,
		E_TotalPrintLayer,
		E_CurPrintProgress,
        E_PrintSpeed,
        E_BedDstTemp,
        E_NozzleDstTemp,
        E_BedTemp,
        E_NozzleTemp,
        E_GroupNo,
        E_X,
        E_Y,
        E_Z,
		E_XY_Autohome,
		E_Z_Autohome,
        E_FanOpened,
		E_CaseFan,
		E_AuxiliaryFan,
        E_LedOpened,
		E_TransProgress,
		E_ImportProgress,
		E_SentFileName,
		E_ErrorMessage,
		E_TransErrorMessage,
		E_Nozzle2Temp,
		E_Nozzle2DstTemp,
		E_ChamberTemp,
		E_ChamberDstTemp,
		E_SendingFileName
    };

    Q_PROPERTY(QString pcPrinterID READ pcPrinterID WRITE setPcPrinterID NOTIFY pcPrinterIDChanged)
    Q_PROPERTY(QString pcPrinterModel READ pcPrinterModel WRITE setPcPrinterModel NOTIFY pcPrinterModelChanged)
	Q_PROPERTY(int errorKey READ errorKey WRITE setErrorKey NOTIFY errorKeyChanged)
	Q_PROPERTY(int errorCode READ errorCode WRITE setErrorCode NOTIFY errorCodeChanged)
	Q_PROPERTY(int fluiddPort READ fluiddPort WRITE setFluiddPort NOTIFY fluiddPortChanged)
	Q_PROPERTY(int printerType READ printerType WRITE setPrinterType NOTIFY printerTypeChanged)
	Q_PROPERTY(int multiNumber READ multiNumber WRITE setMultiNumber NOTIFY multiNumberChanged)
	Q_PROPERTY(int mainsailPort READ mainsailPort WRITE setMainsailPort NOTIFY mainsailPortChanged)
    Q_PROPERTY(int pcPrinterState READ pcPrinterState WRITE setPcPrinterState NOTIFY pcPrinterStateChanged)
	Q_PROPERTY(int pcPrinterStatus READ pcPrinterStatus WRITE setPcPrinterStatus NOTIFY pcPrinterStatusChanged)
	Q_PROPERTY(int pcPrinterMethod READ pcPrinterMethod WRITE setPcPrinterMethod NOTIFY pcPrinterMethodChanged)
    Q_PROPERTY(int pcHasCamera READ pcHasCamera WRITE setPcHasCamera NOTIFY pcHasCameraChanged)
	Q_PROPERTY(int pcHasSelect READ pcHasSelect WRITE setPcHasSelect NOTIFY pcHasSelectChanged)
	Q_PROPERTY(int enableSelfTest READ enableSelfTest WRITE setEnableSelfTest NOTIFY enableSelfTestChanged)
    Q_PROPERTY(QString pcDeviceName READ pcDeviceName WRITE setPcDeviceName NOTIFY pcDeviceNameChanged)
    Q_PROPERTY(QString pcIpAddr READ pcIpAddr WRITE setPcIpAddr NOTIFY pcIpAddrChanged)
    Q_PROPERTY(QString pcGCodeName READ pcGCodeName WRITE setPcGCodeName NOTIFY pcGCodeNameChanged)
    Q_PROPERTY(int pcTotalPrintTime READ pcTotalPrintTime WRITE setPcTotalPrintTime NOTIFY pcTotalPrintTimeChanged)
    Q_PROPERTY(int pcLeftTime READ pcLeftTime WRITE setPcLeftTime NOTIFY pcLeftTimeChanged)
	Q_PROPERTY(int pcCurPrintLayer READ pcCurPrintLayer WRITE setPcCurPrintLayer NOTIFY pcCurPrintLayerChanged)
	Q_PROPERTY(int pcTotalPrintLayer READ pcTotalPrintLayer WRITE setPcTotalPrintLayer NOTIFY pcTotalPrintLayerChanged)
	Q_PROPERTY(int pcCurPrintProgress READ pcCurPrintProgress WRITE setPcCurPrintProgress NOTIFY pcCurPrintProgressChanged)
    Q_PROPERTY(int pcPrintSpeed READ pcPrintSpeed WRITE setPcPrintSpeed NOTIFY pcPrintSpeedChanged)
    Q_PROPERTY(int pcBedDstTemp READ pcBedDstTemp WRITE setPcBedDstTemp NOTIFY pcBedDstTempChanged)
    Q_PROPERTY(int pcNozzleDstTemp READ pcNozzleDstTemp WRITE setPcNozzleDstTemp NOTIFY pcNozzleDstTempChanged)
    Q_PROPERTY(float pcBedTemp READ pcBedTemp WRITE setPcBedTemp NOTIFY pcBedTempChanged)
    Q_PROPERTY(float pcNozzleTemp READ pcNozzleTemp WRITE setPcNozzleTemp NOTIFY pcNozzleTempChanged)
    Q_PROPERTY(int pcGroup READ pcGroup WRITE setPcGroup NOTIFY pcGroupChanged)
    Q_PROPERTY(float pcX READ pcX WRITE setPcX NOTIFY pcXChanged)
    Q_PROPERTY(float pcY READ pcY WRITE setPcY NOTIFY pcYChanged)
    Q_PROPERTY(float pcZ READ pcZ WRITE setPcZ NOTIFY pcZChanged)
	Q_PROPERTY(bool xyAutohome READ xyAutohome WRITE setXyAutohome NOTIFY xyAutohomeChanged)
	Q_PROPERTY(bool zAutohome READ zAutohome WRITE setZAutohome NOTIFY zAutohomeChanged)
    Q_PROPERTY(bool fanOpened READ fanOpened WRITE setFanOpened NOTIFY fanOpenedChanged)
	Q_PROPERTY(bool caseFan READ caseFan WRITE setCaseFan NOTIFY caseFanChanged)
	Q_PROPERTY(bool auxiliaryFan READ auxiliaryFan WRITE setAuxiliaryFan NOTIFY auxiliaryFanChanged)
    Q_PROPERTY(bool ledOpened READ ledOpened WRITE setLedOpened NOTIFY ledOpenedChanged)
    Q_PROPERTY(float gCodeTransProgress READ gCodeTransProgress WRITE setGCodeTransProgress NOTIFY gCodeTransProgressChanged)
	Q_PROPERTY(float gCodeImportProgress READ gCodeImportProgress WRITE setGCodeImportProgress NOTIFY gCodeImportProgressChanged)
	Q_PROPERTY(QString sentFileName READ sentFileName WRITE setSentFileName NOTIFY sentFileNameChanged)
	Q_PROPERTY(QString errorMessage READ errorMessage WRITE setErrorMessage NOTIFY errorMessageChanged)
	Q_PROPERTY(QString transErrorMessage READ transErrorMessage WRITE setTransErrorMessage NOTIFY transErrorMessageChanged)
	Q_PROPERTY(int nozzleCount READ nozzleCount WRITE setNozzleCount NOTIFY nozzleCountChanged)
	Q_PROPERTY(float nozzle2Temp READ nozzle2Temp WRITE setNozzle2Temp NOTIFY nozzle2TempChanged)
	Q_PROPERTY(float chamberTemp READ chamberTemp WRITE setChamberTemp NOTIFY chamberTempChanged)
	Q_PROPERTY(float nozzle2DstTemp READ nozzle2DstTemp WRITE setNozzle2DstTemp NOTIFY nozzle2DstTempChanged)
	Q_PROPERTY(float chamberDstTemp READ chamberDstTemp WRITE setChamberDstTemp NOTIFY chamberDstTempChanged)

	Q_PROPERTY(int fanSpeed READ fanSpeed WRITE setFanSpeed NOTIFY fanSpeedChanged)
	Q_PROPERTY(int caseFanSpeed READ caseFanSpeed WRITE setCaseFanSpeed NOTIFY caseFanSpeedChanged)
	Q_PROPERTY(int auxiliaryFanSpeed READ auxiliaryFanSpeed WRITE setAuxiliaryFanSpeed NOTIFY auxiliaryFanSpeedChanged)

	Q_PROPERTY(float machineHeight READ machineHeight)
	Q_PROPERTY(float machineWidth READ machineWidth)
	Q_PROPERTY(float machineDepth READ machineDepth)

	Q_PROPERTY(QString sendingFileName READ sendingFileName NOTIFY sendingFileNameChanged)

    explicit ListModelItem(QObject* parent = nullptr);
    ListModelItem(const ListModelItem& listModelItem);
    ~ListModelItem();

    const QString &pcPrinterID() const;
	void setPcPrinterID(const QString& printerID);

    const QString &pcPrinterModel() const; 
	void setPcPrinterModel(const QString& printerModel);

	int errorKey() const;
	void setErrorKey(int key);

	int errorCode() const;
	void setErrorCode(int code);

	int fluiddPort() const;
	void setFluiddPort(int port);

	int printerType() const;
	void setPrinterType(int type);

	int multiNumber() const;
	void setMultiNumber(int number);

	int mainsailPort() const;
	void setMainsailPort(int port);

	int moonrakerPort() const;
	void setMoonrakerPort(const int& port);

    int pcPrinterState() const;
    void setPcPrinterState(int state);

	int pcPrinterStatus() const;
	void setPcPrinterStatus(int status);

	int pcPrinterMethod() const;
	void setPcPrinterMethod(int method);

    int pcHasCamera() const;
	void setPcHasCamera(int hasCamera);

	int pcHasSelect() const;
	void setPcHasSelect(int hasSelect);

	int enableSelfTest() const;
	void setEnableSelfTest(int enabled);

    const QString &pcDeviceName() const;
    void setPcDeviceName(const QString& deviceName);

    const QString &pcIpAddr() const;
	void setPcIpAddr(const QString& ipAddr);

    const QString &pcGCodeName() const;
	void setPcGCodeName(const QString& gcodeName);

    int pcTotalPrintTime() const;
	void setPcTotalPrintTime(int totalPrintTime); 

    int pcLeftTime() const;
	void setPcLeftTime(int leftTime);

	int pcCurPrintLayer() const;
	void setPcCurPrintLayer(int layer);

	int pcTotalPrintLayer() const;
	void setPcTotalPrintLayer(int totalLayer);

    int pcCurPrintProgress() const;
	void setPcCurPrintProgress(int progress);

    int pcPrintSpeed() const;
    void setPcPrintSpeed(int speed);

    int pcBedDstTemp() const;
    void setPcBedDstTemp(int bedDstTemp);

    int pcNozzleDstTemp() const;
    void setPcNozzleDstTemp(int nozzleDstTemp);

	float pcBedTemp() const;
	void setPcBedTemp(float bedTemp);

	float pcNozzleTemp() const;
	void setPcNozzleTemp(float nozzleTemp);

    int pcGroup() const;
    void setPcGroup(int groupNo);

    float pcX() const;
    void setPcX(float newPcX);

    float pcY() const;
    void setPcY(float newPcY);

    float pcZ() const;
    void setPcZ(float newPcZ);

	bool xyAutohome() const;
	void setXyAutohome(bool home);

	bool zAutohome() const;
	void setZAutohome(bool home);

    bool fanOpened() const;
    void setFanOpened(bool opened);

	bool caseFan() const;
	void setCaseFan(bool opened);

	bool auxiliaryFan() const;
	void setAuxiliaryFan(bool opened);

	int fanSpeed() const;
	void setFanSpeed(int speed);

	int caseFanSpeed() const;
	void setCaseFanSpeed(int speed);

	int auxiliaryFanSpeed() const;
	void setAuxiliaryFanSpeed(int speed);

    bool ledOpened() const;
    void setLedOpened(bool opened);

	float gCodeTransProgress() const;
	void setGCodeTransProgress(float progress);

	float gCodeImportProgress() const;
	void setGCodeImportProgress(float progress);

	QString sentFileName() const;
	void setSentFileName(const QString& filename);

	QString errorMessage() const;
	void setErrorMessage(const QString& message);

	QString transErrorMessage() const;
	void setTransErrorMessage(const QString& message);

	int nozzleCount() const;
	void setNozzleCount(int nozzleCount);

	float nozzle2Temp() const;
	void setNozzle2Temp(float nozzle2Temp);

	float chamberTemp() const;
	void setChamberTemp(float chamberTemp);

	float nozzle2DstTemp() const;
	void setNozzle2DstTemp(float nozzle2Temp);

	float chamberDstTemp() const;
	void setChamberDstTemp(float chamberTemp);

	float machineHeight() const;
	void setMachineHeight(float height);
	float machineWidth() const;
	void setMachineWidth(float width);
	float machineDepth() const;
	void setMachineDepth(float depth);

	QString sendingFileName() const;
	void setSendingFileName(const QString& filename);

	friend class CusListModel;

signals:
	void errorKeyChanged();
	void errorCodeChanged();
	void fluiddPortChanged();
	void printerTypeChanged();
	void multiNumberChanged();
	void mainsailPortChanged();
	void pcPrinterModelChanged();
    void pcPrinterStateChanged();
	void pcPrinterStatusChanged();
	void pcPrinterMethodChanged();
	
    void pcHasCameraChanged();
	void pcHasSelectChanged();
	void enableSelfTestChanged();
    void pcDeviceNameChanged();
    void pcIpAddrChanged();
    void pcGCodeNameChanged();
    void pcTotalPrintTimeChanged();
    void pcLeftTimeChanged();
	void pcCurPrintLayerChanged();
	void pcTotalPrintLayerChanged();
	void pcCurPrintProgressChanged();
    void pcPrintSpeedChanged();
    void pcBedDstTempChanged();
    void pcNozzleDstTempChanged();
    void pcBedTempChanged();
    void pcNozzleTempChanged();
    void pcGroupChanged();
    void pcXChanged();
    void pcYChanged();
    void pcZChanged();
	void xyAutohomeChanged();
	void zAutohomeChanged();
    void pcPrinterIDChanged();
    void fanOpenedChanged();
	void caseFanChanged();
	void auxiliaryFanChanged();
    void ledOpenedChanged();
    void gCodeTransProgressChanged();
	void gCodeImportProgressChanged();
	void sentFileNameChanged();
	void errorMessageChanged();
	void transErrorMessageChanged();
	void nozzleCountChanged();
	void nozzle2TempChanged();
	void chamberTempChanged();
	void nozzle2DstTempChanged();
	void chamberDstTempChanged();
	void fanSpeedChanged();
	void caseFanSpeedChanged();
	void auxiliaryFanSpeedChanged();
	void sendingFileNameChanged();

private:
	QVector<int> m_roles = QVector<int>();
	QString m_pcPrinterID;
    QString m_pcPrinterModel;
	int m_errorKey;
	int m_errorCode;
	int m_fluiddPort;
	int m_printerType;
	int m_multiNumber;
	int m_mainsailPort;
	int m_moonrakerPort;
    int m_pcPrinterState;
	int m_pcPrinterStatus;
	int m_pcPrinterMethod;//0:local 1:lan 2:cloud
    int m_pcHasCamera;
	int m_pcHasSelect;
	int m_enableSelfTest;
    QString m_pcDeviceName;
    QString m_pcIpAddr;
    QString m_pcGCodeName;
    int m_pcTotalPrintTime;
    int m_pcLeftTime;
	int m_pcCurPrintLayer;
	int m_pcTotalPrintLayer;
    int m_pcCurPrintProgress;
    int m_pcPrintSpeed;
    int m_pcBedDstTemp;
    int m_pcNozzleDstTemp;
    float m_pcBedTemp;
	float m_pcNozzleTemp;
    int m_pcGroup;
	float m_pcX;
	float m_pcY;
	float m_pcZ;
	bool m_xyAutohome;
	bool m_zAutohome;
    bool m_fanOpened;
	bool m_caseFan;
	bool m_auxiliaryFan;
    bool m_ledOpened;
	float m_gCodeTransProgress;
	float m_gCodeImportProgress;
	QString m_sentFileName;
	QString m_errorMessage;
	QString m_transErrorMessage;
	int m_nozzleCount;
	float m_nozzle2Temp;
	float m_chamberTemp;
	float m_nozzle2DstTemp;
	float m_chamberDstTemp;
	int m_fanSpeed;
	int m_caseFanSpeed;
	int m_auxiliaryFanSpeed;
	float m_machineHeight;
	float m_machineWidth;
	float m_machineDepth;
	QString m_sendingFileName;
};
Q_DECLARE_METATYPE(ListModelItem)

#endif
