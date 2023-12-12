#ifndef CREATIVE_KERNEL_PROFILEPARAMETERMODEL_1676527659531_H
#define CREATIVE_KERNEL_PROFILEPARAMETERMODEL_1676527659531_H
#include <QtCore/QAbstractListModel>
#include "us/usettings.h"
#include "internal/models/parameterprofilecategorymodel.h"
namespace creative_kernel
{
	class PrintProfile;
	class ProfileParameterModel : public QAbstractListModel 
	{
        Q_OBJECT;
	public:
		ProfileParameterModel(us::USettings* settings, QObject* parent = nullptr);
		virtual ~ProfileParameterModel();

		Q_INVOKABLE void setFilter(const QString& category, const QString& filter, bool professional);
		Q_INVOKABLE void setMaterialCategory(const QString& category); // property flow cool temperature gcode
		Q_INVOKABLE void setExtruderCategory(const QString& category, const bool& professional = false); // basic linewidth retraction
		Q_INVOKABLE void setMachineCategory(const QString& category, const QString& subCategory); // basic move
		Q_INVOKABLE void reCalculateSetting(QString key,bool bReset=false);
		Q_INVOKABLE QString value(QString key) const;
		Q_INVOKABLE QString defaultValue(QString key) const;
		Q_INVOKABLE void setValue(QString key,QString value);
		Q_INVOKABLE void setOverideValue(QString key, QString value);
		Q_INVOKABLE bool isUserSetting(QString key) const;
		Q_INVOKABLE void resetValue(QString key);
		Q_INVOKABLE void sestExtruderIndex(int index) { m_extruderindex = index; }
		Q_INVOKABLE QObject* search(const QString& category, const QString& filter, bool professional);
		Q_INVOKABLE QAbstractListModel* searchCategoryModel(bool professional, bool isSingleExtruder, const QString& filter);
		Q_INVOKABLE void addSearchSettings(QObject* settings);
		void applyUserSetting(us::USettings* settings);
		void mergeMachineSettings(const int& extruderIndex);
		void mergeMaterialSettings(const int& extruderIndex);
		void mergeFilterSettings(us::USettings* settings);
		QList<us::USetting*> filtterSettings() { return m_filterSettings; }
		bool hasKeyInFilter(const QString& key);
		QMap<QString, QList<QString> > getAffetecdKeys() const {
			return m_affectedKeys;
		};
	protected:
		int rowCount(const QModelIndex& parent = QModelIndex{}) const override;
		QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
		bool setData(const QModelIndex& index, const QVariant& value, int role) override;
		QHash<int, QByteArray> roleNames() const override;
		int m_extruderindex = 0;

	protected:
		us::USettings* m_settings;
		us::USettings *m_searchSettings;
		QList<us::USetting*> m_filterSettings;
		ParameterProfileCategoryModel* m_parameterProfileCategoryModel=nullptr;

	private:
		QString fixFloatValue(QString, us::USetting* setting) const;
		us::USettings* m_global_settings = nullptr;
		us::USettings* m_userSettings = nullptr;
		us::USettings* m_defaultSettings = nullptr;
		QMap<QString, QList<QString> > m_affectedKeys;
	};
}

#endif // CREATIVE_KERNEL_PROFILEPARAMETERMODEL_1676527659531_H
