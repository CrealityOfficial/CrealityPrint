#include "cusaddprintermodel.h"

#include <set>

#include <QtCore/QFileInfo>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QDebug>

#include "interface/appsettinginterface.h"
#include "interface/machineinterface.h"
#include "buildinfo.h"

namespace creative_kernel
{
    CusAddPrinterModel::CusAddPrinterModel(QObject* parent) : QAbstractListModel(parent) {}

    CusAddPrinterModel::~CusAddPrinterModel(){}

    void CusAddPrinterModel::initialize() {

        QString file_path{ DEFAULT_CONFIG_ROOT + QStringLiteral("/") +
      #ifdef CUSTOM_MACHINE_LIST
          QStringLiteral("machineList_custom.json")
      #else
          QStringLiteral("machineList.json")
      #endif // CUSTOM_MACHINE_LIST
        };

        if (!QFileInfo{ file_path }.isFile()) {
            qDebug() << "File isn't exists!";
            return;
        }

        QFile file{ file_path };
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "File open failed!";
            return;
        }

        QJsonParseError error{ QJsonParseError::ParseError::NoError };
        QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error != QJsonParseError::ParseError::NoError) {
            qDebug() << "parseJson:" << error.errorString();
            return;
        }

        file.close();

        type_printer_map_.clear();
        type_name_list_.clear();
        type_printer_name_map_.clear();
        current_type_name_.clear();
        current_printer_name_.clear();
        current_printer_vector_.clear();

        for (const auto& type_printer_ref : document.array()) {
            if (!type_printer_ref.isObject()) {
                continue;
            }

            const auto type_printer = type_printer_ref.toObject();
            if (type_printer.empty()) {
                continue;
            }

            auto type_name = type_printer.value(QStringLiteral("name")).toString();
            if (type_name.isEmpty()) {
                continue;
            }

            const auto printer_array = type_printer.value(QStringLiteral("subnodes")).toArray();
            if (printer_array.empty()) {
                continue;
            }

            decltype(type_printer_map_)::value_type info_pair{ type_name, {} };
            auto& printer_list = type_printer_map_.emplace(std::move(info_pair)).first->second;

            decltype(type_printer_name_map_)::value_type name_pair{ type_name, {} };
            auto& printer_name_list = type_printer_name_map_.emplace(std::move(name_pair)).first->second;

            type_name_list_.push_back(std::move(type_name));

            for (const auto& printer_value : printer_array) {
                if (!printer_value.isObject()) {
                    continue;
                }

                const auto printer = printer_value.toObject();
                if (printer.empty()) {
                    continue;
                }

                auto printer_name = printer.value(QStringLiteral("name")).toString();
                if (printer_name.isEmpty()) {
                    continue;
                }
                auto printer_code_name = printer.value(QStringLiteral("codeName")).toString();
                if (printer.contains("showLevel") && printer.value(QStringLiteral("showLevel")).isString())
                {
                    auto showLevel = printer.value(QStringLiteral("showLevel")).toString();
                    QStringList levels = showLevel.split(" ");
                    bool bShow = false;
                    Q_FOREACH(auto level, levels)
                    {
                        if (QString(PROJECT_VERSION_EXTRA).contains(level))
                        {
                            bShow = true;
                            break;
                        }
                    }
                    if(!bShow)
                    {
                        continue;
                    }
                    
                }

                PrinterInfo printer_info;
                if (printer_name.startsWith("Fast-") || printer_name.startsWith("Nebula-"))
                {
                    printer_info.image_url = QStringLiteral("qrc:/UI/photo/machineImage/machineImage_%1.png").arg(printer_name.mid(printer_name.indexOf("-")+1));
                }
                else
                {
                    printer_info.image_url = QStringLiteral("qrc:/UI/photo/machineImage/machineImage_%1.png").arg(printer_name);
                }

                printer_info.name = printer_name;
                printer_info.code_name = printer_code_name.isEmpty() ? printer_name : printer_code_name;

                printer_list.emplace_back(std::move(printer_info));
                printer_name_list.push_back(std::move(printer_name));
            }
        }

        PrinterInfo defaultInfo;
        const auto& default_name = defaultInfo.name;
        const auto default_depth = QString::number(defaultInfo.depth);
        const auto default_width = QString::number(defaultInfo.width);
        const auto default_height = QString::number(defaultInfo.height);
        const auto default_nozzle_size = QString::number(defaultInfo.nozzle_size);

        for (auto& type_printers_pair : type_printer_map_) {
            for (auto& printer : type_printers_pair.second) {
                QSharedPointer<us::USettings> settings = createDefaultMachineSetting(printer.name);
                printer.depth = settings->value(QStringLiteral("machine_depth"), default_depth).toInt();
                printer.width = settings->value(QStringLiteral("machine_width"), default_width).toInt();
                printer.height = settings->value(QStringLiteral("machine_height"), default_height).toInt();
                printer.nozzle_size = settings->value(QStringLiteral("machine_nozzle_size"), default_nozzle_size).toFloat();
                printer.nozzle_count = settings->value(QStringLiteral("machine_extruder_count"), default_nozzle_size).toInt();

                static const std::set<QString> GRANULAR_PRINTERS{
                    QStringLiteral("Piocreat G5"),
                    QStringLiteral("Piocreat G5 Pro"),
                    QStringLiteral("Piocreat G12"),
                    QStringLiteral("Piocreat G40"),
                    QStringLiteral("Piocreat G50"),
                };

                printer.granular = GRANULAR_PRINTERS.find(
                    settings->value(QStringLiteral("machine_name"), default_name))
                    != GRANULAR_PRINTERS.cend();
            }
        }

        selectPrinter(type_name_list_.empty() ? QString{} : type_name_list_.front());

        qDebug() << "CusAddPrinterModel Series : " << type_name_list_;
    }

    QString CusAddPrinterModel::getCurrentTypeName() {
        return current_type_name_;
    }

    QString CusAddPrinterModel::getCurrentPrinterName() {
        return current_printer_name_;
    }

    QStringList CusAddPrinterModel::getTypeNameList() {
        return type_name_list_;
    }

    QStringList CusAddPrinterModel::getPrinterNameList(const QString& type) {
        auto iter = type_printer_name_map_.find(type);
        if (iter != type_printer_name_map_.cend()) {
            return iter->second;
        }

        return {};
    }

    QString CusAddPrinterModel::getModelNameByCodeName(const QString& codeName)
    {
        for (auto& type_printers_pair : type_printer_map_) {
            for (auto& printer : type_printers_pair.second) {
                if (printer.code_name == codeName)
                {
                    return printer.name;
                }
            }
        }
        return codeName;
    }

    void CusAddPrinterModel::selectPrinter(const QString& type, const QString& printer) {
        auto iter = type_printer_name_map_.find(type);
        if (iter == type_printer_name_map_.cend()) {
            return;
        }

        if (!printer.isEmpty() && !iter->second.contains(printer)) {
            return;
        }

        if (current_type_name_ == type && current_printer_name_ == printer) {
            return;
        }

        if (current_type_name_ != type) {
            current_type_name_ = type;
            Q_EMIT sigCurrentTypeNameChanged(type);
        }

        if (current_printer_name_ != printer) {
            current_printer_name_ = printer;
            Q_EMIT sigCurrentPrinterNameChanged(printer);
        }

        decltype(current_printer_vector_) new_printer_vector{};
        if (current_printer_name_.isEmpty()) {
            for (const auto& printer_info : type_printer_map_.find(type)->second) {
                new_printer_vector.emplace_back(printer_info);
            }
        }
        else {
            for (const auto& printer_info : type_printer_map_.find(type)->second) {
                if (printer_info.name == printer) {
                    new_printer_vector.emplace_back(printer_info);
                    break;
                }
            }
        }

        beginResetModel();
        current_printer_vector_ = std::move(new_printer_vector);
        endResetModel();
    }

    int CusAddPrinterModel::rowCount(const QModelIndex& parent) const {
        std::ignore = parent;
        return current_printer_vector_.size();
    }

    QVariant CusAddPrinterModel::data(const QModelIndex& index, int role) const {
        if (index.row() < 0 || index.row() >= current_printer_vector_.size()) {
            return {};
        }

        QVariant result;
        const auto& printer = current_printer_vector_.at(index.row());
        switch (static_cast<DataRole>(role)) {
        case DataRole::IMAGE_URL:
            result = printer.image_url;
            break;
        case DataRole::NAME:
            result = printer.name;
            break;
        case DataRole::DEPTH:
            result = printer.depth;
            break;
        case DataRole::WIDTH:
            result = printer.width;
            break;
        case DataRole::HEIGHT:
            result = printer.height;
            break;
        case DataRole::NOZZLE_SIZE:
            result = printer.nozzle_size;
            break;
        case DataRole::NOZZLE_COUNT:
            result = printer.nozzle_count;
            break;
        case DataRole::GRANULAR:
            result = printer.granular;
            break;
        case DataRole::ADDED:
            result = false;
            break;
        default:
            break;
        }

        return result;
    }

    QHash<int, QByteArray> CusAddPrinterModel::roleNames() const {
        return {
          { static_cast<int>(DataRole::IMAGE_URL), QByteArrayLiteral("model_image_url") },
          { static_cast<int>(DataRole::NAME), QByteArrayLiteral("model_name") },
          { static_cast<int>(DataRole::DEPTH), QByteArrayLiteral("model_depth") },
          { static_cast<int>(DataRole::WIDTH), QByteArrayLiteral("model_width") },
          { static_cast<int>(DataRole::HEIGHT), QByteArrayLiteral("model_height") },
          { static_cast<int>(DataRole::NOZZLE_COUNT), QByteArrayLiteral("model_nozzle_count") },
          { static_cast<int>(DataRole::NOZZLE_SIZE), QByteArrayLiteral("model_nozzle_size") },
          { static_cast<int>(DataRole::GRANULAR), QByteArrayLiteral("model_granular") },
          { static_cast<int>(DataRole::ADDED), QByteArrayLiteral("model_added") },
        };
    }
}
