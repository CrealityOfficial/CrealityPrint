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
#include "internal/parameter/parameterpath.h"
#include "external/kernel/kernel.h"
#include <QCoreApplication>
namespace creative_kernel
{
    CusAddPrinterModel::CusAddPrinterModel(QObject* parent) : QAbstractListModel(parent) {}

    CusAddPrinterModel::~CusAddPrinterModel() {}

    void CusAddPrinterModel::initialize() {
        QString file_path{ _pfpt_default_root + QStringLiteral("/") +
#ifdef CUSTOM_MACHINE_LIST
          QStringLiteral("machineList_custom.json")
#else
          QStringLiteral("machineList.json")
#endif // CUSTOM_MACHINE_LIST
        };

        qDebug() << "machineList == " << file_path;

        QFile file{ file_path };
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "machineList open failed!" << file.errorString();
            return;
        }

        QJsonParseError error{ QJsonParseError::ParseError::NoError };
        QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error != QJsonParseError::ParseError::NoError) {
            qWarning() << "machineList parseJson:" << error.errorString();
            file.close();
            return;
        }

        file.close();

        type_printer_map_.clear();
        type_name_list_.clear();
        printer_name_list_.clear();
        type_printer_name_map_.clear();
        current_type_name_.clear();
        current_printer_name_.clear();
        current_printer_vector_.clear();

        const auto& rootObj = document.object();
        std::map<int, QString> printer_series;

        if (rootObj.contains("series") && rootObj.value(QStringLiteral("series")).isArray())
        {
            const auto& seriesArray = rootObj.value(QStringLiteral("series")).toArray();
            for (const auto& series : seriesArray)
            {
                auto seriesObj = series.toObject();
                auto type = seriesObj.value(QStringLiteral("type")).toInt();
                if (type != 1)
                {
                    continue;
                }
                auto name = seriesObj.value(QStringLiteral("name")).toString();
                auto id = seriesObj.value(QStringLiteral("id")).toInt();
                decltype(type_printer_map_)::value_type info_pair{ name, {} };
                type_printer_map_.emplace(std::move(info_pair));
                decltype(type_printer_name_map_)::value_type name_pair{ name, {} };
                type_printer_name_map_.emplace(std::move(name_pair));
                printer_series[id] = name;
                type_name_list_.push_back(name);
            }
        }

        if (rootObj.contains("printerList") && rootObj.value(QStringLiteral("printerList")).isArray())
        {
            const auto& printerArray = rootObj.value(QStringLiteral("printerList")).toArray();
            for (const auto& printer : printerArray)
            {
                auto printerObj = printer.toObject();
                PrinterInfo printer_info;
                printer_info.name = printerObj.value(QStringLiteral("name")).toString();
                printer_info.code_name = printerObj.value(QStringLiteral("printerIntName")).toString();
                printer_info.height = printerObj.value(QStringLiteral("zSize")).toInt();
                printer_info.width = printerObj.value(QStringLiteral("xSize")).toInt();
                printer_info.depth = printerObj.value(QStringLiteral("ySize")).toInt();
                printer_info.nozzle_count = printerObj.value(QStringLiteral("nozzleDiameter")).toArray().size();
                if (printer_info.name.startsWith("Fast-"))
                {
                    printer_info.image_url = QStringLiteral("qrc:/UI/photo/machineImage/machineImage_%1.png").arg(printer_info.name.mid(5));
                }
                else
                {
                    printer_info.image_url = QStringLiteral("qrc:/UI/photo/machineImage/machineImage_%1.png").arg(printer_info.name);
                }
                QVariantList nozzleDiameters = printerObj.value(QStringLiteral("nozzleDiameter")).toArray().toVariantList();
                QString nozzleDiameterName = "";
                for(auto nozzleDiameter : nozzleDiameters)
                {
                    nozzleDiameterName +=nozzleDiameter.toString()+"+";
                }
                nozzleDiameterName = nozzleDiameterName.mid(0,nozzleDiameterName.size()-1);
                printer_info.image_url = QString(QStringLiteral("%1/machineImages/%2.png")).arg(_pfpt_default_root).arg(printer_info.code_name);
                QFileInfo fileInfo(printer_info.image_url);
                if (!fileInfo.exists())
                {
                    printer_info.image_url = printerObj.value(QStringLiteral("thumbnail")).toString();
                }
                else {
                    printer_info.image_url = QUrl::fromLocalFile(printer_info.image_url).toString();
                }
                const auto& seriesId = printerObj.value(QStringLiteral("seriesId")).toInt();
                const auto & printer_infoes = type_printer_map_[printer_series[seriesId]];
                bool found = false;
                for (const auto printer : type_printer_map_[printer_series[seriesId]])
                {
                    if (printer.name == printer_info.name)
                    {
                        found = true;
                    }
                }
                if (!found)
                {
                    type_printer_map_[printer_series[seriesId]].emplace_back(printer_info);
                }
                if (!type_printer_name_map_[printer_series[seriesId]].contains(printer_info.name))
                {
                    type_printer_name_map_[printer_series[seriesId]].push_back(printer_info.name);
                    printer_name_list_.push_back(printer_info.name);
                }
                //if (std::find(printer_infoes.cbegin(), printer_infoes.cend(), [=](const PrinterInfo& left, const PrinterInfo& right) { return true; }) != printer_infoes.cend())
                //{
                //    type_printer_map_[printer_series[seriesId]].emplace_back(printer_info);
                //}
            }
        }

        QMutableStringListIterator i(type_name_list_); // pass list as argument
        while (i.hasNext()) {
            int size = getPrinterNameList(i.next()).size();
            if (size == 0) {
                i.remove();
            }
        }

        selectPrinter(type_name_list_.empty() ? QString{} : type_name_list_.front());
        Q_EMIT sigTypeNameListChanged(type_name_list_);

        qDebug() << "CusAddPrinterModel Series : " << type_name_list_;
    }

    void CusAddPrinterModel::reinitialize() {
        beginResetModel();
        initialize();
        endResetModel();
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

    QStringList CusAddPrinterModel::getAllPrinterNameList() {
        return printer_name_list_;
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
    QString CusAddPrinterModel::getThumbByCodeName(const QString& printerModel)
    {
        QString codeName = printerModel;
        if (printerModel == "K1" || printerModel == "K1 Max")
	    {
		    codeName = "CR-"+printerModel;
	    }
        for (auto& type_printers_pair : type_printer_map_) {
            for (auto& printer : type_printers_pair.second) {
                if (printer.code_name == codeName)
                {
                    return printer.image_url;
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
        case DataRole::CODENAME:
            result = printer.code_name;
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
          { static_cast<int>(DataRole::CODENAME), QByteArrayLiteral("model_code_name") },
          { static_cast<int>(DataRole::DEPTH), QByteArrayLiteral("model_depth") },
          { static_cast<int>(DataRole::WIDTH), QByteArrayLiteral("model_width") },
          { static_cast<int>(DataRole::HEIGHT), QByteArrayLiteral("model_height") },
          { static_cast<int>(DataRole::NOZZLE_COUNT), QByteArrayLiteral("model_nozzle_count") },
          { static_cast<int>(DataRole::GRANULAR), QByteArrayLiteral("model_granular") },
          { static_cast<int>(DataRole::ADDED), QByteArrayLiteral("model_added") },
        };
    }
}
