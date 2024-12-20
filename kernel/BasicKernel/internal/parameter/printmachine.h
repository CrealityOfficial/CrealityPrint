#ifndef CREATIVE_KERNEL_PRINTMACHINE_1675853730710_H
#define CREATIVE_KERNEL_PRINTMACHINE_1675853730710_H
#include "internal/parameter/parameterbase.h"
#include "printMaterialModel.h"
#include "printMaterialModelProxy.h"
#include "materialcenter.h"
#include <QVariant>
#include <QMetaType>
namespace creative_kernel
{
	class PrintExtruder;
	class PrintProfile;
	class PrintMaterial;
	class PrintProfileListModel;
	class PrintMaterialListModel;
	class ProfileParameterModel;
	class ExtruderListModel;
	class ParameterDataModel;
	class ParamCheckStruct:public QObject
	{
		Q_OBJECT
		public:
			explicit ParamCheckStruct(QObject* parent = nullptr) : QObject(parent) {};
			ParamCheckStruct(const ParamCheckStruct& pcs) {};
			Q_INVOKABLE QString getKeyStr() { return keyStr; };
			Q_INVOKABLE QString getDefaultValue() { return defaultValue; };
			Q_INVOKABLE QString getCurrentValue() { return currentValue; };
		signals:
		public:
			QString keyStr;
			QString defaultValue;
			QString currentValue;
	};

    class PrintMachine : public ParameterBase {
        Q_OBJECT

        Q_PROPERTY(QStringList materialsName READ materialsName NOTIFY materialsNameChanged)
        Q_PROPERTY(QString inheritBaseName READ inheritBaseName)
		Q_PROPERTY(QStringList userMaterialsName READ userMaterialsName NOTIFY userMaterialsNameChanged)
		Q_PROPERTY(QStringList defaultMaterialsName READ defaultMaterialsName NOTIFY defaultMaterialsNameChanged)
        Q_PROPERTY(QStringList selectMaterialsName READ selectMaterialsName NOTIFY selectMaterialsNameChanged)
        Q_PROPERTY(QAbstractListModel* profilesModel READ profilesModel NOTIFY profilesModelChanged)
        Q_PROPERTY(QAbstractListModel* materialsModel READ materialsModel NOTIFY materialsModelChanged)
        Q_PROPERTY(QAbstractListModel* extrudersModel READ extrudersModel NOTIFY extrudersModelChanged)
        Q_PROPERTY(QObject* currentProfileObject READ currentProfileObject NOTIFY curProfileIndexChanged)
        Q_PROPERTY(QObject* modifiedProcessObject READ modifiedProcessObject NOTIFY modifiedProcessObjectChanged)
        Q_PROPERTY(int curProfileIndex READ curProfileIndex NOTIFY curProfileIndexChanged)
        Q_PROPERTY(bool isCurrentProfileDefault READ isCurrentProfileDefault NOTIFY isCurrentProfileDefaultChanged)
		Q_PROPERTY(bool isDefault READ isDefault WRITE setIsDefault NOTIFY isDefaultChanged)
        Q_PROPERTY(QObject* dataModel READ machineParameterModel NOTIFY dataModelChanged)
		Q_PROPERTY(bool canUpload3mf READ canUpload3mf NOTIFY canUpload3mfChanged)

    public:
		enum PrintType
		{
			PT_DEFAULT,
			PT_FROMDEFAULT,
			PT_FROMUSER
		};

		enum MaterialType
		{
			MaterialSlot_Empty, // "/"
			MaterialSlot_Unknown, // "?"
			MaterialSlot_Type // "type"
		};

		struct SyncExtruderInfo {
			QString color;
			QString boxIndex;
			QString materialRFID;
			QString materialName;
		};

		PrintMachine(QObject* parent = nullptr, const MachineData& meta = MachineData(), PrintType pt = PT_DEFAULT);
		PrintMachine(const PrintMachine& PrintMachine) {}

		virtual ~PrintMachine();

		Q_INVOKABLE QString name();
		Q_INVOKABLE QString baseName();
		Q_INVOKABLE QString codeName();
		Q_INVOKABLE QString inheritBaseName();
		QList<QString> topMaterial();

		bool isDefault();
		void setIsDefault(bool isUser);

		void strChanged(const QString& key, const QString& str) override;

		float machineExtruderDiameters();
		Q_INVOKABLE QString uniqueName();
		Q_INVOKABLE QString uniqueShowName();

		bool isBelt();
		bool isUserMachine();
		Q_INVOKABLE bool isImportedMachine();
        bool centerIsZero();
		int machineType(); //1: fmd, 2 : fdm - laser, 3 : fdm - laser - cnc
		bool canUpload3mf();

        float machineWidth();
        float machineDepth();
        float machineHeight();

		int currentPlateIndex();
		void setCurrentPlateIndex(const int& index);
		void setPrintSequence(const QString& print_sequence);
		void setCurrBedType(const QString& curr_bed_type);
		void setFirstLayerPrintSequence(const QString& first_layer_print_sequence);
		void resetPlateSettings();

		bool enablePrimeTower();
		float currentProcessLayerHeight();
		float currentProcessPrimeVolume();
		float currentProcessPrimeTowerWidth();
		bool enableSupport();
		int currentProcessSupportFilament();
		int currentProcessSupportInterfaceFilament();
		float extruderClearanceRadius();
		float extruderClearanceHeightToLid();
		float extruderClearanceHeightToRod();
		float initialLayerPrintHeight();
		float minLayerHeight();
		float maxLayerHeight();
		QString _printSequence();
		float nozzle_volume();

		float nozzleSize(int index);
		Q_INVOKABLE float currentNozzleSize();
		Q_INVOKABLE void added(bool isInitialize = false);
		Q_INVOKABLE void addImport();
		Q_INVOKABLE void reset();
		void mergeAndSaveExtruders();
		void resetExtruders();
		Q_INVOKABLE void removed();

		//
		Q_INVOKABLE QObject* machineParameterModel();
		Q_INVOKABLE QObject* extruder1DataModel();
		Q_INVOKABLE QObject* extruder2DataModel();
		ParameterDataModel* getExtruder1DataModel();
		ParameterDataModel* getExtruder2DataModel();
		ParameterDataModel* getExtruderDataModel(size_t index);
		ParameterDataModel* getDataModel();
		PrintExtruder* getExtruder(int index);

		//profile
		Q_INVOKABLE QAbstractListModel* profilesModel();
		Q_INVOKABLE QStringList profileNames();
		Q_INVOKABLE int curProfileIndex();
		PrintProfile* profile(int index);
		int profilesCount();
		Q_INVOKABLE QObject* profileObject(int index);
		PrintProfile* currentProfile();
		QObject* currentProfileObject();
		QString getProcessValue(const QString& processName, const QString& key);

		Q_INVOKABLE bool isModified();
		Q_INVOKABLE bool isMaterialModified();
		Q_INVOKABLE QString modifiedMaterialName();
		Q_INVOKABLE QString modifiedProcessName();
		Q_INVOKABLE PrintProfile* modifiedProcessObject();
		Q_INVOKABLE bool isCurrentProcessModified();

		Q_INVOKABLE void saveMaterial();
		Q_SIGNAL void materialSaved(QObject* material);
		Q_INVOKABLE void abandonMaterialMod();
		Q_INVOKABLE void abandonProcessMod();

		Q_INVOKABLE bool hasMaterial(const QString& name);
		Q_INVOKABLE bool isMaterialInFactory(const QString& name);
		Q_INVOKABLE bool isMaterialInUser(const QString& name);
		Q_INVOKABLE bool isProcessInFactory(const QString& name);
		Q_INVOKABLE bool isProcessInUser(const QString& name);
		Q_INVOKABLE bool checkMaterialName(const QString& oldName, const QString& newName);
		Q_INVOKABLE bool checkProfileName(const QString& name);
		Q_INVOKABLE QString generateNewProfileName(const QString& prefix = QString(""));
		Q_INVOKABLE void addProfile(const QString& profileName, const QString& templateProfile);
		Q_INVOKABLE void saveCurrentProfile();
		Q_INVOKABLE void removeProfile(QObject* profile);
		Q_INVOKABLE void setCurrentProfile(int index);
		Q_INVOKABLE bool isCurrentProfileDefault();

		//打印机配置导入导出
		Q_INVOKABLE void exportPrinterFromQml(const QString& urlString);
		//耗材配置导入导出
		Q_INVOKABLE void importMaterialFromQml(const QString& urlString);
		//配置文件导入导出
		Q_INVOKABLE void exportProfileFromQml(const QString& urlString);
		Q_INVOKABLE void exportProfileFromQmlIni(const QString& urlString);
		Q_INVOKABLE void importProfileFromQml(const QString& urlString);
		Q_INVOKABLE void importProfileFromQmlIni(const QString& urlString);

		//重新同步耗材
		Q_INVOKABLE void resyncExtruders(const QStringList& rfid, const QVariantList& color, const QStringList& syncIdList, const QStringList& stateList = QStringList());
		Q_INVOKABLE void savePrinterSlot(QList<QVariantMap> data);
		Q_INVOKABLE QString findMaterialByRFID(const QString& rfid);
		//同步
		Q_INVOKABLE void syncExtruders(const QStringList& rfid, const QVariantList& color, QList<QVariantMap> data);

		Q_INVOKABLE void reloadMaterial(const QString& materialName);
		Q_INVOKABLE void reloadProcess(const QString& processName);

		//extruder
		Q_INVOKABLE void reload();
		Q_INVOKABLE int extruderCount();//physical
		Q_INVOKABLE bool currentMachineSupportExportImage();
		bool currentMachineIsBbl();
		const QList<PrintExtruder*>& extruders() const;
		Q_INVOKABLE QObject* extruderObject(int index);
		Q_INVOKABLE int extruderMaterialIndex(int extruderIndex);
		Q_INVOKABLE QString extruderMaterialName(int extruderIndex);
		Q_INVOKABLE void setExtruderMaterial(int extruderIndex, const QString& materialName);
		Q_INVOKABLE void setExtruderMaterial(int extruderIndex, int materialIndex);
		Q_INVOKABLE int extruderAllCount();
		//Q_INVOKABLE void setExtruderColor(int extruderIndex, const QColor& extruderColor);
		Q_INVOKABLE void setExtruderColor(int extruderIndex, const QString& extruderColor);
		Q_INVOKABLE QColor extruderColor(int extruderIndex) const;
		Q_INVOKABLE void addExtruder();
		Q_INVOKABLE void removeExtruder();
		Q_INVOKABLE void setFilamentColor(const QVariantList& colorList);
		QString getExtruderValue(const QString& key);

		//materials
		Q_INVOKABLE QAbstractListModel* materialsModel();
		Q_INVOKABLE QVariant materialsModelProxy();
		PrintMaterial* material(int index);
		int materialCount();

		Q_INVOKABLE bool modifyMaterialName(const QString& oldMaterialName, const QString& newMaterialName);
		Q_INVOKABLE QStringList supportTypes();
		Q_INVOKABLE QStringList selectTypes(int extruderIndex = 0);
		Q_INVOKABLE void selectChanged(bool checked, QString materialName, int extruderIndex = 0);

		Q_INVOKABLE QObject* materialObject(int index) const;
		Q_INVOKABLE QObject* materialObject(const QString& materialName) const;
		PrintMaterial* materialAt(int index) const;
		PrintMaterial* materialWithName(const QString& materialName) const;
		void filterMeterialsFromTypes();
		MaterialData findMetaByFile(const QString& materialName);
		//lisugui addmeterialdlg
		//返回耗材添加页面的数据
		// Q_INVOKABLE QVariant materialsModel();
		//添加用户自定义耗材
		Q_INVOKABLE int addUserMaterial(const QString& materialModel, const QString& userMaterialName, int extruderNum = 0);
		//删除用户自定义耗材
		Q_INVOKABLE void deleteUserMaterial(const QString& materialName, int index);
		//添加机器同步过来的耗材
		void addMachineMaterial(const QStringList& rfid, const QVariantList& color);
		//导入材料
		Q_INVOKABLE void importMaterial(const QString& srcPath, const QString& userMaterialName, int extruderNum = 0);
		Q_INVOKABLE void finishMeterialSelect(const int& index);
		Q_INVOKABLE void setExtruderStatus(bool extruder0, bool extruder1);
		Q_INVOKABLE void save() override;
		void exportSetting(const QString& fileName) override;

		//
		Q_INVOKABLE QString getFileNameByPath(const QString& path);
		Q_INVOKABLE QString getMaterialType(const QString& path);
		Q_INVOKABLE void transferProfile(QObject* from, QObject* to);
		ExtruderListModel* extrudersListModel();
		QAbstractListModel* extrudersModel();
		QList<us::USetting*> currentMaterialCover();

		//Q_INVOKABLE void reset() override;;
		QString _getCurrentProfile();
		bool machineDirty();
		void clearMachineDirty();
		QSharedPointer<us::USettings> createCurrentGlobalSettings();
		void handleLegacy();
		us::USettings* currentGlobalSettings();

        QList<QSharedPointer<us::USettings>> createCurrentExtruderSettings();

		//接口使用
        Q_INVOKABLE QStringList materialsNameList();
		Q_INVOKABLE QStringList materialsNameInMachine();
		Q_INVOKABLE QStringList nozzleDiameter();
		//显示使用
		QStringList materialsName(int extureIndex = 0);
		QStringList defaultMaterialsName();
		QStringList selectMaterialsName(int extureIndex = 0);
        QStringList userMaterialsName();

		//查询不同耗材和配置的value有差异的keys
		Q_INVOKABLE QStringList materialDiffKeys(int materialIndex1, int materialIndex2);
		Q_INVOKABLE QStringList profileDiffKeys(int profileIndex1, int profileIndex2);
		QStringList materialDiffKeys(const QString materialName1, const QString materialName2);
		QStringList profileDiffKeys(const QString profileName1, const QString profileName2);

		bool isSyncedExtruder();
		QVector<SyncExtruderInfo>& syncedExtrudersInfo();
		void resetState();

	signals:
		void profilesModelChanged();
		void materialsModelChanged();
		void extrudersModelChanged();
        void materialsNameChanged();
		void userMaterialsNameChanged();
		void selectMaterialsNameChanged();
		void curProfileIndexChanged();
		void modifiedProcessObjectChanged();
		void isCurrentProfileDefaultChanged();
		void extruderColorChanged();
		void isDefaultChanged();
		void extruderClearanceRadiusChanged(float height);
		void extruderClearanceHeightToLidChanged(float volume);
		void extruderClearanceHeightToRodChanged(float width);
		void currentBedTypeChanged(const QString& str);
		void defaultMaterialsNameChanged();
		void dataModelChanged();
		void canUpload3mfChanged();

		/// @param extruder PrintExtruder* object
		void extruderAdded(QObject* extruder);

		/// @param extruder PrintExtruder* object
		void extruderRemoved(QObject* extruder);

    protected:
		void _addUserMaterialToJson(const MaterialData& materialMeta);
		void _addProfile(const QString& profileName, const QString& templateProfile);
		void _addProfile(PrintProfile* profile);
		void _removeProfile(PrintProfile* profile);
		void _setCurrentProfile(PrintProfile* profile);
		void _cacheCurrentProfileIndex();
		void _update_model_filament(int numFilaments);

		MaterialData findMetaByName(const QString& machineName);
		QList<QObject*> generatePrinterParamCheck();
		QList<QObject*> generateMaterialParamCheck();
		void _getSelectMaterials(QStringList& selectMaterialNames);
		void _getSupportMaterials(QStringList& supportMaterialNames);
		QList<MaterialData> materialMetasInMachine();
		PrintMaterial* _findMaterial(const QString& materialName);
		void _cacheExtruderMaterialIndex();

		int _caculateLevel(const QString& name);
		QColor getNextExtruderColor();
	private:
		void addExtruder(const QString& color, const QString& curMaterial = QString());


	protected:
		MachineData m_data;
		std::map<QString, ProfileParameterModel*>  m_machineParameterModels;

		//配置
		QList<float> m_nozzleSizes;
		PrintProfileListModel* m_profilesModel=nullptr;
		QList<PrintProfile*> m_profiles;
		PrintProfile* m_currentProfile{ nullptr };
		us::USettings* m_usNeed = nullptr;//校验参数的setting

		//喷嘴
		QList<PrintExtruder*> m_extruders;
		ExtruderListModel* m_extrudersModel{ nullptr };

		//耗材
		QList<QString> m_selectMaterials_0;//当前选择的耗材
		QList<QString> m_selectMaterials_1;
		QList<MaterialData> m_UserMaterialsData;//用户自定义耗材
		QList<MaterialData*> m_SupportMeterialsDataList;//当前机型支持的所有材料类型下的所有具体材料
		QList<PrintMaterial*> m_materials;
		PrintMaterialListModel* m_materialsModel{ nullptr };

		QList<QObject*> m_ParamCheckList;
		//
		ParameterDataModel* m_BasicDataModel = nullptr;
		PrintMaterialModel* m_PrintMaterialModel = nullptr;
		PrintMaterialModelProxy* m_ModelProxy = nullptr;
		QSharedPointer<us::USettings> m_globalSetting = nullptr;
		us::USettings* m_PlateSetting = nullptr;
		QString m_oldBedType;
		QList<QString> m_resyncRfid;
		QList<QVariant> m_resyncColor;
		QList<QVariantMap>m_resyncMap;
	private:
		PrintType m_Pt;
		int m_colorIndex = 0;
		int m_plateIndex = 0;
		bool isSyncedExtruderInfo = false; //是否同步过喷嘴信息
		QVector<SyncExtruderInfo> m_SyncExtruderInfos;
	};

}

Q_DECLARE_METATYPE(creative_kernel::PrintMachine);
Q_DECLARE_METATYPE(creative_kernel::ParamCheckStruct);
#endif // CREATIVE_KERNEL_PRINTMACHINE_1675853730710_H
