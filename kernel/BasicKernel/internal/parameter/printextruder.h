#ifndef CREATIVE_KERNEL_PRINTEXTRUDER_1675853730711_H
#define CREATIVE_KERNEL_PRINTEXTRUDER_1675853730711_H
#include "internal/parameter/parameterbase.h"

namespace creative_kernel
{
	class ProfileParameterModel;
	class PrintMaterial;
	class PrintExtruder : public ParameterBase
	{
		Q_OBJECT
		Q_PROPERTY(int materialIndex READ materialIndex NOTIFY materialIndexChanged)
	public:
		PrintExtruder(const QString& machineName, int index, QObject* parent = nullptr);
		virtual ~PrintExtruder();

		void added();
		void removed();
		void setEnabled(bool enabled) { m_enabled = enabled; }
		bool getEnabled() { return m_enabled; }

		Q_INVOKABLE void setMaterial(const QString& materialName);
		Q_INVOKABLE void setMaterial(int materialIndex);
		Q_INVOKABLE QAbstractListModel* extruderParameterModel(const QString& category, bool professional/* = false*/);
		Q_INVOKABLE QString materialName();
		Q_INVOKABLE void save();
		Q_INVOKABLE void cancel();
		Q_INVOKABLE void reset();
		Q_INVOKABLE void reset(const QString& category, bool professional/* = false*/);
		int materialIndex();

	signals:
		void materialIndexChanged();

	protected:
		int m_index;
		QString m_machineName;
		bool m_enabled = true;
		QString m_materialName;

		std::map<QString, ProfileParameterModel*>  m_extruderParameterModels;
	};
}

#endif // CREATIVE_KERNEL_PRINTEXTRUDER_1675853730711_H