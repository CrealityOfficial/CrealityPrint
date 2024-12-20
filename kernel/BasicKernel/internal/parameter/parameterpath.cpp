#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QTextCodec>
#include <fstream>
#include "parameterpath.h"
#include "us/settingdef.h"

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/filewritestream.h"

#include <buildinfo.h>

#include "qtusercore/string/resourcesfinder.h"
#include "interface/appsettinginterface.h"
#include "qtusercore/string/resourcesfinder.h"
#include <external/us/defaultloader.h>
#include "qtusercore/module/systemutil.h"
#include <QCoreApplication>
#include "interface/projectinterface.h"
namespace creative_kernel
{
	QString pathFromPFPT(ParameterFilePathType type)
	{
		QString tails[(int)ParameterFilePathType::pfpt_max_num]{
			"default",
            "default/base",
            "default/keys",
            "default/ui",
            "default/parampack",
            "Machines",
            "Extruders",
            "Profiles",
            "Materials",
            "user",
            "user/parampack",
		};

		return qtuser_core::getOrCreateAppDataLocation(getEnginePathPrefix() + tails[(int)type]);
	}

    QStringList machine_parameter_keys;
    QStringList extruder_parameter_keys;
    QStringList profile_parameter_keys;
    QStringList material_parameter_keys;
    void loadDefaultKeys()
    {
#if 1
        us::loadMetaMachineKeys(machine_parameter_keys);
        us::loadMetaExtruderKeys(extruder_parameter_keys);
        us::loadMetaProfileKeys(profile_parameter_keys);
        us::loadMetaMaterialKeys(material_parameter_keys);
#else
        QString machine_key_file = QString("%1/keys").arg(_pfpt_default_machines);
        machine_parameter_keys = us::loadKeys(machine_key_file);
        QString extruder_key_file = QString("%1/extruder_keys.json").arg(_pfpt_def(pfpt_default_keys));
        extruder_parameter_keys = us::loadKeys(extruder_key_file);
#ifdef CUSTOM_KEY
		QString profile_key_file = QString("%1/keys_custom").arg(_pfpt_default_profiles);
#else
		QString profile_key_file = QString("%1/profile_keys.json").arg(_pfpt_def(pfpt_default_keys));
#endif

        profile_parameter_keys = us::loadKeys(profile_key_file);
        QString material_key_file = QString("%1/material_keys.json").arg(_pfpt_def(pfpt_default_keys));
        material_parameter_keys = us::loadKeys(material_key_file);
#endif
    }

    QStringList getMetaMachineKeys()
    {
        return machine_parameter_keys;
    }

    QStringList getMetaExtruderKeys()
    {
        return extruder_parameter_keys;
    }

    QStringList getMetaProfileKeys()
    {
        return profile_parameter_keys;
    }

    QStringList getMetaMaterialKeys()
    {
        return material_parameter_keys;
    }

    void loadMachineMeta(QList<MachineData>& metas, const bool& user)
    {
        QString file_path;
        if (user)
        {
            file_path = QString("%1/machineList.json").arg(_pfpt_user_root);
        }
        else
        {
            file_path = QString("%1/machineList.json").arg(_pfpt_default_root);
        }

        QFile file{ file_path };
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "loadMachineMeta File open failed!";
            file.setFileName(QCoreApplication::applicationDirPath() + QStringLiteral("/resources/sliceconfig/orca/default/machineList.json"));
            if (!file.open(QIODevice::ReadOnly)) {
                {
                    qWarning() << "machineList open2 failed!" << file.errorString();
                    return;
                }
            }
            return;
        }
        QJsonParseError error{ QJsonParseError::ParseError::NoError };
        QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error != QJsonParseError::ParseError::NoError) {
            qDebug() << "parseJson:" << error.errorString();
            return;
        }

        file.close();

        const auto& rootObj = document.object();
        if (rootObj.contains("printerList") && rootObj.value(QStringLiteral("printerList")).isArray())
        {
            const auto& printerArray = rootObj.value(QStringLiteral("printerList")).toArray();
            for (const auto& printer : printerArray)
            {
                auto printerObj = printer.toObject();

                MachineData meta;
                meta.baseName = printerObj.value(QStringLiteral("name")).toString();
                meta.codeName = printerObj.value(QStringLiteral("printerIntName")).toString();
                meta.extruderCount = printerObj.value(QStringLiteral("nozzleDiameter")).toArray().size();
                meta.isUser = user;
                const auto& materialArray = printerObj.value(QStringLiteral("supMaterialType")).toArray();
                for (const auto& material : materialArray)
                {
                    auto materialObj = material.toString();
                    meta.supportMaterialTypes.append(materialObj);
                }

                for (int i = 0; i < meta.extruderCount; ++i)
                {
                    const auto& nozzleDiameters = printerObj.value(QStringLiteral("nozzleDiameter")).toArray();
                    float nozzleDiameter = nozzleDiameters[i].toString().toFloat();
                    meta.extruderDiameters.append(nozzleDiameter);
                }
                metas.append(meta);
            }
        }
    }

    void saveMachineMetaUser(const MachineData& meta)
    {
        //1.��ȡ
        QString userMachine = QString("%1/machineList.json").arg(_pfpt_user_root);
        QFile file(userMachine);
        if (!file.open(QFile::ReadWrite | QFile::Text))
        {
            return;
        }

        QString str = file.readAll();
        file.close();
        QJsonDocument qdoc = QJsonDocument::fromJson(str.toUtf8());
        QJsonObject rootObj = qdoc.object();

        QJsonValue qValue = rootObj.value("printerList");
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

        QJsonArray nozzleDiameterArray;
        for (int i = 0; i < meta.extruderCount; i++)
        {
            nozzleDiameterArray.append(QString::number(meta.extruderDiameters[i], 'f', 1));
        }

        QJsonArray supMaterialTypeArray;
        for (int j = 0; j < meta.supportMaterialTypes.size(); j++)
        {
            supMaterialTypeArray.append(meta.supportMaterialTypes[j]);
        }

        QJsonObject nameObj;
        nameObj["name"] = meta.baseName;
        nameObj["printerIntName"] = meta.codeName;
        nameObj["nozzleDiameter"] = nozzleDiameterArray;
        nameObj["supMaterialType"] = supMaterialTypeArray;
        arry.append(nameObj);
        rootObj["printerList"] = arry;

        //3.����д��
        QFile inFile(userMachine);
        if (!inFile.open(QFile::WriteOnly | QFile::Text))
        {
            return;
        }

        qdoc.setObject(rootObj);
        QByteArray qba = qdoc.toJson(QJsonDocument::Indented);
        inFile.write(qba);
        inFile.close();
    }

    void removeMachineMetaUser(const QString& machineName)
    {
        //1.��ȡ
        QString userMachine = QString("%1/machineList.json").arg(_pfpt_user_root);
        QFile file(userMachine);
        if (!file.open(QFile::ReadOnly | QFile::Text))
        {
            return;
        }

        QString str = file.readAll();
        file.close();
        QJsonDocument qdoc = QJsonDocument::fromJson(str.toUtf8());
        QJsonObject rootObj = qdoc.object();

        QJsonValue qValue = rootObj.value("printerList");
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
                    QJsonArray strDia = qjObj.value("nozzleDiameter").toArray();

                    QString temp = strName;
                    for (int i = 0; i < strDia.count(); ++i)
                    {
                        const auto& nozzleDiameter = strDia.at(i).toString();
                        temp = temp + "-" + nozzleDiameter;
                    }
                    if (temp == machineName)
                    {
                        arry.removeAt(index);
                        rootObj["printerList"] = arry;
                        break;
                    }
                }
            }
        }

        //3.����д��
        QFile inFile(userMachine);
        if (!inFile.open(QFile::WriteOnly | QFile::Text))
        {
            return;
        }
        qdoc.setObject(rootObj);
        QByteArray qba = qdoc.toJson(QJsonDocument::Indented);
        inFile.write(qba);
        inFile.close();
    }

    void loadMaterialMeta(QList<MaterialData>& metas, const bool& user, const QString& machineName)
    {
        QString file_path;
        if (user)
        {
            file_path = QString("%1/%2/materials_user.json").arg(_pfpt_user_parampack).arg(machineName);
        }
        else
        {
#ifdef CUSTOM_MATERIAL_LIST
            constexpr auto FILE_NAME{ "materials_custom.json" };
#else
            constexpr auto FILE_NAME{ "materialList.json" };
#endif
            file_path = QStringLiteral("%1/%2").arg(_pfpt_default_root).arg(FILE_NAME);

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
			if (metaNameCache.contains(name)) {		// �����ظ�����
				continue;
			}
			metaNameCache.insert(name);

            MaterialData meta;
            meta.name = name;
            meta.id = material.value(QStringLiteral("id")).toString();
            meta.type = material.value(QStringLiteral("type")).toString();
            meta.brand = material.value(QStringLiteral("brand")).toString();
            meta.rank = material.value(QStringLiteral("rank")).toInt();
            meta.isUserDef = user;

            QString materialDiameter = material.value(QStringLiteral("supportDiameters")).toString();
            meta.diameter = materialDiameter.toFloat();

            metas.append(std::move(meta));
        }

#if 0 //_DEBUG
        qDebug() << "Material Meta Data:";
        for (const MaterialData& meta : metas)
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
        //1.��ȡ
        QString path = QString("%1/%2/materials_user.json").arg(_pfpt_user_parampack).arg(machineName);
        QFile file(path);
        if (!file.open(QFile::ReadOnly | QFile::Text))
        {
            return;
        }

        QString str = file.readAll();
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

                    QString temp = strName;
                    if (!strName.contains("-" + strDia.left(4)))
                    {
                        temp += "-" + strDia.left(4);
                    }

                    if (temp == materialName)
                    {
                        //2.ɾ��
                        arry.removeAt(index);
                        rootObj["materials"] = arry;
                        break;
                    }
                }
            }
        }

        //3.����д��
        QFile inFile(QString("%1/%2/materials_user.json").arg(_pfpt_user_parampack).arg(machineName));
        if (!inFile.open(QFile::WriteOnly | QFile::Text))
        {
            return;
        }

        qdoc.setObject(rootObj);
        QByteArray qba = qdoc.toJson(QJsonDocument::Indented);
        inFile.write(qba);
        inFile.close();
    }

    void reNameUserMaterialFromJson(const QString& materialName, const QString& newMaterialName, const QString& machineName)
    {
        //1.��ȡ
        QString path = QString("%1/%2/materials_user.json").arg(_pfpt_user_parampack).arg(machineName);
        QFile file(path);
        if (!file.open(QFile::ReadOnly | QFile::Text))
        {
            return;
        }

        QString str = file.readAll();
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

        //3.����д��
        QFile inFile(QString("%1/%2/materials_user.json").arg(_pfpt_user_parampack).arg(machineName));
        if (!inFile.open(QFile::WriteOnly | QFile::Text))
        {
            return;
        }

        qdoc.setObject(rootObj);
        QByteArray qba = qdoc.toJson(QJsonDocument::Indented);
        inFile.write(qba);
        inFile.close();
    }

    void saveMateriaMeta(const MaterialData& meta, const QString& machineName)
    {
        //1.��ȡ
        QString filePath = QString("%1/%2/materials_user.json").arg(_pfpt_user_parampack).arg(machineName);
        QFile file(filePath);
        if (!file.open(QFile::ReadWrite | QFile::Text))
        {
            return;
        }

        QString str = file.readAll();
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

        QJsonObject nameObj;
        nameObj["name"] = meta.name;
        nameObj["type"] = meta.type;
        nameObj["brand"] = meta.brand;
        nameObj["supportDiameters"] = QString::number(meta.diameter, 'f', 2);
        arry.append(nameObj);
        rootObj["materials"] = arry;

        //3.����д��
        QFile inFile(QString("%1/%2/materials_user.json").arg(_pfpt_user_parampack).arg(machineName));
        if (!inFile.open(QFile::WriteOnly | QFile::Text))
        {
            return;
        }

        QJsonDocument qdocIn;
        qdocIn.setObject(rootObj);
        QByteArray qba = qdocIn.toJson(QJsonDocument::Indented);
        inFile.write(qba);
        inFile.close();
    }

    void loadOfficialMachineNames(QStringList& names)
    {
        names.clear();
        QDir profile_dir(_pfpt_default_parampack);
        for (const auto& entry : profile_dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            names << entry.fileName();
        }
    }

    void loadUserMachineNames(QStringList& names)
    {
        names.clear();
        QDir profile_dir(_pfpt_user_parampack);
        for (const auto& entry : profile_dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            names << entry.fileName();
        }
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

    void makeSureUserParamPackDir(const QString& machineUniqueName)
    {
        QStringList paramdirs = { "Materials","Overrides","Processes" };
        for (const auto& dir : paramdirs)
        {
            QString _folder = QString("%1/%2/%3").arg(_pfpt_user_parampack).arg(machineUniqueName).arg(dir);
            mkMutiDir(_folder);
        }
    }

    void removeSubDirectory(const QString& folder, const QString& subFolder)
    {

    }

    QStringList fetchFileNames(const QString& directory, bool noExt)
    {
        QDir dir(directory);
        dir.setFilter(QDir::Files);
        dir.setSorting(QDir::Time | QDir::Reversed);

        QFileInfoList fileInfos = dir.entryInfoList();

        QStringList fileNames;
        for (QFileInfo& fileInfo : fileInfos)
        {
            QString name =  fileInfo.fileName();
            if (!fileNames.contains(name) && !name.contains(".cover"))
            {
                if (noExt)
                {
                    name = name.mid(0, name.lastIndexOf("."));
                }
                fileNames.append(name);
            }
                
        }
        return fileNames;
    }

    void createMachineSettings(const QString& uniqueName, us::USettings* uSettings, QList<us::USettings*>* extruderSettings, bool isUser, MachineData* data)
    {
        QString fileName = defaultMachineFile(uniqueName, isUser);

        us::DefaultLoader loader;
        loader.loadDefault(fileName, uSettings, extruderSettings, data);
        if(data->inherits_from!="")
        {
            QString inherits_fileName = defaultMachineFile(data->inherits_from, false);
            us::DefaultLoader inherits_loader;
            MachineData inherits_data;
            us::USettings settings1;
            QList<us::USettings*> extruderSettings1;
            inherits_loader.loadDefault(inherits_fileName, &settings1, &extruderSettings1, &inherits_data);
            data->preferredProcess = inherits_data.preferredProcess;
            data->extruderCount = inherits_data.extruderCount;
            //data->extruderDiameters = inherits_data.extruderDiameters;
            data->is_bbl_printer = inherits_data.is_bbl_printer;
            data->top_material = inherits_data.top_material;
        }
        uSettings->appendDefault(machine_parameter_keys);
        for (const auto& extruder : *extruderSettings)
        {
            extruder->appendDefault(extruder_parameter_keys);
        }
    }

    us::USettings* createMachineCoverSettings(const QString& uniqueName)
    {
        QString fileName = machineCoverFile(uniqueName);

        QStringList preKeys;
        return createSettings(fileName, preKeys);
    }

	QString defaultMachineFile(const QString& baseMachineName, bool isUser)
	{
        if (isUser)
        {
            return QString("%1/%2/%2.def.json").arg(_pfpt_user_parampack).arg(baseMachineName);
        }
		return QString("%1/%2/%2.def.json").arg(_pfpt_default_parampack).arg(baseMachineName);
	}

    QString machineCoverFile(const QString& uniqueName)
    {
        QString tmpProject = projectUI()->getAutoProjectDirPath();
        mkMutiDir(QString("%1/%2/").arg(tmpProject).arg(uniqueName));
        QString fileName = QString("%1/%2/%2.def.json.cover").arg(tmpProject).arg(uniqueName);
        
        return fileName;
    }

    void removeUserMachineFile(const QString& uniqueName)
    {
        QString filePath = defaultMachineFile(uniqueName, true);
        QString coverFilePath = machineCoverFile(uniqueName);

        QFile machineFile(filePath);
        if(machineFile.exists())
            machineFile.remove();

        QFile coverMachineFile(coverFilePath);
        if (coverMachineFile.exists())
            coverMachineFile.remove();
    }

	QString userExtruderFile(const QString& machine, int index)
	{
        QString tmpProject = projectUI()->getAutoProjectDirPath();
        mkMutiDir(QString("%1/%2/").arg(tmpProject).arg(machine));
        QString name = QString("%1/%2/extruder_%3.json.cover").arg(tmpProject).arg(machine).arg(index);
		return name;
	}

    void removeUserExtuderFile(const QString& machineName, int index)
    {
        QString extruderFilePath = userExtruderFile(machineName, index);

        QFile machineFile(extruderFilePath);
        if (machineFile.exists())
            machineFile.remove();
    }

    us::USettings* createUserExtruderSettings(const QString& machineName, int index)
    {
        QString fileName = userExtruderFile(machineName, index);

        QStringList preKeys;
        return createSettings(fileName, preKeys);
    }

    us::USettings* createUserExtruderSettingsFromUser(const QString& machineName, int index, const QString& sourceMachineName)
    {
        QString fileName = userExtruderFile(machineName, index);

        QStringList preKeys;
        return createSettings(fileName, preKeys);
    }

    void copyFile2User(const QString& source, const QString& machineName, const QString& profileName, bool createCover)
    {
        QString dest = userProfileCoverFile(machineName, profileName);
        qtuser_core::copyFile(source, dest, false);
        if (createCover)
        {
            dest = userProfileCoverFile(machineName, profileName);
            qtuser_core::copyFile(source, dest, false);
        }
    }

    void copyPrinterFile2User(const QString& source, const QString& machineName, const QString& profileName, bool createCover)
    {
        QString dest = userProfileCoverFile(machineName, profileName);
        qtuser_core::copyFile(source, dest, false);
        if (createCover)
        {
            dest = userProfileCoverFile(machineName, profileName);
            qtuser_core::copyFile(source, dest, false);
        }
    }

    void copyMaterialFile2User(const QString& source, const QString& machineName, const QString& profileName, bool createCover)
    {
        QString dest = userProfileCoverFile(machineName, profileName);
        qtuser_core::copyFile(source, dest, false);
        if (createCover)
        {
            dest = userProfileCoverFile(machineName, profileName);
            qtuser_core::copyFile(source, dest, false);
        }
    }

    QString defaultProfileFile(const QString& machineName, const QString& profile)
    {
        return QString("%1/%2/Processes/%3").arg(_pfpt_default_parampack).arg(machineName).arg(profile);
    }

    QString defaultProfileCoverFile(const QString& machineName, const QString& profile, const QString& materialName)
    {
        QString name = QString("%1/%2/Processes/%3.cover").arg(_pfpt_user_parampack).arg(machineName).arg(materialName.isEmpty() ? profile : materialName + "@" + profile);
        return name;
    }

    QString userProfileFile(const QString& machineName, const QString& profile, const QString& materialName)
    {
        QString name = QString("%1/%2/Processes/%3").arg(_pfpt_user_parampack).arg(machineName).arg(profile);
        return name;
    }

	QString userProfileCoverFile(const QString& machineName, const QString& profile, const QString& materialName)
	{
        QString tmpProject = projectUI()->getAutoProjectDirPath();
        mkMutiDir(QString("%1/%2/Processes/").arg(tmpProject).arg(machineName));
        QString name = QString("%1/%2/Processes/%3.cover").arg(tmpProject).arg(machineName).arg(profile);

        return name;
	}

    QStringList fetchOfficialProfileNames(const QString& machineName)
    {
        return fetchFileNames(QString("%1/%2/Processes").arg(_pfpt_default_parampack).arg(machineName));
    }

    QStringList fetchUserProfileNames(const QString& machineName)
    {
        return fetchFileNames(QString("%1/%2/Processes").arg(_pfpt_user_parampack).arg(machineName));
    }

    void removeUserProfileFile(const QString& machineName, const QString& fileName, const QString& materialName)
    {
        QFile profileFile(userProfileFile(machineName, fileName));
        profileFile.remove();
        QFile coverFile(userProfileCoverFile(machineName, fileName, materialName));
        coverFile.remove();
    }

    us::USettings* createDefaultProfileSettings(const QString& machineName, const QString& profileName)
    {
        QString fileName = defaultProfileFile(machineName, profileName);
        return createSettings(fileName, profile_parameter_keys);
    }
    us::USettings* createDefaultProfileCoverSettings(const QString& machineName, const QString& profileName, const QString& materialName)
    {
        QString fileName = userProfileCoverFile(machineName, profileName, materialName);
        QStringList preKeys;
        return createSettings(fileName, preKeys);
    }

    us::USettings* createUserProfileSettings(const QString& machineName, const QString& profileName)
    {
        QString fileName = userProfileFile(machineName, profileName);
        return createSettings(fileName, profile_parameter_keys);
    }

    us::USettings* createUserProfileCoverSettings(const QString& machineName, const QString& profileName, const QString& materialName)
    {
        QString fileName = userProfileCoverFile(machineName, profileName, materialName);
        QStringList preKeys;
        return createSettings(fileName, preKeys);
    }

    QString defaultMaterialFile(const QString& uniqueName, const QString& materialName, bool user)
    {
        if (user)
        {
            return QString("%1/%2/Materials/%3.json").arg(_pfpt_user_parampack).arg(uniqueName).arg(materialName);
        }
        return QString("%1/%2/Materials/%3.json").arg(_pfpt_default_parampack).arg(uniqueName).arg(materialName);
    }

    QString materialCoverFile(const QString& uniqueName, const QString& materialName)
    {
        QString tmpProject = projectUI()->getAutoProjectDirPath();
        mkMutiDir(QString("%1/%2/Materials").arg(tmpProject).arg(uniqueName));
        return QString("%1/%2/Materials/%3.json.cover").arg(tmpProject).arg(uniqueName).arg(materialName);
    }

    QString userExtruderOverrideFile(const QString& machineUniqueName, const QString& materialUniqueName)
    {
        QString fileName = QString("%1/%2/Overrides/%3.json").arg(_pfpt_user_parampack)
            .arg(machineUniqueName).arg(materialUniqueName);
        return fileName;
    }

    QString defaultExtruderOverrideFile(const QString& machineUniqueName, const QString& materialUniqueName)
    {
        QString fileName = QString("%1/%2/Overrides/%3.json").arg(_pfpt_default_parampack)
            .arg(machineUniqueName).arg(materialUniqueName);
        return fileName;
    }

    QString userProcessOverrideFile(const QString& machineUniqueName, const QString& materialUniqueName, const QString& process)
    {
        QString fileName = QString("%1/%2/Overrides/%3-%4.json").arg(_pfpt_user_parampack)
            .arg(machineUniqueName).arg(process).arg(materialUniqueName);
        return fileName;
    }

    QString defaultProcessOverrideFile(const QString& machineUniqueName, const QString& materialUniqueName, const QString& process)
    {
        QString fileName = QString("%1/%2/Overrides/%3-%4.json").arg(_pfpt_default_parampack)
            .arg(machineUniqueName).arg(process).arg(materialUniqueName);
        return fileName;
    }

    us::USettings* createMaterialSettings(const QString& uniqueName, const MaterialData& data, bool user)
    {
        QString fileName = defaultMaterialFile(uniqueName, data.uniqueName(), user);

        return createSettings(fileName, material_parameter_keys);
    }

    us::USettings* createMaterialCoverSettings(const QString& uniqueName, const MaterialData& data)
    {
        QString fileName = materialCoverFile(uniqueName, data.uniqueName());
        QStringList preKeys;
        return createSettings(fileName, preKeys);
    }

    us::USettings* createProcessOverrideSettings(const QString& uniqueName, const MaterialData& data, const QString& quality)
    {
        QString fileName = QString("%1/%2/Overrides/%3-%4.json").arg(_pfpt_user_parampack)
            .arg(uniqueName).arg(quality).arg(data.uniqueName());
        if (!QFile::exists(fileName))
        {
            fileName = QString("%1/%2/Overrides/%3-%4.json").arg(_pfpt_default_parampack)
                .arg(uniqueName).arg(quality).arg(data.uniqueName());
        }
        if (!QFile::exists(fileName))
        {
            return new us::USettings();
        }
        return createSettings(fileName);
    }

    us::USettings* createExtruderOverrideSettings(const QString& machineUniqueName, const QString& materialUniqueName)
    {
        QString fileName = userExtruderOverrideFile(machineUniqueName, materialUniqueName);
        if (!QFile::exists(fileName))
        {
            fileName = defaultExtruderOverrideFile(machineUniqueName, materialUniqueName);
        }
        if (!QFile::exists(fileName))
        {
            return new us::USettings();
        }
        return createSettings(fileName);
    }
}
