#ifndef CREATIVE_KERNEL_PRINTEXTRUDER_1675853730711_H
#define CREATIVE_KERNEL_PRINTEXTRUDER_1675853730711_H
#include "internal/parameter/parameterbase.h"
#include "internal/models/parameterdatamodel.h"
#include <qcolor.h>

namespace creative_kernel
{
	class ProfileParameterModel;
	class PrintMaterial;
	class PrintExtruder : public ParameterBase
	{
		Q_OBJECT
		Q_PROPERTY(int materialIndex READ materialIndex NOTIFY materialIndexChanged)
		Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
		Q_PROPERTY(int extruderIndex READ extruderIndex WRITE setExtruderIndex NOTIFY extruderIndexChanged)
		Q_PROPERTY(QObject* dataModel READ getDataModelObject NOTIFY dataModelChanged)
		Q_PROPERTY(bool physical READ physical CONSTANT)

	public:
		PrintExtruder(const QString& machineName, int index, QObject* parent = nullptr, bool isPhysical = true, const QColor& materialColor = Qt::GlobalColor::white, us::USettings* uSettings = nullptr);
		virtual ~PrintExtruder();

		void added();
		void removed();
		void setEnabled(bool enabled) { m_enabled = enabled; }
		bool getEnabled() { return m_enabled; }
		bool physical() const { return m_isPhysical; }

		void setColor(const QColor& color);
		QColor color() const;

		int extruderIndex();
		void setExtruderIndex(int index);

		PrintMaterial* curMaterial();

		Q_INVOKABLE void setMaterial(const QString& materialName);
		Q_INVOKABLE void setMaterial(int materialIndex);
		Q_INVOKABLE QString materialName();
		Q_INVOKABLE void save() override;
		Q_INVOKABLE void reset() override;
		Q_INVOKABLE void cancel();
		Q_INVOKABLE int index() {return m_index;}
		Q_INVOKABLE int materialIndex();

		ParameterDataModel* getDataModel() const;
		Q_INVOKABLE QObject* getDataModelObject() const;

	signals:
		void materialIndexChanged();
		void colorChanged();
		void extruderIndexChanged();
		void dataModelChanged();

	protected:
		int m_index;
		QString m_machineName;
		bool m_enabled = true;
		QString m_materialName;
		bool m_isPhysical = true;
		QColor m_materialColor;

		mutable ParameterDataModel* m_dataModel{ nullptr };

		std::map<QString, ProfileParameterModel*>  m_extruderParameterModels;
	};
}

#endif // CREATIVE_KERNEL_PRINTEXTRUDER_1675853730711_H
