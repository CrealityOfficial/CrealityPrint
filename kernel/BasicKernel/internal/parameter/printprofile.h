#ifndef CREATIVE_KERNEL_PRINTPROFILE_1675853730711_H
#define CREATIVE_KERNEL_PRINTPROFILE_1675853730711_H
#include "internal/parameter/parameterbase.h"
#include "internal/models/parameterdatamodel.h"

namespace creative_kernel
{
	class ParameterDataModel;
	class PrintProfile : public ParameterBase
	{
		Q_OBJECT;
		Q_PROPERTY(QObject* dataModel READ basicDataModel CONSTANT);
		Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged);
	public:
		PrintProfile(const QString& machineName,
			const QString& fileName, QObject* parent = nullptr, bool isDefault = true);
		virtual ~PrintProfile();

		void added();
		void removed();
		void setDefalut(bool bDefault) {m_isDefault=bDefault;}

		void setName(const QString& newName);
		Q_INVOKABLE QString name();
		Q_INVOKABLE QString uniqueName();
		Q_INVOKABLE void changeUserFileName(const QString& newName);

		Q_INVOKABLE QAbstractListModel* profileParameterModel(const bool& isSingleExtruder);
		Q_INVOKABLE QObject* basicDataModel();
		ParameterDataModel* getDataModel();

		Q_INVOKABLE QString profileFileName();
		Q_INVOKABLE bool isDefault();
		void setPrintMachine(ParameterBase* base) { m_curPrintMachine = base; }
		ParameterBase* curPrintMachine() {
			return m_curPrintMachine;
		}
		void enableChanged(const QString& key, bool enabled) override;
		void strChanged(const QString& key, const QString& str) override;
		Q_INVOKABLE void save() override;
		Q_INVOKABLE void cancel();
		Q_INVOKABLE void reset() override;

		//���ղ�ͬ�����ĶԱ�
		Q_INVOKABLE QStringList processDiffKeys(QObject* obj);

	signals:
		void nameChanged();
		void enablePrimeTowerChanged(bool enabled);
		void currentProcessLayerHeightChanged(float height);
		void currentProcessPrimeVolumeChanged(float volume);
		void currentProcessPrimeTowerWidthChanged(float width);
		void enableSupportChanged(bool enabled);
		void currentProcessSupportFilamentChanged(int index);
		void currentProcessSupportInterfaceFilamentChanged(int index);
		void printSequenceChanged(const QString& str);

	private:
		QString getCurrentMaterialName() const;
		void generateName(const QString& fileName);

	protected:
		QString m_name;
		QString m_profileFileName;
		QString m_machineName;
		bool m_isDefault;

		ParameterDataModel* m_BasicDataModel = nullptr;
		ParameterBase* m_curPrintMachine = nullptr;
	};
}

#endif // CREATIVE_KERNEL_PRINTPROFILE_1675853730711_H
