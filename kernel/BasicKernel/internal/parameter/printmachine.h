#ifndef CREATIVE_KERNEL_PRINTMACHINE_1675853730710_H
#define CREATIVE_KERNEL_PRINTMACHINE_1675853730710_H
#include "internal/parameter/parameterbase.h"
#include "printMaterialModel.h"
#include "printMaterialModelProxy.h"
#include <QVariant>
#include <QMetaType>
namespace creative_kernel
{
	class PrintExtruder;
	class PrintProfile;
	class PrintMaterial;
	class PrintProfileListModel;
	class ProfileParameterModel;
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


	class PrintMachine : public ParameterBase
	{
		Q_OBJECT
        Q_PROPERTY(QStringList materialsName READ materialsName NOTIFY materialsNameChanged)
		Q_PROPERTY(QStringList selectMaterialsName READ selectMaterialsName NOTIFY selectMaterialsNameChanged)
        Q_PROPERTY(QStringList userMaterialsName READ userMaterialsName NOTIFY userMaterialsNameChanged)
		Q_PROPERTY(int curProfileIndex READ curProfileIndex NOTIFY curProfileIndexChanged)
		Q_PROPERTY(bool isCurrentProfileDefault READ isCurrentProfileDefault NOTIFY isCurrentProfileDefaultChanged)

    public:
		enum PrintType
		{
			PT_DEFAULT,
			PT_FROMDEFAULT,
			PT_FROMUSER
		};

		PrintMachine(QObject* parent = nullptr, const MachineMeta& meta = MachineMeta(),
			const MachineData& data = MachineData(), PrintType pt = PT_DEFAULT);
		virtual ~PrintMachine();

		Q_INVOKABLE QString name();
		QString uniqueName();
		QString uniqueBasicName();

		bool isBelt();
		bool isUserMachine();
		bool isFromUserMachine();
        bool centerIsZero();
		QString fromUserMachineName();
		int machineType(); //1: fmd, 2 : fdm - laser, 3 : fdm - laser - cnc

        float machineWidth();
        float machineDepth();
        float machineHeight();

		float nozzleSize(int index);
		Q_INVOKABLE void added(bool isInitialize = false);
		Q_INVOKABLE void addImport();
		Q_INVOKABLE void reset();
		Q_INVOKABLE void removed();
		Q_INVOKABLE QAbstractListModel* machineParameterModel(const QString& category, const QString& subCategory);
		Q_INVOKABLE QList<QObject*> generateParamCheck();
		Q_INVOKABLE void resetProfileModify();

		//profile
		Q_INVOKABLE QAbstractListModel* profilesModel();
		Q_INVOKABLE QStringList profileNames();
		Q_INVOKABLE int curProfileIndex();
		PrintProfile* profile(int index);
		int profilesCount();
		Q_INVOKABLE QObject* profileObject(int index);
		Q_INVOKABLE QObject* currentProfileObject();

		Q_INVOKABLE bool checkMaterialName(const QString& oldName, const QString& newName);
		Q_INVOKABLE bool checkProfileName(const QString& name);
		Q_INVOKABLE QString generateNewProfileName(const QString& prefix = QString(""));
		Q_INVOKABLE void addProfile(const QString& profileName, const QString& templateProfile);
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

		//extruder
		Q_INVOKABLE int extruderCount();
		Q_INVOKABLE bool currentMachineSupportExportImage();
		Q_INVOKABLE QObject* extruderObject(int index);
		Q_INVOKABLE int extruderMaterialIndex(int extruderIndex);
		Q_INVOKABLE QString extruderMaterialName(int extruderIndex);
		Q_INVOKABLE void setExtruderMaterial(int extruderIndex, const QString& materialName);
		Q_INVOKABLE void setExtruderMaterial(int extruderIndex, int materialIndex);

		//materials
		Q_INVOKABLE bool modifyMaterialName(const QString& oldMaterialName, const QString& newMaterialName);
		Q_INVOKABLE QStringList supportTypes();
		Q_INVOKABLE QStringList selectTypes(int extruderIndex = 0);
		Q_INVOKABLE void selectChanged(bool checked, QString materialName, int extruderIndex = 0);

		Q_INVOKABLE QObject* materialObject(int index);
		Q_INVOKABLE QObject* materialObject(const QString& materialName);

		//lisugui addmeterialdlg
		Q_INVOKABLE void filterMeterialsFromTypes();
		//返回耗材添加页面的数据
		Q_INVOKABLE QVariant materialsModel();
		//添加用户自定义耗材
		Q_INVOKABLE void addUserMaterial(const QString& materialModel, const QString& userMaterialName, int extruderNum = 0);
		//删除用户自定义耗材
		Q_INVOKABLE void deleteUserMaterial(const QString& materialName, int index);
		//导入材料
		Q_INVOKABLE void importMaterial(const QString& srcPath, const QString& userMaterialName, int extruderNum = 0);
		Q_INVOKABLE void finishMeterialSelect(const int& index);
		Q_INVOKABLE void setExtruderStatus(bool extruder0, bool extruder1);
		Q_INVOKABLE void save() override;
		//
		Q_INVOKABLE QString getFileNameByPath(const QString& path);
		Q_INVOKABLE QString getMaterialType(const QString& path);

		QList<us::USetting*> currentMaterialCover();
		//Q_INVOKABLE void reset() override;;
		QString _getCurrentProfile();
		bool machineDirty();
		void clearMachineDirty();
		QSharedPointer<us::USettings> createCurrentGlobalSettings();
        QList<QSharedPointer<us::USettings>> createCurrentExtruderSettings();

		//接口使用
        Q_INVOKABLE QStringList materialsNameList();
        Q_INVOKABLE QStringList materialsNameInMachine();
		//显示使用
		QStringList materialsName(int extureIndex = 0);
		QStringList selectMaterialsName(int extureIndex = 0);
        QStringList userMaterialsName();

    signals:
        void materialsNameChanged();
		void userMaterialsNameChanged();
		void selectMaterialsNameChanged();
		void curProfileIndexChanged();
		void isCurrentProfileDefaultChanged();

    protected:
		void _addUserMaterialToJson(const MaterialMeta& materialMeta);
		void _addProfile(const QString& profileName, const QString& templateProfile);
		void _addProfile(PrintProfile* profile);
		void _removeProfile(PrintProfile* profile);
		void _setCurrentProfile(PrintProfile* profile);
		void _cacheCurrentProfileIndex();

		QList<QObject*> generatePrinterParamCheck();
		QList<QObject*> generateMaterialParamCheck();
		void _getSelectMaterials(QStringList& selectMaterialNames);
		void _getSupportMaterials(QStringList& supportMaterialNames);
		QList<MaterialMeta> materialMetasInMachine();
		PrintMaterial* _findMaterial(const QString& materialName);
		void _cacheSelectMaterials(const int& index);
		void _cacheExtruderMaterialIndex();

		int _caculateLevel(const QString& name);

	protected:
		MachineMeta m_meta;
		MachineData m_data;
		std::map<QString, ProfileParameterModel*>  m_machineParameterModels;

		//配置
		QList<float> m_nozzleSizes;
		PrintProfileListModel* m_profilesModel;
		QList<PrintProfile*> m_profiles;
		PrintProfile* m_currentProfile;
		PrintProfile* m_emptyProfile;
		us::USettings* m_usNeed = nullptr;//校验参数的setting

		//喷嘴
		QList<PrintExtruder*> m_extruders;
		PrintExtruder* m_emptyExtruder;

		//耗材
		QList<QString> m_selectMaterials_0;
		QList<QString> m_selectMaterials_1;
		QList<MaterialMeta> m_UserMaterialsData;//用户当前选中的材料
		QList<MaterialMeta*> m_SupportMeterialsDataList;//当前机型支持的所有材料类型下的所有具体材料
		QList<PrintMaterial*> m_materials;
		PrintMaterial* m_emptyMaterial;

		QList<QObject*> m_ParamCheckList;
		//
		PrintMaterialModel* m_PrintMaterialModel = nullptr;
		PrintMaterialModelProxy* m_ModelProxy = nullptr;

	private:
		PrintType m_Pt;
	};
	
}
Q_DECLARE_METATYPE(creative_kernel::ParamCheckStruct);
#endif // CREATIVE_KERNEL_PRINTMACHINE_1675853730710_H
