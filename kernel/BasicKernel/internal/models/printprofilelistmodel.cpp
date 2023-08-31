#include "printprofilelistmodel.h"
#include "internal/parameter/printmachine.h"
#include "internal/parameter/printprofile.h"
#include "internal/parameter/printmaterial.h"
#include "internal/parameter/printextruder.h"

namespace creative_kernel
{
    enum {
        NameRole = Qt::UserRole + 1,
        DespRole,
        LayerRole,
        MaterialRole,
        QualityRole,
        QualityImg,
        ISDefaultRole,
        line_width,
        wall_line_count,
        infill_sparse_density,
        adhesion_type,
        material_print_temperature,
        material_bed_temperature,
        speed_print,
        speed_inner,
        speed_outer
    };

    PrintProfileListModel::PrintProfileListModel(PrintMachine* machine, QObject* parent)
        : QAbstractListModel(parent)
        , m_machine(machine)
    {
    }

    PrintProfileListModel::~PrintProfileListModel()
    {
    }

    void PrintProfileListModel::notifyReset()
    {
        beginResetModel();
        endResetModel();
    }

    int PrintProfileListModel::rowCount(const QModelIndex& parent) const
    {
        if (!m_machine)
            return 0;

        return m_machine->profilesCount();
    }

    QVariant PrintProfileListModel::getProfileValue(PrintProfile* profile, const QString& key, const QVariant& defaultValue) const
    {
        if (profile->userSettings()->hasKey(key))
        {
            return profile->userSettings()->vvalue(key, defaultValue);
        }
        else
        {
            return profile->settings()->vvalue(key, defaultValue);
        }
    }

    bool PrintProfileListModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        return true;
    }

    QHash<int, QByteArray> PrintProfileListModel::roleNames() const
    {
        QHash<int, QByteArray> roles;
        roles[NameRole] = "profileName";
        roles[DespRole] = "profileTips";
        roles[LayerRole] = "profileLayerHeight";
        roles[MaterialRole] = "profileMaterial";
        roles[QualityRole] = "profileQuality";
        roles[QualityImg] = "profileQualityImg";
        roles[ISDefaultRole] = "profileIsDefault";
        roles[line_width] = "profileLineWidth";
        roles[wall_line_count] = "profileWallLineCount";
        roles[infill_sparse_density] = "profileInfillSparseDensity";
        roles[adhesion_type] = "profileAdhesionType";
        roles[material_print_temperature] = "profileMaterialPrintTemperature";
        roles[material_bed_temperature] = "profileMaterialBedTemperature";
        roles[speed_print] = "profileSpeedPrint";
        roles[speed_inner] = "profileSpeedInner";
        roles[speed_outer] = "profileSpeedOuter";

        return roles;
    }

    QVariant PrintProfileListModel::data(const QModelIndex& index, int role) const
    {
        if(!m_machine)
            return QVariant(QVariant::Invalid);

        int i = index.row();
        if (i < 0 || i >= m_machine->profilesCount())
            return QVariant(QVariant::Invalid);

        PrintProfile* profile = m_machine->profile(i);
        if (!profile)
            return QVariant(QVariant::Invalid);

        us::USettings* settings = profile->settings();
        us::USettings* userSetting = profile->userSettings();
        if (role == NameRole)
        {
            QString name = profile->name();
        
            if (name == "high") name = ("High Quality");
            else if (name == "middle") name = ("Quality");
            else if (name == "low")name = ("Normal");
            else if (name == "best")name = ("Best quality");
            else if (name == "fast")name = ("Fast");
            return QVariant::fromValue(name);
        }
        if (role == ISDefaultRole)
        {
            return QVariant::fromValue(profile->isDefault());
        }
        if (settings)
        {
            if (role == DespRole)
            {
                return QVariant::fromValue(QString(""));
            }
            if (role == LayerRole)
            {
                return getProfileValue(profile, "layer_height", "0.0").toDouble();
            }
            if (role == MaterialRole)
            {
                return QVariant::fromValue(QString("PLA"));
            }
            if (role == QualityRole)
            {
                return QVariant::fromValue(QString(""));
            }
            if (role == line_width)
            {
                double line_width = 0.0;
                if (userSetting->hasKey("line_width"))
                {
                    return userSetting->vvalue("line_width", "0.0").toDouble();
                }
                for (int i = 0; i < m_machine->extruderCount(); i++)
                {
                    PrintExtruder* extruder = qobject_cast<PrintExtruder*>(m_machine->extruderObject(i));
                    if (!extruder)
                    {
                        continue;
                    }
                    if (extruder->userSettings()->hasKey("line_width"))
                    {
                        line_width = extruder->userSettings()->vvalue("line_width", "0.0").toDouble();
                    }
                    else
                    {
                        line_width = extruder->settings()->vvalue("line_width", "0.0").toDouble();
                    }
                }
                return line_width;
            }
            if (role == wall_line_count)
            {
                return getProfileValue(profile, "wall_line_count", "0.0").toDouble();
            }
            if (role == infill_sparse_density)
            {
                return getProfileValue(profile, "infill_sparse_density", "0.0").toDouble();
            }
            if (role == adhesion_type)
            {
                return getProfileValue(profile, "adhesion_type", "");
            }
            if (role == material_print_temperature || role == material_bed_temperature)
            {
                double print_temperature = 0.0;
                double bed_temperature = 0.0;
                for (int i = 0; i < m_machine->extruderCount(); i++)
                {
                    int materialIndex = m_machine->extruderMaterialIndex(i);
                    if (materialIndex == -1)
                        continue;
                    QString materialName = m_machine->extruderMaterialName(i);
                    PrintMaterial* material = qobject_cast<PrintMaterial*>(m_machine->materialObject(materialName));
                    if (!material)
                    {
                        continue;
                    }
                    if (material->userSettings()->hasKey("material_print_temperature"))
                    {
                        print_temperature = material->userSettings()->vvalue("material_print_temperature", "0.0").toDouble();
                    }
                    else
                    {
                        print_temperature = material->settings()->vvalue("material_print_temperature", "0.0").toDouble();
                    }
                    if (material->userSettings()->hasKey("material_bed_temperature"))
                    {
                        bed_temperature = material->userSettings()->vvalue("material_bed_temperature", "0.0").toDouble();
                    }
                    else
                    {
                        bed_temperature = material->settings()->vvalue("material_bed_temperature", "0.0").toDouble();
                    }
                }
                if (role == material_bed_temperature)
                {
                    if (userSetting->hasKey("material_bed_temperature"))
                    {
                        return userSetting->vvalue("material_bed_temperature", "0.0").toDouble();
                    }
                    return bed_temperature;
                }
                if (role == material_print_temperature)
                {
                    if (userSetting->hasKey("material_print_temperature"))
                    {
                        return userSetting->vvalue("material_print_temperature", "0.0").toDouble();
                    }
                    return print_temperature;
                }
            }
            if (role == speed_print)
            {
                return getProfileValue(profile, "speed_print", "0.0").toDouble();
            }
            if (role == speed_inner)
            {
                return getProfileValue(profile, "speed_wall_0", "0.0").toDouble();
            }
            if (role == speed_outer)
            {
                return getProfileValue(profile, "speed_wall_x", "0.0").toDouble();
            }
        }

        return QVariant(QVariant::Invalid);
    }
}