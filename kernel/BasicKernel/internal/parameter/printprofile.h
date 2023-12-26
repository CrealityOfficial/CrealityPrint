#ifndef CREATIVE_KERNEL_PRINTPROFILE_1675853730711_H
#define CREATIVE_KERNEL_PRINTPROFILE_1675853730711_H
#include "internal/parameter/parameterbase.h"

namespace creative_kernel
{
	class ParameterProfileCategoryModel;
	class ProfileParameterModel;
	class PrintProfile : public ParameterBase
	{
		Q_OBJECT
	public:
		PrintProfile(const QString& machineName, 
			const QString& fileName, QObject* parent = nullptr);
		virtual ~PrintProfile();

		void added();
		void removed();

		Q_INVOKABLE QString name();
		QString uniqueName();

		Q_INVOKABLE QAbstractListModel* profileCategoryModel(const bool& isSingleExtruder);
		Q_INVOKABLE QAbstractListModel* profileParameterModel(const bool& isSingleExtruder);

		Q_INVOKABLE void setparameterValue(QString key, QString value);
		Q_INVOKABLE QVariantList parameterList(const QString key);

		Q_INVOKABLE QString profileFileName();
		bool isDefault();
		void setPrintMachine(ParameterBase* base) { m_curPrintMachine = base; }
		ParameterBase* curPrintMachine() {
			return m_curPrintMachine;
		}
		Q_INVOKABLE void save() override;
		Q_INVOKABLE void cancel();
		Q_INVOKABLE void reset() override;
		Q_INVOKABLE void reset(us::USettings* us);

		QMap<QString, QList<QString> > getAffetecdKeys();
		QString getModelValue(QString key) const;

	private:
		QString getCurrentMaterialName() const;

	protected:
		QString m_name;
		QString m_profileFileName;
		QString m_machineName;
		bool m_isDefault;
		ParameterBase* m_curPrintMachine;
		ProfileParameterModel* m_profileParameterModel;
		ParameterProfileCategoryModel* m_parameterProfileCategoryModel;
	};
}

#endif // CREATIVE_KERNEL_PRINTPROFILE_1675853730711_H