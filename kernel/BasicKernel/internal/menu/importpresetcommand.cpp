#include "importpresetcommand.h"
#include "interface/commandinterface.h"
#include "cxkernel/interface/iointerface.h"
#include "qtusercore/module/quazipfile.h"
#include "internal/parameter/parameterpath.h"
#include "internal/parameter/parametermanager.h"
#include <QFileInfo>
#include <QJsonDocument>
#include <QIODevice>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include "kernel/kernel.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include <rapidjson/filereadstream.h>
#include "internal/parameter/parametercache.h"
#include <QCoreApplication>
#include <QTextCodec>
namespace creative_kernel
{
    ImportPresetCommand::ImportPresetCommand()
        :ActionCommand()
    {
        m_shortcut = "Ctrl+I";
        m_actionname = QCoreApplication::translate("MenuCmdObj", "Preset Config");
        m_actionNameWithout = "Import Preset";
        m_icon = "qrc:/kernel/images/open.png";
        m_eParentMenu = eMenuType_File;
        m_zipFile = new qtuser_core::QuazipFile();
        addUIVisualTracer(this,this);
        //processOrcaPrinter("C:\\Creality K1 (0.4 nozzle).orca_printer");
        //process3MFPrinter("C:\\test4.3mf");
    }

    ImportPresetCommand::~ImportPresetCommand()
    {
    }

    void ImportPresetCommand::setModelType(ModelNType type)
    {
        m_partType = type;
    }

    void ImportPresetCommand::execute()
    {
        cxkernel::openFile(this);
    }

    void ImportPresetCommand::onThemeChanged(ThemeCategory category)
    {
    }

    void ImportPresetCommand::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = QCoreApplication::translate("MenuCmdObj", "Preset Config");// +"        " + m_shortcut;
    }

    QString ImportPresetCommand::filter()
    {
        return QString("Preset File (*.zip *.orca_printer *.3mf)");
    }

    void ImportPresetCommand::handle(const QString& fileName)
    {
        QFileInfo fileInfo(fileName);
        if(fileInfo.suffix()=="zip")
        {
            getKernel()->parameterManager()->loadParamFromProject(fileName);
        }
        if(fileInfo.suffix()=="orca_printer")
        {
            processOrcaPrinter(fileName);
        }
        if(fileInfo.suffix()=="3mf")
        {
            process3MFPrinter(fileName);
        }
    }
    QFileInfo findFiles(const QString &startDir, const QString &baseName,const QString &extension) {
        QFileInfo findFileInfo;
        QDir dir(startDir);
    
        // 获取所有文件和目录
        QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    
        for (int i = 0; i < list.size(); ++i) {
            QFileInfo fileInfo = list.at(i);
            if (fileInfo.isFile() && fileInfo.fileName() == (baseName +"." +extension)) {
                qDebug() << "Found file:" << fileInfo.absoluteFilePath();
                return fileInfo;
            }
    
            // 如果是目录，递归调用
            if (fileInfo.isDir()) {
                findFileInfo = findFiles(fileInfo.absoluteFilePath(), baseName, extension);
                if (findFileInfo.exists())
                {
                    break;
                }
            }
        }
        return findFileInfo;
    }
    QList<QFileInfo> findProfileFiles(const QString &startDir, const QString &likeName) {
        QList<QFileInfo> findFilesInfo;
        QDir dir(startDir);
    
        // 获取所有文件和目录
        QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    
        for (int i = 0; i < list.size(); ++i) {
            QFileInfo fileInfo = list.at(i);
    
            // 如果是文件，检查扩展名
            qDebug() << fileInfo.fileName();
            if (fileInfo.isFile() && fileInfo.fileName().indexOf(likeName)>0) {
                qDebug() << "Found file:" << fileInfo.absoluteFilePath();
                findFilesInfo.append(fileInfo);
            }
    
            // 如果是目录，递归调用
            if (fileInfo.isDir()) {
                findFilesInfo = findProfileFiles(fileInfo.absoluteFilePath(), likeName);
                
            }
        }
        return findFilesInfo;
    }
    QJsonObject ImportPresetCommand::loadFromKeys(QString inherits)
    {
        QJsonObject outObject;
        QString inheritFile = findFiles( QCoreApplication::applicationDirPath() + "/resources/thirdparty/", inherits, "json").absoluteFilePath();
        QFile file(inheritFile);
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            return outObject;
        }
        QByteArray data = file.readAll();
        file.close();
        
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if(!doc.isNull())
        {
            QJsonObject rootObj = doc.object();
            if(rootObj.contains("inherits"))
            {
                QJsonObject inherits = this->loadFromKeys(rootObj["inherits"].toString());
                outObject = inherits;
            }
            for(auto key:rootObj.keys())
            {
                    outObject[key] = rootObj[key];
            }
            
        }
        int index = inheritFile.lastIndexOf("machine");
        if(index>1)
        { 
            int begin = inheritFile.mid(0,index-1).lastIndexOf("/");
            outObject["producter"] = inheritFile.mid(begin+1,index-begin-2);
        }
        
        return outObject;
    }
    bool ImportPresetCommand::save2json(QString preset_name,QString path,QString type,QJsonObject json)
    {
        QString modifiedPresetName;
         QJsonDocument outdocument;
         QString outPath;
         QString extruderDiameter = QString::number(m_machineMetaData.extruderDiameters.at(0),10,1);
         modifiedPresetName = m_machineMetaData.codeName+"-"+extruderDiameter;
            if (type == "printer")
            {

                
                QDir dir(_pfpt_user_parampack + "/" + modifiedPresetName);
                if (!dir.exists())
                {
                    dir.mkdir(_pfpt_user_parampack + "/" + modifiedPresetName);
                }
                dir.setPath(_pfpt_user_parampack + "/" + modifiedPresetName + "/Materials");
                if (!dir.exists())
                {
                    dir.mkdir(_pfpt_user_parampack + "/" + modifiedPresetName + "/Materials");
                }
                dir.setPath(_pfpt_user_parampack + "/" + modifiedPresetName + "/Processes");
                if (!dir.exists())
                {
                    dir.mkdir(_pfpt_user_parampack + "/" + modifiedPresetName + "/Processes");
                }

                outPath = _pfpt_user_parampack + "/" + modifiedPresetName +"/" + modifiedPresetName+".def.json";
            }
            else if (type == "materials")
            {
                QString matrialName = path.mid(path.lastIndexOf("/")).replace(".json","-"+QString::number(m_machineMetaData.supportMaterialDiameters.at(0), 10, 2) + ".json");
                outPath = _pfpt_user_parampack +"/" + modifiedPresetName + "/Materials" + matrialName;
            }
            else {
                outPath = _pfpt_user_parampack + "/" + modifiedPresetName + "/Processes"+ path.mid(path.lastIndexOf("/"));
            }
            QFile file(outPath);
            if (!file.open(QIODevice::WriteOnly)) {
                    // 处理错误，例如可以抛出异常或者返回错误标志
                    return false; // 或者其他错误处理
                }
            outdocument.setObject(json);
            file.write(outdocument.toJson());
            file.close();
    }
    bool ImportPresetCommand::processOrcaProfile(QString type,QString path,QString preset_name)
    {
        
        QString modifiedPresetName;
        QJsonObject json;
        QJsonObject metadata;
        

        json.insert("version", "1.0");
        json.insert("engine_version", "1.9.1.0");
         QStringList extruderKeys;
        us::loadMetaExtruderKeys(extruderKeys);//getExtrudersKeys();
                
        auto processLambda = [this, &json,&type,extruderKeys,&metadata](QIODevice& device)->bool{
                QByteArray data = device.readAll();
                QJsonDocument doc = QJsonDocument::fromJson(data);
                QJsonObject engine_data;
                QJsonArray extruders;
                QJsonObject extruder;
                QJsonObject printer;
                QJsonObject extruder_engine_data;
                     if(!doc.isNull())
                    {
                        QJsonObject rootObj = doc.object();
                        
                        QJsonObject inherits;
                        if(rootObj.contains("inherits"))
                        {
                            inherits = this->loadFromKeys(rootObj["inherits"].toString());
                            //qDebug() << "inherits<<keys<<"<<inherits.keys().size();
                        }
                        for(auto key:rootObj.keys())
                        {
                            inherits[key] = rootObj[key];
                        }
                        //qDebug() << inherits.keys().size();
                        for(auto key:inherits.keys())
                        {
                            QString value;
                            if (inherits[key].isString())
                            { 
                                value = inherits[key].toString();
                            }
                            if (inherits[key].isArray())
                            {
                                QJsonArray array = inherits[key].toArray();
                                for (auto itr = array.begin(); itr != array.end(); ++itr) {
                                    value += itr->toString()+",";
                                }
                                value = value.mid(0, value.length() - 1);
                            }
                          
                            if(type!="printer")
                            {
                                    engine_data.insert(key, value);
                            }else{
                                if (extruderKeys.contains(key))
                                {
                                        extruder_engine_data.insert(key, value);
                                }
                                    
                                printer.insert(key, value);
                             }
                                
                            
                                
                            
                            //rootObj[key]
                        }
                        
                    }
                    if(type!="printer")
                    {
                        metadata.insert("name",engine_data["name"]);
                        json["engine_data"] =  engine_data;
                        if(type=="materials")
                        {
                            m_machineMetaData.supportMaterialNames.append(engine_data["name"].toString());
                            if (!m_machineMetaData.supportMaterialTypes.contains(engine_data["filament_type"].toString()))
                            {
                                m_machineMetaData.supportMaterialTypes.append(engine_data["filament_type"].toString());
                            }
                            
                            
                        }
                        
                    }
                        
                    else{
                        extruder["engine_data"] = extruder_engine_data;
                        QJsonObject extruder_metadata;
                        extruder_metadata.insert("preferred_material",printer["default_filament_profile"]);
                        extruder["metadata"] = extruder_metadata;
                        extruders.append(extruder);
                        json["engine_data"] =  QJsonObject();
                        json["extruders"] =  extruders;
                        json["printer"] =  printer;
                        metadata.insert("show_name", printer["name"]);
                        metadata.insert("is_import",true);
                        metadata.insert("preferred_process", printer["default_print_profile"]);
                        
                        if(m_machineMetaData.extruderDiameters.size()==0)
                            m_machineMetaData.extruderDiameters << printer["printer_variant"].toString().toFloat();
                        m_machineMetaData.supportMaterialDiameters.append(1.75);
                        QRegularExpression regex("\\s\\([0-9]\\.[0-9]\\snozzle\\)|\\s[0-9]\\.[0-9]\\snozzle");
                        
                        QString modifiedPresetName = printer["name"].toString().replace(regex, "");
                        m_machineMetaData.codeName = modifiedPresetName;
                        m_machineMetaData.baseName = modifiedPresetName;
                        
                        
                        
                    }
                    json.insert("metadata", metadata);
                
                              
                    return true;
            };
            

            if(m_zipFile->openSubFile(path,processLambda))
            {
                save2json(preset_name,path,type,json);
            }
            else {
                if (m_zipFile->openSubFile(QTextCodec::codecForLocale()->toUnicode(QByteArray::fromStdString(path.toStdString())), processLambda))
                {
                    save2json(preset_name,path,type,json);
                }
            }
           
            if(type=="printer")
            {
                QJsonObject printer = json["printer"].toObject();
                QString inherits_preset_name = printer["inherits"].toString();
                QString producter = printer["producter"].toString();
                
                QList<QFileInfo> filaments = findProfileFiles(QCoreApplication::applicationDirPath()  + "/resources/thirdparty/profiles/"+producter+"/filament","json");
                for(QFileInfo info:filaments)
                {
                    json = QJsonObject();
                    metadata = QJsonObject();
                    QFile ifile(info.absoluteFilePath());
                    if(ifile.open(QIODevice::ReadOnly))
                    {
                        type = "materials";
                        if(processLambda(ifile))
                        {
                            QJsonObject filament = json["engine_data"].toObject();
                            int size = filament.size();
                            
                            if (filament.contains("compatible_printers"))
                            {
                                QString compatible_printers = filament["compatible_printers"].toString();

                                    if(compatible_printers.split(",").indexOf(preset_name) >= 0)
                                    {
                                        save2json(preset_name,info.absoluteFilePath(),type,json);
                                    }

                                
                            }
                            
                        }
                        ifile.close();
                        
                    }
                    //QFile::copy(info.absoluteFilePath(),_pfpt_user_parampack +"/" + preset_name + "/Materials/" +info.fileName());
                }
                QList<QFileInfo> processes = findProfileFiles(QCoreApplication::applicationDirPath()  + "/resources/thirdparty/profiles/" + producter + "/process", "json");
                for (QFileInfo info : processes)
                {
                    json = QJsonObject();
                    metadata = QJsonObject();
                    QFile ifile(info.absoluteFilePath());
                    if (ifile.open(QIODevice::ReadOnly))
                    {
                        type = "process";
                        if (processLambda(ifile))
                        {
                            QJsonObject process = json["engine_data"].toObject();
   
                            if (process.contains("compatible_printers"))
                            {
                                QString compatible_printers = process["compatible_printers"].toString();
                                //for (auto itr = compatible_printers.begin(); itr != compatible_printers.end(); ++itr) {
                                    //qDebug()<<itr->toString();
                                if (compatible_printers.split(",").indexOf(preset_name) >= 0)
                                {
                                    save2json(preset_name,info.absoluteFilePath(), type, json);
                                }
                                // }


                            }

                        }
                        ifile.close();

                    }
                    //QFile::copy(info.absoluteFilePath(),_pfpt_user_parampack +"/" + preset_name + "/Materials/" +info.fileName());
                }

            }
            
            return true;
    }
    bool ImportPresetCommand::processStructFile(QIODevice& device)
    {

        QByteArray data = device.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QString preset_name="";
        if (!doc.isNull()) {
        // 获取根节点（JSON对象）
            QJsonObject rootObj = doc.object();
            if(rootObj.contains("printer_preset_name"))
            {
                preset_name = rootObj["printer_preset_name"].toString();
                
            }
            if(rootObj.contains("printer_config"))
            {
                QJsonArray printer_configs = rootObj["printer_config"].toArray();
                for(auto itr=printer_configs.begin();itr!=printer_configs.end();++itr){
                    QString path = itr->toString();
                    processOrcaProfile("printer",path, preset_name);
                    //processPrinter(path,_pfpt_user_parampack+path.mid(path.indexOf("/")));
                }
            }
            QString modifiedPresetName = "";
            if (m_machineMetaData.extruderDiameters.size() > 0)
            {
                QString extruderDiameter = QString::number(m_machineMetaData.extruderDiameters.at(0), 10, 1);
                modifiedPresetName = m_machineMetaData.codeName + "-" + extruderDiameter;
            }
            else {
                return false;
            }
            
             if(rootObj.contains("filament_config"))
            {
                QJsonArray printer_configs = rootObj["filament_config"].toArray();
                QDir dir(_pfpt_user_parampack+"/"+ modifiedPresetName + "/Materials");
                if(!dir.exists())
                {
                    dir.mkdir(_pfpt_user_parampack+"/"+ modifiedPresetName +"/Materials");
                }
                for(auto itr=printer_configs.begin();itr!=printer_configs.end();++itr){
                    QString path = itr->toString();
                    processOrcaProfile("materials",path, preset_name);
                    //processFilament(path,_pfpt_user_parampack+path.mid(path.indexOf("/"));
                }
            }
             if(rootObj.contains("process_config"))
            {
                QJsonArray printer_configs = rootObj["process_config"].toArray();
                QDir dir(_pfpt_user_parampack+"/"+ modifiedPresetName +"/Processes");
                if(!dir.exists())
                {
                    dir.mkdir(_pfpt_user_parampack+"/"+ modifiedPresetName +"/Processes");
                }
                for(auto itr=printer_configs.begin();itr!=printer_configs.end();++itr){
                    QString path = itr->toString();
                    processOrcaProfile("processes",path, preset_name);
                    //processProcess(path,_pfpt_user_parampack+path.mid(path.indexOf("/"));
                }
            }

            
        }
        return true;
    }
    void ImportPresetCommand::processOrcaPrinter(QString filename)
    {
        //auto lambda = [](int a)->string{if(a==2)return "123";else return "456";};
        
        QuazipSubFunc structureFun = std::bind(&ImportPresetCommand::processStructFile, this, std::placeholders::_1); 
        
        m_zipFile->open(filename);
        if(m_zipFile->openSubFile("bundle_structure.json",structureFun))
        {
                    m_machineMetaData.isUser = true;
                    m_machineMetaData.is_import = true;
                    removeCacheMachine(m_machineMetaData.uniqueName());
                    writeCacheMachineExtruder(m_machineMetaData.uniqueName(), QColor("#00ff00"), true);
                    getKernel()->parameterManager()->importPrinterFromOrca(m_machineMetaData);
        }
        
    }
    bool ImportPresetCommand::process3MFStructFile(QIODevice& device)
    {
        MachineData data;
        data.isUser = true;
        data.is_import = true;
        QByteArray jsonData = device.readAll();
        qDebug()<<jsonData.size();

        char read_buffer[100 * 1024] = { 0 };
        rapidjson::Document document;
        //rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
        const char* jsondata;
        jsondata = jsonData.data();
        document.Parse(jsondata);   

        if (document.HasParseError())
        {
            return false;
        }
        QString printer_settings_id;
        QString printer_variant;
        QStringList inherits_groups;
        //QString inherits_group;
        QString print_settings_id;
        if (document.HasMember("printer_settings_id") && document["printer_settings_id"].IsString())
        {
            printer_settings_id = document["printer_settings_id"].GetString();
        }
        if (document.HasMember("printer_variant") && document["printer_variant"].IsString())
        {
            printer_variant = document["printer_variant"].GetString();
        }
        if (document.HasMember("print_settings_id") && document["print_settings_id"].IsString())
        {
            print_settings_id = document["print_settings_id"].GetString();
        }
        
        if (document.HasMember("inherits_group") && document["inherits_group"].IsArray())
        {
            const auto inherits_group_array = document["inherits_group"].GetArray();
            std::string value;
            for (size_t i = 0; i < inherits_group_array.Size(); i++)
            {
                inherits_groups.append(inherits_group_array[i].GetString());   
            }
        }
        //判断机型是否继承。继承机型只写入.cover文件。非继承机型完整写入
        QString printer_inherits_group = "";
        bool printer_inherited = false;
        /*if (inherits_groups.size()>0)
        {
            if (inherits_groups.at(inherits_groups.size() - 1) != "")
            {//非继承机型
                printer_inherits_group = inherits_groups.at(inherits_groups.size() - 1);
                printer_inherited = true;
            }
        }*/
        
        QRegularExpression regex("\\s\\([0-9]\\.[0-9]\\snozzle\\)|\\s[0-9]\\.[0-9]\\snozzle");
        QString modifiedPresetName = printer_settings_id.replace(regex, "");
        if (modifiedPresetName.isEmpty() && document.HasMember("printer_model") && document["printer_model"].IsString())
        {
            modifiedPresetName = document["printer_model"].GetString();
        }
        data.baseName = modifiedPresetName;
        data.codeName = modifiedPresetName;
        if(data.extruderDiameters.size()==0)
            data.extruderDiameters << printer_variant.toFloat();
        QString machine_file_name = printer_inherited ? machineCoverFile(data.uniqueName()): defaultMachineFile(data.uniqueName(), true);
        makeSureUserParamPackDir(data.uniqueName());
        rapidjson::Document docJsonFile;
        docJsonFile.SetObject();
        auto& allocator = docJsonFile.GetAllocator();

        QStringList machine_parameter_keys = getMetaMachineKeys();
        QStringList extruder_parameter_keys = getMetaExtruderKeys();
        QStringList profile_parameter_keys = getMetaProfileKeys();
        QStringList material_parameter_keys = getMetaMaterialKeys();
        QStringList different_settings_to_system;

        if (document.HasMember("different_settings_to_system") && document["different_settings_to_system"].IsArray())
        {
            const auto& array = document["different_settings_to_system"].GetArray();
            for (size_t i = 0; i < array.Size(); i++)
            {
                different_settings_to_system.append(QString::fromStdString(array[i].GetString()));
            }
        }
        auto printerObj = rapidjson::Value{ rapidjson::Type::kObjectType };
        for (const auto& key : machine_parameter_keys)
        {
            
            if(different_settings_to_system.size()>0 && printer_inherited)
            {
                 if(!different_settings_to_system.at(0).contains(key))
                {
                    continue;
                }
            }
           
            auto key_string = key.toStdString();
            const char* ckey = key_string.c_str();
            if (document.HasMember(ckey) && document[ckey].IsString())
            {
                printerObj.AddMember(rapidjson::Value(ckey, allocator), rapidjson::Value(document[ckey].GetString(), allocator), allocator);
            }
            if (document.HasMember(ckey) && document[ckey].IsArray())
            {
                const auto arrayObj = document[ckey].GetArray();
                std::string value;
                for (size_t i = 0; i < arrayObj.Size(); i++)
                {
                    if (!value.empty())
                    {
                        value += ",";
                    }
                    value += arrayObj[i].GetString();
                }
                printerObj.AddMember(rapidjson::Value(ckey, allocator), rapidjson::Value(value.c_str(), allocator), allocator);
            }
        }
        //rapidjson::Value v1(document, allocator);
        docJsonFile.AddMember("printer", printerObj, allocator);

        auto extruder_array = rapidjson::Value{ rapidjson::Type::kArrayType };
        for (int i = 0; i < 1; i++)
        {
            auto extruder_object = rapidjson::Value{ rapidjson::Type::kObjectType };
            auto engine_object = rapidjson::Value{ rapidjson::Type::kObjectType };
            
            for (const auto& key : extruder_parameter_keys)
            {
                if(different_settings_to_system.size()>0 && printer_inherited)
                {
                    if(!different_settings_to_system.at(0).contains(key))
                    {
                        continue;
                    }
                }
                auto key_string = key.toStdString();
                const char* ckey = key_string.c_str();
                if (document.HasMember(ckey) && document[ckey].IsString())
                {
                    engine_object.AddMember(rapidjson::Value(ckey, allocator), rapidjson::Value(document[ckey].GetString(), allocator), allocator);
                }
                if (document.HasMember(ckey) && document[ckey].IsArray())
                {
                    const auto& array = document[ckey][0];
                    engine_object.AddMember(rapidjson::Value(ckey, allocator), rapidjson::Value(array.GetString(), allocator), allocator);
                }
            }
            extruder_object.AddMember("engine_data", engine_object, allocator);
            extruder_array.PushBack(extruder_object, allocator);
        }
        docJsonFile.AddMember("extruders", std::move(extruder_array), allocator);
        rapidjson::StringBuffer strbuf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
        docJsonFile.Accept(writer);
        QFile machine_cover_file(machine_file_name);
        if (!machine_cover_file.open(QIODevice::WriteOnly))
        {
            return false;
        }
        machine_cover_file.write(std::string(strbuf.GetString(), strbuf.GetSize()).c_str(), strbuf.GetSize());
        machine_cover_file.close();

        ////判断工艺是否继承
        QString process_inherits_group = print_settings_id;
        bool process_inherited = false;
        /*if (inherits_groups.size()>0)
        {
            if (inherits_groups.at(0) != "")
            {
                process_inherits_group = inherits_groups.at(0);
                process_inherited = true;
            }
        }*/
        docJsonFile.SetObject();
        auto processObj = rapidjson::Value{ rapidjson::Type::kObjectType };

        for (const auto& key : profile_parameter_keys)
        {
            if(different_settings_to_system.size()>0 && process_inherited)
            {
                if(!different_settings_to_system.at(different_settings_to_system.size()-1).contains(key))
                {
                    continue;
                }
            }
            auto key_string = key.toStdString();
            const char* ckey = key_string.c_str();
            if (document.HasMember(ckey) && document[ckey].IsString())
            {
                processObj.AddMember(rapidjson::Value(ckey, allocator), rapidjson::Value(document[ckey].GetString(), allocator), allocator);
            }
            if (document.HasMember(ckey) && document[ckey].IsArray())
            {
                const auto arrayObj = document[ckey].GetArray();
                std::string value;
                for (size_t i = 0; i < arrayObj.Size(); i++)
                {
                    if (!value.empty())
                    {
                        value += ",";
                    }
                    value += arrayObj[i].GetString();
                }
                processObj.AddMember(rapidjson::Value(ckey, allocator), rapidjson::Value(value.c_str(), allocator), allocator);
            }
        }
        docJsonFile.AddMember("engine_data", std::move(processObj), allocator);
        QString default_print_profile;
        if (document.HasMember("print_settings_id") && document["print_settings_id"].IsString())
        {
            default_print_profile = document["print_settings_id"].GetString();
        }
        QString process_file_name = process_inherited? userProfileCoverFile(data.uniqueName(), default_print_profile+".json"): userProfileFile(data.uniqueName(), default_print_profile + ".json");

        rapidjson::StringBuffer strbufProcess;
        rapidjson::Writer<rapidjson::StringBuffer> writerProcess(strbufProcess);
        docJsonFile.Accept(writerProcess);
        QFile process_json_file(process_file_name);
        if (!process_json_file.open(QIODevice::WriteOnly))
        {
            return false;
        }
        process_json_file.write(std::string(strbufProcess.GetString(), strbufProcess.GetSize()).c_str(), strbufProcess.GetSize());
        process_json_file.close();
        
        //处理耗材
        QStringList print_materials;
        if (document.HasMember("filament_settings_id") && document["filament_settings_id"].IsArray())
        {
            const auto& array = document["filament_settings_id"];
            for (size_t i = 0; i < array.Size(); i++)
            {
                print_materials << array[i].GetString();
            }
        }
        for (size_t i = 0; i < print_materials.size(); i++)
        {
            docJsonFile.SetObject();
            //判断耗材是否继承
            bool material_inherited = false;
            QString material_inherits_group;
            /*
            if (inherits_groups.size() > 0)
            {
                if (inherits_groups.at(i+1) != "")
                {//
                    material_inherits_group = inherits_groups.at(i+1);
                    material_inherited = true;
                }
            }*/
            QString material_file_name = material_inherited ? materialCoverFile(data.uniqueName(), print_materials[i] + "-1.75"):defaultMaterialFile(data.uniqueName(), print_materials[i]+"-1.75", true);
            auto engine_object = rapidjson::Value{ rapidjson::Type::kObjectType };
            for (const auto& key : material_parameter_keys)
            {
                if(different_settings_to_system.size()>2 && material_inherited)
                {
                    if(!different_settings_to_system.at(i+1).contains(key))
                    {
                        continue;
                    }
                }
                auto key_string = key.toStdString();
                const char* ckey = key_string.c_str();
                if (document.HasMember(ckey) && document[ckey].IsArray())
                {
                    const auto& array = document[ckey][i];
                    engine_object.AddMember(rapidjson::Value(ckey, allocator), rapidjson::Value(array.GetString(), allocator), allocator);
                }
            }
            //engine_object.AddMember(rapidjson::Value("name", allocator), rapidjson::Value(print_materials[i].toUtf8().data(), allocator), allocator);
            docJsonFile.AddMember("engine_data", std::move(engine_object), allocator);
            rapidjson::StringBuffer strbufProcess;
            rapidjson::Writer<rapidjson::StringBuffer> writerProcess(strbufProcess);
            docJsonFile.Accept(writerProcess);
            QFile material_json_file(material_file_name);
            if (!material_json_file.open(QIODevice::WriteOnly))
            {
                return false;
            }
            material_json_file.write(std::string(strbufProcess.GetString(), strbufProcess.GetSize()).c_str(), strbufProcess.GetSize());
            material_json_file.close();
        }

        if (document.HasMember("filament_colour") && document["filament_colour"].IsArray())
        {
            const auto& array = document["filament_colour"].GetArray();
            removeCacheMachine(data.uniqueName());
            for (size_t i = 0; i < array.Size(); i++)
            {
                const auto& color = array[i].GetString();
                writeCacheMachineExtruder(data.uniqueName(), QColor(color), i ? false: true);
            }
            for (size_t i = 0; i < print_materials.length(); i++)
            {
                writeCacheExtruderMaterialIndex(data.uniqueName(),i,print_materials[i] + "-1.75");
            }
        }
        if (document.HasMember("filament_type") && document["filament_type"].IsArray())
        {
            const auto& array = document["filament_type"].GetArray();
            for (size_t i = 0; i < array.Size(); i++)
            {
                auto filament_type = array[i].GetString();
                if(!data.supportMaterialTypes.contains(filament_type))
                    data.supportMaterialTypes.append(filament_type);
            }
        }
       
        
        data.supportMaterialDiameters.append(1.75);
        m_machineMetaData = data;
        QJsonObject metadata;
        QString type = "materials";
        QJsonObject inherits_json;
        
        auto processLambda = [this, &inherits_json, &type, extruder_parameter_keys, &metadata](QIODevice& device)->bool {
            QByteArray data = device.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QJsonObject engine_data;
            QJsonArray extruders;
            QJsonObject extruder;
            QJsonObject printer;
            QJsonObject extruder_engine_data;
            if (!doc.isNull())
            {
                QJsonObject rootObj = doc.object();

                QJsonObject inherits;
                if (rootObj.contains("inherits"))
                {
                    inherits = this->loadFromKeys(rootObj["inherits"].toString());
                    //qDebug() << "inherits<<keys<<"<<inherits.keys().size();
                }
                for (auto key : rootObj.keys())
                {
                    inherits[key] = rootObj[key];
                }
                //qDebug() << inherits.keys().size();
                for (auto key : inherits.keys())
                {
                    QString value;
                    if (inherits[key].isString())
                    {
                        value = inherits[key].toString();
                    }
                    if (inherits[key].isArray())
                    {
                        QJsonArray array = inherits[key].toArray();
                        for (auto itr = array.begin(); itr != array.end(); ++itr) {
                            value += itr->toString() + ",";
                        }
                        value = value.mid(0, value.length() - 1);
                    }

                    if (type != "printer")
                    {
                        engine_data.insert(key, value);
                    }
                    else {
                        if (extruder_parameter_keys.contains(key))
                        {
                            extruder_engine_data.insert(key, value);
                        }

                        printer.insert(key, value);
                    }




                    //rootObj[key]
                }

            }
            if (type != "printer")
            {
                //qDebug() << engine_data.keys();
                metadata.insert("name", engine_data["name"]);
                inherits_json["engine_data"] = engine_data;
                if (type == "materials")
                {
                    m_machineMetaData.supportMaterialNames.append(engine_data["name"].toString());
                    if (!m_machineMetaData.supportMaterialTypes.contains(engine_data["filament_type"].toString()))
                    {
                        m_machineMetaData.supportMaterialTypes.append(engine_data["filament_type"].toString());
                    }


                }

            }

            else {
                extruder["engine_data"] = extruder_engine_data;
                QJsonObject extruder_metadata;
                extruder_metadata.insert("preferred_material", printer["default_filament_profile"]);
                extruder["metadata"] = extruder_metadata;
                extruders.append(extruder);
                inherits_json["engine_data"] = QJsonObject();
                inherits_json["extruders"] = extruders;
                inherits_json["printer"] = printer;
                metadata.insert("show_name", printer["name"]);
                metadata.insert("is_import", true);
                metadata.insert("preferred_process", printer["default_print_profile"]);
                if(m_machineMetaData.extruderDiameters.size()==0)
                    m_machineMetaData.extruderDiameters << printer["printer_variant"].toString().toFloat();
                m_machineMetaData.supportMaterialDiameters.append(1.75);
                QRegularExpression regex("\\s\\([0-9]\\.[0-9]\\snozzle\\)|\\s[0-9]\\.[0-9]\\snozzle");

                QString modifiedPresetName = printer["name"].toString().replace(regex, "");
                m_machineMetaData.codeName = modifiedPresetName;
                m_machineMetaData.baseName = modifiedPresetName;



            }
            inherits_json.insert("metadata", metadata);


            return true;
        };
         //保存机型文件
        type = "printer";
        if(printer_inherits_group!="")
        {
            QString inheritPrinterFile = findFiles(QCoreApplication::applicationDirPath()  + "/resources/thirdparty/", printer_inherits_group, "json").absoluteFilePath();
            QFile printerFile(inheritPrinterFile);
            if(printerFile.open(QIODevice::ReadOnly))
            {
                if (processLambda(printerFile))
                {
                    m_machineMetaData.baseName = data.baseName;
                    m_machineMetaData.codeName = data.codeName;
                    save2json(printer_inherits_group, defaultMachineFile(data.uniqueName(), true), type, inherits_json);
                }
            }
        }
        QJsonObject printer_json = inherits_json["printer"].toObject();
        QString producter = printer_json["producter"].toString();
        if (producter != "")
        {
            
            QList<QFileInfo> filaments = findProfileFiles(QCoreApplication::applicationDirPath()  + "/resources/thirdparty/profiles/" + producter + "/filament", "json");
            for (QFileInfo info : filaments)
            {
                inherits_json = QJsonObject();
                metadata = QJsonObject();
                QFile ifile(info.absoluteFilePath());
                if (ifile.open(QIODevice::ReadOnly))
                {
                    type = "materials";
                    if (processLambda(ifile))
                    {
                        QJsonObject filament = inherits_json["engine_data"].toObject();
                        int size = filament.size();

                        if (filament.contains("compatible_printers"))
                        {
                            QString compatible_printers = filament["compatible_printers"].toString();

                            if (compatible_printers.split(",").indexOf(printer_inherits_group) >= 0)
                            {
                                save2json(printer_inherits_group, info.absoluteFilePath(), type, inherits_json);
                            }
                        }

                    }
                    ifile.close();

                }
            }
            QList<QFileInfo> processes = findProfileFiles(QCoreApplication::applicationDirPath()  + "/resources/thirdparty/profiles/" + producter + "/process", "json");
            for (QFileInfo info : processes)
            {
                inherits_json = QJsonObject();
                metadata = QJsonObject();
                QFile ifile(info.absoluteFilePath());
                if (ifile.open(QIODevice::ReadOnly))
                {
                    type = "process";
                    if (processLambda(ifile))
                    {
                        QJsonObject process = inherits_json["engine_data"].toObject();

                        if (process.contains("compatible_printers"))
                        {
                            QString compatible_printers = process["compatible_printers"].toString();
                            //for (auto itr = compatible_printers.begin(); itr != compatible_printers.end(); ++itr) {
                                //qDebug()<<itr->toString();
                            if (compatible_printers.split(",").indexOf(printer_inherits_group) >= 0)
                            {
                                save2json(printer_inherits_group, info.absoluteFilePath(), type, inherits_json);
                            }
                            // }


                        }

                    }
                    ifile.close();

                }
                //QFile::copy(info.absoluteFilePath(),_pfpt_user_parampack +"/" + preset_name + "/Materials/" +info.fileName());
            }
        }
        
        
        return true;

    }
    bool ImportPresetCommand::process3MFPrinter(QString filename)
    {
        QuazipSubFunc structureFun = std::bind(&ImportPresetCommand::process3MFStructFile, this, std::placeholders::_1); 
        
        m_zipFile->open(filename);
        if(m_zipFile->openSubFile("Metadata/project_settings.config",structureFun))
        {
            
                    m_machineMetaData.isUser = true;
                    getKernel()->parameterManager()->importPrinterFromOrca(m_machineMetaData);
                    return true;
        }
        return false;
    }
}
