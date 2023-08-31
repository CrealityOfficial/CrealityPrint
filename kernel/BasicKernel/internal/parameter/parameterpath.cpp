#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QTextCodec>
#include <fstream>
#include "parameterpath.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/filewritestream.h"

#include <buildinfo.h>

#include "qtusercore/string/resourcesfinder.h"
#include "interface/appsettinginterface.h"
#include "qtusercore/string/resourcesfinder.h"

namespace creative_kernel
{
	QString pathFromPFPT(ParameterFilePathType type)
	{
		QString tails[(int)ParameterFilePathType::pfpt_max_num]{
			"default",
			"default/Machines",
			"default/Extruders",
            "default/Profiles",
            "default/Materials",
            "default/MachineSupportMaterials",
            "Machines",
            "Extruders",
            "Profiles",
            "Materials",
            "MachineSupportMaterials",
		};

		return qtuser_core::getOrCreateAppDataLocation(tails[(int)type]);
	}

    QStringList machine_parameter_keys;
    QStringList extruder_parameter_keys;
    QStringList profile_parameter_keys;
    QStringList material_parameter_keys;
    void loadDefaultKeys()
    {
        QString machine_key_file = QString("%1/keys").arg(_pfpt_default_machines);
        machine_parameter_keys = us::loadKeys(machine_key_file);
        QString extruder_key_file = QString("%1/keys").arg(_pfpt_default_extruders);
        extruder_parameter_keys = us::loadKeys(extruder_key_file);
#ifdef CUSTOM_KEY
		QString profile_key_file = QString("%1/keys_custom").arg(_pfpt_default_profiles);
#else
		QString profile_key_file = QString("%1/keys").arg(_pfpt_default_profiles);
#endif

        profile_parameter_keys = us::loadKeys(profile_key_file);
        QString material_key_file = QString("%1/keys").arg(_pfpt_default_materials);
        material_parameter_keys = us::loadKeys(material_key_file);

        qDebug() << QString("machine_parameter_keys -> [%1]").arg(machine_key_file);
        qDebug() << machine_parameter_keys;
        qDebug() << QString("extruder_parameter_keys -> [%1]").arg(extruder_key_file);
        //qDebug() << extruder_parameter_keys;
        //qDebug() << QString("profile_parameter_keys -> [%1]").arg(profile_key_file);
        //qDebug() << profile_parameter_keys;
        //qDebug() << QString("material_parameter_keys -> [%1]").arg(material_key_file);
        //qDebug() << material_parameter_keys;
    }

    QStringList loadCategoryKeys()
    {
        QString category_file = QString("%1/category").arg(_pfpt_default_profiles);
        QFile file(category_file);
        if (!file.exists())
            category_file = QString("%1/Profiles/category").arg(_qr_default);
        return us::loadKeys(category_file);
    }
    QStringList loadSingleCategoryKeys()
    {
        QString category_file = QString("%1/category_single").arg(_pfpt_default_profiles);
        QFile file(category_file);
        if (!file.exists())
            category_file = QString("%1/Profiles/category_single").arg(_qr_default);
        return us::loadKeys(category_file);
    }

    QStringList loadMaterialKeys()
    {
        QString _file = QString("%1/keys").arg(_pfpt_default_materials);
        QFile file(_file);
        if (!file.exists())
            _file = QString("%1/Materials/keys").arg(_qr_default);
        return us::loadKeys(_file);
    }

    QStringList loadMaterialKeys(const QString& category)
    {
        QString _file = QString("%1/keys-%2").arg(_pfpt_default_materials).arg(category);
        QFile file(_file);
        if (!file.exists())
            _file = QString("%1/Materials/keys-%2").arg(_qr_default).arg(category);
        return us::loadKeys(_file);
    }

    QStringList loadExtruderKeys(const QString& category)
    {
        if (category.isEmpty())
        {
            return extruder_parameter_keys;
        }
        QString _file = QString("%1/keys-%2").arg(_pfpt_default_extruders).arg(category);
        QFile file(_file);
        if (!file.exists())
            _file = QString("%1/Extruders/keys-%2").arg(_qr_default).arg(category);
        return us::loadKeys(_file);
    }

    QStringList loadMachineKeys(const QString& category)
    {
        if (category.isEmpty())
        {
            return machine_parameter_keys;
        }
        QString _file = QString("%1/keys-%2").arg(_pfpt_default_machines).arg(category);
        QFile file(_file);
        if (!file.exists())
            _file = QString("%1/Machines/keys-%2").arg(_qr_default).arg(category);
        return us::loadKeys(_file);
    }

    void loadMachineMeta(QList<MachineMeta>& metas, const bool& user)
    {
        QString file_path;
        if (user)
        {
            file_path = QString("%1/machines_user.json").arg(_pfpt_machines);
        }
        else
        {
            file_path = QString("%1/machines.json").arg(_pfpt_default_machines);

            if (!QFileInfo{ file_path }.isFile()) {
                file_path = QString("%1/Machines/machines.json").arg(_qr_default);
            }
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

        QJsonArray machines = document.object().value(QStringLiteral("machines")).toArray();
        for (const QJsonValueRef& machineRef : machines) {
            if (!machineRef.isObject()) {
                continue;
            }

            MachineMeta meta;
            QJsonObject machine = machineRef.toObject();
            meta.baseName = machine.value(QStringLiteral("name")).toString();
            meta.extruderCount = machine.value(QStringLiteral("extruderCount")).toInt();
            meta.isUser = user;
            QString extruderDiameters = machine.value(QStringLiteral("supportExtruderDiameters")).toString();
            QStringList extruders = extruderDiameters.split(";");
            if (extruders.count() != meta.extruderCount)
            {
                qDebug() << "loadMachineMeta error , extruder mismatch.";
                continue;
            }

            for (const QString& extruder : extruders)
            {
                QList<float> values;
                QStringList diameters = extruder.split(",");
                for (const QString& diameter : diameters)
                    values.append(diameter.toFloat());

                meta.supportExtruderDiameters.append(values);
            }

            QString materialType = machine.value(QStringLiteral("supportMaterialTypes")).toString();
            QStringList materialTypes = materialType.split(",");
            for (QString type : materialTypes)
            {
                meta.supportMaterialTypes.append(type.trimmed());
            }

            QString materialDiameter = machine.value(QStringLiteral("supportMaterialDiameters")).toString();
            QStringList materialDiameters = materialDiameter.split(",");
            for (const QString& diameter : materialDiameters)
                meta.supportMaterialDiameters.append(diameter.toFloat());

            if (machine.contains("supportMaterialNames") && machine.value(QStringLiteral("supportMaterialNames")).isString())
            {
                QString materialName = machine.value(QStringLiteral("supportMaterialNames")).toString();
                meta.supportMaterialNames = materialName.split(",");
            }

            metas.append(std::move(meta));
        }

#if 0 //_DEBUG
        qDebug() << "Machine Meta Data:";
        for (const MachineMeta& meta : metas)
        {
            qDebug() << meta.baseName;
            qDebug() << meta.supportExtruderDiameters;
            qDebug() << meta.supportMaterialDiameters;
            qDebug() << meta.supportMaterialTypes;
        }
#endif
    }

    void saveMachineMeta(const MachineMeta& meta)
    {
        std::string machineListFile = QString("%1/machines_user.json").arg(_pfpt_machines).toStdString();

        QFile file(QString("%1/machines_user.json").arg(_pfpt_machines));
        rapidjson::Document jsonDoc;
        rapidjson::Value machineArray(rapidjson::kArrayType);
        bool fileExist = false;
        if (file.exists())
        {
            FILE* file = fopen(machineListFile.c_str(), "r");

            char read_buffer[10 * 1024] = { 0 };
            rapidjson::FileReadStream reader_stream(file, read_buffer, sizeof(read_buffer));
            jsonDoc.ParseStream(reader_stream);
            fclose(file);
            QString strTemp(read_buffer);
            if (strTemp.contains(meta.baseName))
            {
                return;
            }
            if (!jsonDoc.HasParseError())
            {
                bool res = jsonDoc.HasMember("machines");
                if (res)
                {
                    machineArray = jsonDoc["machines"];
                    fileExist = true;
                }
            }
        }
        jsonDoc.SetObject();

        rapidjson::Document::AllocatorType& allocator = jsonDoc.GetAllocator();

        rapidjson::Value objValue;
        objValue.SetObject();
        objValue.AddMember("name", rapidjson::Value(meta.baseName.toStdString().c_str(), allocator), allocator);
        objValue.AddMember("extruderCount", meta.extruderCount, allocator);
        std::string supportExtruderDiameters;

        for (int i = 0; i < meta.extruderCount; i++)
        {
            for (int j = 0; j < meta.supportExtruderDiameters[i].size(); j++)
            {
                supportExtruderDiameters += std::to_string(meta.supportExtruderDiameters[i][j]);
                if (j < meta.supportExtruderDiameters.size() - 1)
                {
                    supportExtruderDiameters += ",";
                }
            }
            if (i < meta.extruderCount - 1)
            {
                supportExtruderDiameters += ";";
            }
        }

        objValue.AddMember("supportExtruderDiameters", rapidjson::Value(supportExtruderDiameters.c_str(), allocator), allocator);

        std::string supportMaterialTypes;

        for (int j = 0; j < meta.supportMaterialTypes.size(); j++)
        {
            supportMaterialTypes += meta.supportMaterialTypes[j].toStdString();
            if (j < meta.supportMaterialTypes.size() - 1)
            {
                supportMaterialTypes += ",";
            }
        }

        objValue.AddMember("supportMaterialTypes", rapidjson::Value(supportMaterialTypes.c_str(), allocator), allocator);


        std::string supportMaterialDiameters;

        for (int j = 0; j < meta.supportMaterialDiameters.size(); j++)
        {
            supportMaterialDiameters += std::to_string(meta.supportMaterialDiameters[j]);
            if (j < meta.supportMaterialDiameters.size() - 1)
            {
                supportMaterialDiameters += ",";
            }
        }
        objValue.AddMember("supportMaterialDiameters", rapidjson::Value(supportMaterialDiameters.c_str(), allocator), allocator);

        machineArray.PushBack(objValue, allocator);

        if (!fileExist)
        {
            jsonDoc.AddMember("machines", machineArray, allocator);
        }

        rapidjson::StringBuffer strbuf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
        jsonDoc.Accept(writer);

        std::string json(strbuf.GetString(), strbuf.GetSize());

        std::ofstream ofs(machineListFile);
        ofs << json;
    }

    void saveMachineMetaUser(const MachineMeta& meta)
    {
        //1.读取
        QString userMachine = QString("%1/machines_user.json").arg(_pfpt_machines);
        QFile file(userMachine);
        if (!file.open(QFile::ReadWrite | QFile::Text))
        {
            return;
        }

        QTextStream outStream(&file);
        outStream.setCodec("UTF-8");
        QString str = outStream.readAll();
        file.close();
        QJsonDocument qdoc = QJsonDocument::fromJson(str.toUtf8());
        QJsonObject rootObj = qdoc.object();

        QJsonValue qValue = rootObj.value("machines");
        QJsonArray arry;
        if (qValue.type() == QJsonValue::Array)
        {
            for (int index = 0; index < arry.count(); ++index)
            {
                QJsonValue jv = arry.at(index);
                QJsonObject qobj = jv.toObject();
                if (qobj["name"] == meta.baseName)
                {
                    qDebug() << "duplicate name";
                    return;
                }
            }

            arry = qValue.toArray();
        }

        QString supportExtruderDiameters;
        for (int i = 0; i < meta.extruderCount; i++)
        {
            for (int j = 0; j < meta.supportExtruderDiameters[i].size(); j++)
            {
                supportExtruderDiameters += QString::number(meta.supportExtruderDiameters[i][j]);
                if (j < meta.supportExtruderDiameters.size() - 1)
                {
                    supportExtruderDiameters += ",";
                }
            }
            if (i < meta.extruderCount - 1)
            {
                supportExtruderDiameters += ";";
            }
        }

        QString supportMaterialTypes;
        for (int j = 0; j < meta.supportMaterialTypes.size(); j++)
        {
            supportMaterialTypes += meta.supportMaterialTypes[j];
            if (j < meta.supportMaterialTypes.size() - 1)
            {
                supportMaterialTypes += ",";
            }
        }

        QString supportMaterialDiameters;
        for (int j = 0; j < meta.supportMaterialDiameters.size(); j++)
        {
            supportMaterialDiameters += QString::number(meta.supportMaterialDiameters[j]);
            if (j < meta.supportMaterialDiameters.size() - 1)
            {
                supportMaterialDiameters += ",";
            }
        }

        QJsonObject nameObj;
        nameObj["name"] = meta.baseName;
        nameObj["extruderCount"] = meta.extruderCount;
        nameObj["supportExtruderDiameters"] = supportExtruderDiameters;
        nameObj["supportMaterialTypes"] = supportMaterialTypes;
        nameObj["supportMaterialDiameters"] = supportMaterialDiameters;
        arry.append(nameObj);
        rootObj["machines"] = arry;

        //3.重新写入
        QFile inFile(userMachine);
        if (!inFile.open(QFile::WriteOnly | QFile::Text))
        {
            return;
        }
        QTextStream inStream(&inFile);
        inStream.setCodec("UTF-8");

        qdoc.setObject(rootObj);
        QByteArray qba = qdoc.toJson(QJsonDocument::Indented);
        inStream << qba.data();
        inFile.close();
    }

    void removeMachineMetaUser(const QString& machineName)
    {
        //1.读取
        QString userMachine = QString("%1/machines_user.json").arg(_pfpt_machines);
        QFile file(userMachine);
        if (!file.open(QFile::ReadOnly | QFile::Text))
        {
            return;
        }

        QTextStream outStream(&file);
        outStream.setCodec("UTF-8");
        QString str = outStream.readAll();
        file.close();
        QJsonDocument qdoc = QJsonDocument::fromJson(str.toUtf8());
        QJsonObject rootObj = qdoc.object();

        QJsonValue qValue = rootObj.value("machines");
        if (qValue.type() == QJsonValue::Array)
        {
            QJsonArray arry = qValue.toArray();
            for (int index = 0; index < arry.count(); ++index)
            {
                if (arry.at(index).type() == QJsonValue::Object)
                {
                    QJsonValue jValue = arry.at(index);
                    QJsonObject qjObj = jValue.toObject();
                    QString strName = qjObj.value("name").toString();
                    QString strDia = qjObj.value("supportExtruderDiameters").toString();
                    QString temp = strName + "_" + strDia.split(",")[0];
                    if (temp == machineName)
                    {
                        //2.删除
                        arry.removeAt(index);
                        rootObj["machines"] = arry;
                        break;
                    }
                }
            }
        }

        //3.重新写入
        QFile inFile(userMachine);
        if (!inFile.open(QFile::WriteOnly | QFile::Text))
        {
            return;
        }
        QTextStream inStream(&inFile);
        inStream.setCodec("UTF-8");

        qdoc.setObject(rootObj);
        QByteArray qba = qdoc.toJson(QJsonDocument::Indented);
        inStream << qba.data();
        inFile.close();
    }

    void loadMaterialMeta(QList<MaterialMeta>& metas, const bool& user, const QString& machineName)
    {
        QString file_path;
        if (user)
        {
            file_path = QString("%1/%2/materials_user.json").arg(_pfpt_materials).arg(machineName);
        }
        else
        {
#ifdef CUSTOM_MATERIAL_LIST
            constexpr auto FILE_NAME{ "materials_custom.json" };
#else
            constexpr auto FILE_NAME{ "materials.json" };
#endif
            file_path = QStringLiteral("%1/%2").arg(_pfpt_default_materials).arg(FILE_NAME);

            if (!QFileInfo{ file_path }.isFile()) {
#ifdef CUSTOM_MATERIAL_LIST
                file_path = QStringLiteral("%1/Materials/%2").arg(DEFAULT_CONFIG_ROOT).arg(FILE_NAME);
#else
                file_path = QStringLiteral("%1/Materials/%2").arg(_qr_default).arg(FILE_NAME);
#endif
            }
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

		QSet<QString> metaNameCache;
		for (auto m : metas)
			metaNameCache.insert(m.name);

        QJsonArray materials = document.object().value(QStringLiteral("materials")).toArray();
        for (const QJsonValueRef& materialRef : materials) {
            if (!materialRef.isObject()) {
                continue;
            }

			QJsonObject material = materialRef.toObject();
			QString name = material.value(QStringLiteral("name")).toString();
			if (metaNameCache.contains(name)) {		// 避免重复添加
				continue;
			}
			metaNameCache.insert(name);

            MaterialMeta meta;
            meta.name = name;
            meta.type = material.value(QStringLiteral("type")).toString();
            meta.brand = material.value(QStringLiteral("brand")).toString();
            meta.isUserDef = user;

            QString materialDiameter = material.value(QStringLiteral("supportDiameters")).toString();
            QStringList materialDiameters = materialDiameter.split(",");
            for (const QString& diameter : materialDiameters)
                meta.supportDiameters.append(diameter.toFloat());

            metas.append(std::move(meta));
        }

#if 0 //_DEBUG
        qDebug() << "Material Meta Data:";
        for (const MaterialMeta& meta : metas)
        {
            qDebug() << meta.name;
            qDebug() << meta.brand;
            qDebug() << meta.type;
            qDebug() << meta.supportDiameters;
        }
#endif
    }

    void removeUserMaterialFromJson(const QString& materialName, const QString& machineName)
    {
        //1.读取
        QString path = QString("%1/%2/materials_user.json").arg(_pfpt_materials).arg(machineName);
        QFile file(path);
        if (!file.open(QFile::ReadOnly | QFile::Text))
        {
            return;
        }

        QTextStream outStream(&file);
        outStream.setCodec("UTF-8");
        QString str = outStream.readAll();
        file.close();
        QJsonDocument qdoc = QJsonDocument::fromJson(str.toUtf8());
        QJsonObject rootObj = qdoc.object();

        QJsonValue qValue = rootObj.value("materials");
        if (qValue.type() == QJsonValue::Array)
        {
            QJsonArray arry = qValue.toArray();
            for (int index = 0; index < arry.count(); ++index)
            {
                if (arry.at(index).type() == QJsonValue::Object)
                {
                    QJsonValue jValue = arry.at(index);
                    QJsonObject qjObj = jValue.toObject();
                    QString strName = qjObj.value("name").toString();
                    QString strDia = qjObj.value("supportDiameters").toString();
                    QString temp = strName + "_" + strDia.left(4);
                    if (temp == materialName)
                    {
                        //2.删除
                        arry.removeAt(index);
                        rootObj["materials"] = arry;
                        break;
                    }
                }
            }
        }

        //3.重新写入
        QFile inFile(QString("%1/%2/materials_user.json").arg(_pfpt_materials).arg(machineName));
        if (!inFile.open(QFile::WriteOnly | QFile::Text))
        {
            return;
        }
        QTextStream inStream(&inFile);
        inStream.setCodec("UTF-8");

        qdoc.setObject(rootObj);
        QByteArray qba = qdoc.toJson(QJsonDocument::Indented);
        inStream << qba.data();
        inFile.close();
    }

    void reNameUserMaterialFromJson(const QString& materialName, const QString& newMaterialName, const QString& machineName)
    {
        //1.读取
        QString path = QString("%1/%2/materials_user.json").arg(_pfpt_materials).arg(machineName);
        QFile file(path);
        if (!file.open(QFile::ReadOnly | QFile::Text))
        {
            return;
        }

        QTextStream outStream(&file);
        outStream.setCodec("UTF-8");
        QString str = outStream.readAll();
        file.close();
        QJsonDocument qdoc = QJsonDocument::fromJson(str.toUtf8());
        QJsonObject rootObj = qdoc.object();

        QJsonValue qValue = rootObj.value("materials");
        if (qValue.type() == QJsonValue::Array)
        {
            QJsonArray arry = qValue.toArray();
            for (int index = 0; index < arry.count(); ++index)
            {
                if (arry.at(index).type() == QJsonValue::Object)
                {
                    QJsonValue jValue = arry.at(index);
                    QJsonObject qjObj = jValue.toObject();
                    QString strName = qjObj.value("name").toString();
                    QString strDia = qjObj.value("supportDiameters").toString();
                    QString temp = strName + "_" + strDia.left(4);
                    if (temp == materialName)
                    {
                        QJsonObject temp = qjObj;
                        temp.insert("name", QJsonValue(newMaterialName));
                        arry.removeAt(index);
                        arry.insert(index, temp);
                        rootObj["materials"] = arry;
                        break;
                    }
                }
            }
        }

        //3.重新写入
        QFile inFile(QString("%1/%2/materials_user.json").arg(_pfpt_materials).arg(machineName));
        if (!inFile.open(QFile::WriteOnly | QFile::Text))
        {
            return;
        }
        QTextStream inStream(&inFile);
        inStream.setCodec("UTF-8");

        qdoc.setObject(rootObj);
        QByteArray qba = qdoc.toJson(QJsonDocument::Indented);
        inStream << qba.data();
        inFile.close();
    }

    void saveMateriaMeta(const MaterialMeta& meta, const QString& machineName)
    {
        //1.读取
        QString filePath = QString("%1/%2/materials_user.json").arg(_pfpt_materials).arg(machineName);
        QFile file(filePath);
        if (!file.open(QFile::ReadWrite | QFile::Text))
        {
            return;
        }

        QTextStream outStream(&file);
        outStream.setCodec("UTF-8");
        QString str = outStream.readAll();
        file.close();
        QJsonDocument qdoc = QJsonDocument::fromJson(str.toUtf8());
        QJsonObject rootObj = qdoc.object();

        QJsonValue qValue = rootObj.value("materials");
        QJsonArray arry;
        if (qValue.type() == QJsonValue::Array)
        {
            arry = qValue.toArray();
            for (int index = 0; index < arry.count(); ++index)
            {
                QJsonValue jv = arry.at(index);
                QJsonObject qobj = jv.toObject();
                if (qobj["name"] == meta.name)
                    return;
            }

            arry = qValue.toArray();
        }

        QString sdStr = QString("%1").arg(meta.supportDiameters[0]);
        if (meta.supportDiameters.count() > 1)
        {
            for (int index = 1; index < meta.supportDiameters.count(); ++index)
            {
                sdStr += QString(",%1").arg(meta.supportDiameters[index]);
            }
        }

        QJsonObject nameObj;
        nameObj["name"] = meta.name;
        nameObj["type"] = meta.type;
        nameObj["brand"] = meta.brand;
        nameObj["supportDiameters"] = sdStr;
        arry.append(nameObj);
        rootObj["materials"] = arry;

        //3.重新写入
        QFile inFile(QString("%1/%2/materials_user.json").arg(_pfpt_materials).arg(machineName));
        if (!inFile.open(QFile::WriteOnly | QFile::Text))
        {
            return;
        }
        QTextStream inStream(&inFile);
        inStream.setCodec(QTextCodec::codecForName("UTF-8"));

        qdoc.setObject(rootObj);
        QByteArray qba = qdoc.toJson(QJsonDocument::Indented);
        QString qstr = QString::fromLocal8Bit(qba);
        QByteArray ba = qstr.toLocal8Bit();
        inStream << ba;
        inFile.close();
    }

    us::USettings* createDefaultGlobal()
    {
        us::USettings* settings = new us::USettings();
        settings->loadCompleted();
        return settings;
    }

    us::USettings* createSettings(const QString& fileName, const QStringList& defaultKeys)
    {
        us::USettings* settings = new us::USettings();
        settings->loadDefault(fileName);
        settings->appendDefault(defaultKeys);
        return settings;
    }

    us::USettings* createDefaultMachineSettings(const QString& baseMachineName)
    {
        return createSettings(defaultMachineFile(baseMachineName), machine_parameter_keys);
    }

    void makeSureDirectory(const QString& folder, const QString& subFolder)
    {
        QString _folder = QString("/%1/%2").arg(folder).arg(subFolder);
        qtuser_core::getOrCreateAppDataLocation(_folder);
    }

    void removeSubDirectory(const QString& folder, const QString& subFolder)
    {

    }

    QStringList fetchFileNames(const QString& directory)
    {
        QDir dir(directory);
        dir.setFilter(QDir::Files);
        dir.setSorting(QDir::Time | QDir::Reversed);

        QFileInfoList fileInfos = dir.entryInfoList();

        QStringList fileNames;
        for (QFileInfo& fileInfo : fileInfos)
        {
            QString name = fileInfo.fileName();
            if (!fileNames.contains(name) && !name.contains(".cover"))
                fileNames.append(name);
        }
        return fileNames;
    }

    us::USettings* createMachineSettings(const QString& uniqueName)
    {
        makeSureUserMachineFile(uniqueName);
        QString fileName = userMachineFile(uniqueName);

        return createSettings(fileName, machine_parameter_keys);
    }

    us::USettings* createMachineSettingsImport(const QString& uniqueName)
    {
         makeSureUserMachineFile(uniqueName);
        QString fileName = userMachineFile(uniqueName);

        return createSettings(fileName, machine_parameter_keys);
    }

    us::USettings* createUserMachineSettings(const QString& uniqueName)
    {
        QString fileName = userMachineFile(uniqueName,true);

        return createSettings(fileName, QStringList());
    }

	QString defaultMachineFile(const QString& baseMachineName)
	{
		return QString("%1/%2.default").arg(_pfpt_default_machines).arg(baseMachineName);
	}

    QString userMachineFile(const QString& uniqueName, bool cover)
    {
        QString fileName = QString("%1/%2.default").arg(_pfpt_machines).arg(uniqueName);
        if (cover)
        {
            fileName += ".cover";
        }
        return fileName;
    }

    QString uniqueName2BaseMachineName(const QString& uniqueName)
    {
        return uniqueName.split("_").at(0);
    }

	QString uniqueName2ExtruderSize(const QString& uniqueName,int index)
	{
        QString extruderSize = uniqueName.split("_").at(1);
        if (extruderSize.indexOf("-"))//双喷嘴
        {
            return extruderSize.split("-").at(index);
        }
        else
        {
            return extruderSize;
        }
	}

	void makeSureUserMachineFile(const QString& uniqueName, const QString& dstName, bool isSourceMachineUser)
    {
        QString baseMachineName = uniqueName2BaseMachineName(uniqueName);
        QString userFile = userMachineFile(dstName.isEmpty() ? uniqueName : dstName);

        QString defFile;
        if (isSourceMachineUser)
        {
            defFile = userMachineFile(uniqueName);
        }
        else
        {
            defFile = defaultMachineFile(baseMachineName);
        }

        bool res = qtuser_core::copyFile(defFile, userFile, false);
        if (!res)
        {
            qDebug() << "copy file failed!!!";
        }
    }

    void removeUserMachineFile(const QString& uniqueName)
    {
        QString filePath = userMachineFile(uniqueName);
        QString coverFilePath = userMachineFile(uniqueName, true);

        QFile machineFile(filePath);
        if(machineFile.exists())
            machineFile.remove();

        QFile coverMachineFile(coverFilePath);
        if (coverMachineFile.exists())
            coverMachineFile.remove();
    }

    void coverUserMachineFile(const QString& uniqueName)
    {
        QString defaultFile = defaultMachineFile(uniqueName2BaseMachineName(uniqueName));
        QString userFile = userMachineFile(uniqueName);

        if (!QFile::exists(defaultFile))
        {
            qDebug() << QString("coverUserMachineFile [%1] not exist.").arg(defaultFile);
            return;
        }
        qtuser_core::copyFile(defaultFile, userFile);
    }

	QString defaultExtruderFile(const QString& machineName,int index)
	{
		//return QString("%1/extruder_%2.default").arg(_pfpt_default_extruders).arg(index);
        QString baseName = uniqueName2BaseMachineName(machineName);
        QString extruderSize = uniqueName2ExtruderSize(machineName,index);
        QString machineExtruder = QString("%1/%2/extruder_%3_%4.default").arg(_pfpt_default_extruders).arg(baseName).arg(index).arg(extruderSize);
        if (QFile(machineExtruder).exists())//机型独有的喷嘴配置
        {
            return machineExtruder;
        }
        else//通用的喷嘴配置
        {
			return QString("%1/extruder_%3_%4.default").arg(_pfpt_default_extruders).arg(index).arg(extruderSize);
        }

	}

	QString userExtruderFile(const QString& machine, int index, bool cover)
	{
        QString name = QString("%1/%2_extruder_%3.default").arg(_pfpt_extruders).arg(machine).arg(index);
        if (cover)
        {
            name += ".cover";
        }
		return name;
	}

    void makeSureUserExtruderFile(const QString& machineName, int index, const QString& sourceMachineName, bool isSourceMachineUser)
    {
        QString userFile = userExtruderFile(machineName, index);
        QString defFile = defaultExtruderFile(machineName,index);
        if (isSourceMachineUser)
        {
            defFile = defaultExtruderFile(machineName, index);
        }

        qtuser_core::copyFile(defFile, userFile, false);
    }

    void makeSureUserExtruderFileFromUser(const QString& sourceMachineName, const QString& dstMachineName, int index)
    {
        QString userFile = userExtruderFile(dstMachineName, index);
        QString defFile = userExtruderFile(sourceMachineName, index);
        qtuser_core::copyFile(defFile, userFile, false);
    }

    void removeUserExtuderFile(const QString& machineName, int index)
    {
        QString extruderFilePath = userExtruderFile(machineName, index);
        QString coverExtruderFilePath = userExtruderFile(machineName, index, true);

        QFile machineFile(extruderFilePath);
        if (machineFile.exists())
            machineFile.remove();

        QFile coverMachineFile(coverExtruderFilePath);
        if (coverMachineFile.exists())
            coverMachineFile.remove();
    }

    void coverUserExtuderFile(const QString& machineName, int index)
    {
        QString defaultFile = defaultExtruderFile(machineName,index);
        QString userFile = userExtruderFile(machineName, index);

        if (!QFile::exists(defaultFile))
        {
            qDebug() << QString("coverUserExtuderFile [%1] not exist.").arg(machineName);
            return;
        }
        qtuser_core::copyFile(defaultFile, userFile);
    }

    us::USettings* createExtruderSettings(const QString& machineName, int index, bool defaultPath)
    {
        QString fileName = defaultExtruderFile(machineName,index);
        if (!defaultPath)
        {
            makeSureUserExtruderFile(machineName, index);
            fileName = userExtruderFile(machineName, index);
        }

        auto f = [](const QString& fileName)->us::USettings* {
            us::USettings* settings = new us::USettings();
            settings->loadDefault(fileName);
            settings->appendDefault(extruder_parameter_keys);
            return settings;
        };
        return f(fileName);
    }

    us::USettings* createExtruderSettingsFromUser(const QString& machineName, int index, const QString& sourceMachineName, bool defaultPath)
    {
        QString fileName = defaultExtruderFile(machineName, index);
        if (!defaultPath)
        {
            makeSureUserExtruderFileFromUser(sourceMachineName, machineName, index);
            fileName = userExtruderFile(machineName, index);
        }

        auto f = [](const QString& fileName)->us::USettings* {
            us::USettings* settings = new us::USettings();
            settings->loadDefault(fileName);
            settings->appendDefault(extruder_parameter_keys);
            return settings;
        };
        return f(fileName);
    }

    us::USettings* createUserExtruderSettings(const QString& machineName, int index)
    {
        QString fileName = userExtruderFile(machineName, index, true);

        QStringList preKeys;
        return createSettings(fileName, preKeys);
    }

    us::USettings* createUserExtruderSettingsFromUser(const QString& machineName, int index, const QString& sourceMachineName)
    {
        QString fileName = userExtruderFile(machineName, index, true);

        QStringList preKeys;
        return createSettings(fileName, preKeys);
    }

    void copyFile2User(const QString& source, const QString& machineName, const QString& profileName, bool createCover)
    {
        QString dest = userProfileFile(machineName, profileName);
        qtuser_core::copyFile(source, dest, false);
        if (createCover)
        {
            dest = userProfileFile(machineName, profileName, true);
            qtuser_core::copyFile(source, dest, false);
        }
    }

    void copyPrinterFile2User(const QString& source, const QString& machineName, const QString& profileName, bool createCover)
    {
        QString dest = userProfileFile(machineName, profileName);
        qtuser_core::copyFile(source, dest, false);
        if (createCover)
        {
            dest = userProfileFile(machineName, profileName, true);
            qtuser_core::copyFile(source, dest, false);
        }
    }

    void copyMaterialFile2User(const QString& source, const QString& machineName, const QString& profileName, bool createCover)
    {
        QString dest = userProfileFile(machineName, profileName);
        qtuser_core::copyFile(source, dest, false);
        if (createCover)
        {
            dest = userProfileFile(machineName, profileName, true);
            qtuser_core::copyFile(source, dest, false);
        }
    }

    QString defaultProfileFile(const QString& machineName, const QString& profile)
    {
        return QString("%1/%2/%3.default").arg(_pfpt_default_profiles).arg(machineName).arg(profile);
    }

	QString userProfileFile(const QString& machineName, const QString& profile, bool cover, const QString& materialName)
	{
        QString name = QString("%1/%2/%3").arg(_pfpt_profiles).arg(machineName).arg(materialName.isEmpty() ? profile : materialName + "@" + profile);
        if (cover)
            name += ".cover";
        return name;
	}

    void makeSureUserProfileFiles(const QString& machineName)
    {
        QString folder = QString("Profiles/%1").arg(machineName);
        qtuser_core::getOrCreateAppDataLocation(folder);

        QString directory = QString("%1/%2/").arg(_pfpt_profiles).arg(machineName);
        QString defaultFolder = QString("%1/%2").arg(_pfpt_default_profiles).arg(machineName);
        QDir defaultDir(defaultFolder);
        if (defaultDir.exists())
        {
            if (!qtuser_core::copyDir(defaultFolder, directory, false))
            {
                qDebug() << QString("initializeConfig failed ! please check access right ! source: [%1], destination:[%2]")
                    .arg(defaultFolder).arg(directory);
                return;
            }
        }
        else
        {
            QStringList profileNames;
            profileNames << "low.default";
            profileNames << "middle.default";
            profileNames << "high.default";

            for (const QString& profileName : profileNames)
            {
                QString src = QString("%1/%2").arg(_pfpt_default_profiles).arg(profileName);
                QString dest = userProfileFile(machineName, profileName);
                qtuser_core::copyFile(src, dest, false);
            }
        }
    }

    void makeSureUserProfileFiles_(const QString& machineName, const QString& dstMachineName)
    {
        QString folder = QString("Profiles/%1").arg(dstMachineName);
        qtuser_core::getOrCreateAppDataLocation(folder);

        QString directory = QString("%1/%2/").arg(_pfpt_profiles).arg(dstMachineName);
        QString defaultFolder = QString("%1/%2").arg(_pfpt_default_profiles).arg(machineName);
        QDir defaultDir(defaultFolder);
        if (defaultDir.exists())
        {
            if (!qtuser_core::copyDir(defaultFolder, directory, false))
            {
                qWarning() << QString("initializeConfig failed ! please check access right ! source: [%1], destination:[%2]")
                    .arg(defaultFolder).arg(directory);
                return;
            }
        }
        else
        {
            QStringList profileNames;
            profileNames << "low.default";
            profileNames << "middle.default";
            profileNames << "high.default";

            for (const QString& profileName : profileNames)
            {
                QString src = QString("%1/%2").arg(_pfpt_default_profiles).arg(profileName);
                QString dest = userProfileFile(dstMachineName, profileName);
                qtuser_core::copyFile(src, dest, false);
            }
        }
    }

    void makeUserDefProfileFile(const QString& machineName, const QString& profileName, const QString& copyProfile)
    {
        QString copyProfileName = copyProfile.isEmpty() ? defaultProfileFile(uniqueName2BaseMachineName(machineName), "middle.default")
            : userProfileFile(machineName, copyProfile);
        QString newProfileName = userProfileFile(machineName, profileName);
        QString coverProfileName = userProfileFile(machineName, profileName, true);
        qtuser_core::copyFile(copyProfileName, newProfileName);
        qtuser_core::copyFile(coverProfileName, coverProfileName);
    }

    QStringList fetchUserProfileNames(const QString& machineName, const QString& dstMachineName)
    {
        if (dstMachineName == QString())
        {
            makeSureUserProfileFiles(machineName);
            return fetchFileNames(QString("%1/%2").arg(_pfpt_profiles).arg(machineName));
        }
        else {
            makeSureUserProfileFiles_(machineName, dstMachineName);
            return fetchFileNames(QString("%1/%2").arg(_pfpt_profiles).arg(dstMachineName));
        }
    }

    void removeUserProfileFile(const QString& machineName, const QString& fileName, const QString& materialName)
    {
        QFile profileFile(userProfileFile(machineName, fileName));
        profileFile.remove();
        QFile coverFile(userProfileFile(machineName, fileName, true, materialName));
        coverFile.remove();
    }

    us::USettings* createUserProfileSettings(const QString& machineName, const QString& profileName)
    {
        QString fileName = userProfileFile(machineName, profileName);
        return createSettings(fileName, profile_parameter_keys);
    }
    us::USettings* createUserProfileCoverSettings(const QString& machineName, const QString& profileName, const QString& materialName)
    {
        QString fileName = userProfileFile(machineName, profileName,true, materialName);
        QStringList preKeys;
        return createSettings(fileName, preKeys);
    }

    QString defaultMachineSupportMaterialFile(const QString& machineName)
    {
        QString defFile = QString("%1/%2.default").arg(_pfpt_default_machine_support_materials).arg(machineName);
        QFile file(defFile);
        if (!file.exists())
            defFile = QString("%1/support.default").arg(_pfpt_default_machine_support_materials);
        return defFile;
    }

    QString userMachineSupportMaterialFile(const QString& machineName)
    {
        return QString("%1/%2.default").arg(_pfpt_machine_support_materials).arg(machineName);
    }

    void makeSureUserMachineSupportMaterialFile(const QString& machineName)
    {
        QString defFile = defaultMachineSupportMaterialFile(machineName);
        QString userFile = userMachineSupportMaterialFile(machineName);
        qtuser_core::copyFile(defFile, userFile, false);
    }

    void removeUserMachineSupportMaterialFile(const QString& machineName)
    {

    }

    void coverUserMachineSupportMaterialFile(const QString& machineName)
    {

    }

    QStringList fetchUserMachineSupportMaterialNames(const QString& machineName)
    {
        makeSureUserMachineSupportMaterialFile(machineName);
        QString fileName = userMachineSupportMaterialFile(machineName);

        return us::loadKeys(fileName);
    }

    QString userMaterialFile(const QString& uniqueName, const QString& materialName, bool cover)
    {
        QString folder = QString("Materials/%1").arg(uniqueName);
        qtuser_core::getOrCreateAppDataLocation(folder);

        QString name = QString("%1/%2/%3.default").arg(_pfpt_materials).arg(uniqueName).arg(materialName);
        if (cover)
            name += ".cover";
        return name;
    }

    QString defaultMaterialFile(const QString& materialName)
    {
        return QString("%1/%2.default").arg(_pfpt_default_materials).arg(materialName);
    }

    void makeSureUserMaterialFiles(const QString& uniqueName)
    {
        QString folder = QString("Materials/%1").arg(uniqueName);
        qtuser_core::getOrCreateAppDataLocation(folder);

        QString directory = QString("%1/%2/").arg(_pfpt_materials).arg(uniqueName);
        QString defaultFolder = QString("%1/%2").arg(_pfpt_default_materials).arg(uniqueName2BaseMachineName(uniqueName));
        QDir defaultDir(defaultFolder);
        if (defaultDir.exists())
        {
            if (!qtuser_core::copyDir(defaultFolder, directory, false))
            {
                qDebug() << QString("initializeConfig failed ! please check access right ! source: [%1], destination:[%2]")
                    .arg(defaultFolder).arg(directory);
                return;
            }
        }
        else
        {
            QStringList profileNames;
            profileNames << "PLA-1.75.default";
            profileNames << "PLA-2.85.default";

            for (const QString& profileName : profileNames)
            {
                QString src = QString("%1/%2").arg(_pfpt_default_materials).arg(profileName);
                QString dest = userMaterialFile(uniqueName, profileName);
                qtuser_core::copyFile(src, dest, false);
            }
        }
    }

    QStringList fetchUserMaterialNames(const QString& uniqueName)
    {
        makeSureUserMaterialFiles(uniqueName);
        return fetchFileNames(QString("%1/%2").arg(_pfpt_materials).arg(uniqueName));
    }

    void makeSureUserMaterialFile(const QString& uniqueName, const MaterialData& data)
    {
        QString fileName = userMaterialFile(uniqueName, data.uniqueName());
        QFile file(fileName);
        if (file.exists())
            return;

        QString defFileName = QString("%1/%2/%3_%4.default").arg(_pfpt_default_materials)
            .arg(uniqueName2BaseMachineName(uniqueName)).arg(data.name).arg(data.diameter);

        QFile defFile(defFileName);
        if (defFile.exists())
        {
            qtuser_core::copyFile(defFileName, fileName, true);
            return;
        }

        defFileName = QString("%1/%2_%3.default").arg(_pfpt_default_materials).arg(data.name).arg(data.diameter);
        QFile defFile2(defFileName);
        if (defFile2.exists())
        {
            qtuser_core::copyFile(defFileName, fileName, true);
            return;
        }

        defFileName = QString("%1/%2-%3.default").arg(_pfpt_default_materials).arg(data.type).arg(data.diameter);

        QFile defFile1(defFileName);
        if (defFile1.exists())
        {
            qtuser_core::copyFile(defFileName, fileName, true);
            return;
        }

        qDebug() << QString("makeSureUserMaterialFile error. [%1] [%2]").arg(uniqueName).arg(data.name);
    }

    us::USettings* createMaterialSettings(const QString& uniqueName, const MaterialData& data, bool cover)
    {
        makeSureUserMaterialFile(uniqueName, data);
        QString fileName = userMaterialFile(uniqueName, data.uniqueName(), cover);

        return createSettings(fileName, material_parameter_keys);
    }
    us::USettings* createUserMaterialSettings(const QString& uniqueName, const MaterialData& data)
    {
        QString fileName = userMaterialFile(uniqueName, data.uniqueName(),true);
        QStringList preKeys;
        return createSettings(fileName, preKeys);
    }

    us::USettings* createMaterialOverrideSettings(const QString& uniqueName, const MaterialData& data, const QString& quality)
    {
        QString defFileName = QString("%1/%2/%5/%3_%4.default.cover").arg(_pfpt_default_materials)
            .arg(uniqueName2BaseMachineName(uniqueName)).arg(data.name).arg(data.diameter).arg(quality);
        if (!QFile::exists(defFileName))
        {
            return nullptr;
        }
        QString filePath = QString("%1/%2/%3").arg(_pfpt_materials).arg(uniqueName).arg(quality);
        QString fileName = QString("%1/%2.default.cover").arg(filePath).arg(data.uniqueName());
        if (!QFile::exists(fileName))
        {
            QDir dir(filePath);
            if (!dir.exists())
            {
                dir.mkdir(filePath);
            }
            QFile::copy(defFileName, fileName);
        }
        return createSettings(fileName);
    }

}
