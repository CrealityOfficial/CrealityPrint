#include "cx3dprojectmanager.h"
#include "quazip/quazipfile.h"
#include "kernel/kernel.h"
#include <QtDebug>
#include <QMatrix4x4>
#include "cx3dexportjob.h"
#include <QtCore/QFileInfo>
#include "data/modelspace.h"
#include "interface/spaceinterface.h"
#include "interface/machineinterface.h"

#include <QStandardPaths>
#include "qtusercore/string/resourcesfinder.h"
#include "cxkernel/data/modelndataserial.h"
#include <sstream>
#include <QtXml/QDomDocument>
#include "external/interface/appsettinginterface.h"

namespace cx3d {
    void serializeDoubleData(QuaZipFile& file, const std::vector<std::vector<double>>& data, const QString& filename)
    {
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate, QuaZipNewInfo(filename))) {
            // 写入数据的行数和列数
            size_t numRows = data.size();
            file.write(reinterpret_cast<const char*>(&numRows), sizeof(numRows));
            for (const auto& row : data) {
                size_t numCols = row.size();
                file.write(reinterpret_cast<const char*>(&numCols), sizeof(numCols));
                if (numCols == 0) {
                    continue;
                }
                file.write(reinterpret_cast<const char*>(row.data()), numCols * sizeof(double));
            }
            file.close();
        }
    }

    void deserializeDoubleData(QuaZipFile& file, std::vector<std::vector<double>>& data)
    {
        data.clear();
        if (file.open(QIODevice::ReadOnly)) {
            // 读取数据的行数
            size_t numRows;
            file.read(reinterpret_cast<char*>(&numRows), sizeof(numRows));
            // 遍历每一行
            for (size_t i = 0; i < numRows; ++i) {
                // 读取每一行的列数
                size_t numCols;
                file.read(reinterpret_cast<char*>(&numCols), sizeof(numCols));
                // 创建当前行的向量
                std::vector<double> row(numCols);
                if (numCols == 0) {
                    continue;
                }
                // 读取当前行的数据
                file.read(reinterpret_cast<char*>(row.data()), numCols * sizeof(double));
                // 将当前行添加到二维向量中
                data.emplace_back(std::move(row));
            }
            file.close();
        }
    }

    void serializeStringData(QuaZipFile& file, const std::vector<std::vector<std::string>>& data, const QString& filename)
    {
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate, QuaZipNewInfo(filename))) {
            // 写入数据的行数和列数
            size_t numRows = data.size();
            file.write(reinterpret_cast<const char*>(&numRows), sizeof(numRows));
            for (const auto& row : data) {
                size_t numCols = row.size();
                file.write(reinterpret_cast<const char*>(&numCols), sizeof(numCols));
                for (const auto& str : row) {
                    size_t strSize = str.size(); // 获取字符串的长度
                    file.write(reinterpret_cast<const char*>(&strSize), sizeof(strSize));
                    file.write(str.c_str(), strSize); // 写入字符串数据
                }
            }
            file.close();
        }
    }

    void deserializeStringData(QuaZipFile& file, std::vector<std::vector<std::string>>& data)
    {
        data.clear();
        if (file.open(QIODevice::ReadOnly)) {
            // 读取数据的行数和列数
            size_t numRows;
            file.read(reinterpret_cast<char*>(&numRows), sizeof(numRows));
            data.resize(numRows);
            for (size_t i = 0; i < numRows; ++i) {
                size_t numCols;
                file.read(reinterpret_cast<char*>(&numCols), sizeof(numCols));
                data[i].resize(numCols);
                for (size_t j = 0; j < numCols; ++j) {
                    size_t strSize;
                    file.read(reinterpret_cast<char*>(&strSize), sizeof(strSize));
                    std::vector<char> buffer(strSize);
                    file.read(buffer.data(), strSize);
                    data[i][j] = std::string(buffer.data(), strSize);
                }
            }
            file.close();
        }
    }

    Cx3dFileProject::Cx3dFileProject(const QString& filePath, qtuser_core::Progressor* progressor, QObject* parent, bool isBackup, bool isWrite)
        : m_progress(progressor), QObject(parent)
    {
        m_quaZip = new QuaZip();
        m_quaZip->setUtf8Enabled(true);
        m_modelFile = new QuaZipFile();
        m_contentXml = new contentXML();

        m_projectPath = filePath;
        m_mode = isWrite;
        if (m_mode) {
            m_xmlWriter = new QXmlStreamWriter();
            m_quaZip->setZipName(m_projectPath);//设置zip 文件名
            if (!m_quaZip->open(QuaZip::mdCreate))//创建zip
            {
                qDebug() << "open QuaZip::mdCreate error!" << endl;
                m_quaZip->close();
                delete(m_quaZip);
                m_quaZip = nullptr;
                m_modelFile = nullptr;
                return;
            }
            if (m_quaZip) {
                m_quaZip->close();
                if (!m_quaZip->open(QuaZip::mdAdd))//打开ZIP文件，以便在存档中添加文件。
                {
                    qDebug() << "open QuaZip::mdAdd error!" << endl;
                    m_quaZip->close();
                    delete(m_quaZip);
                    m_quaZip = nullptr;
                    m_modelFile = nullptr;
                }
            }
            m_modelFile = new QuaZipFile(m_quaZip);
            m_backup = isBackup;
        } else {
            m_xmlReader = new QXmlStreamReader();
        }
    }

    Cx3dFileProject::~Cx3dFileProject()
    {
        if (m_mode) {
            if (m_quaZip != nullptr && m_quaZip->isOpen()) {
                m_xmlWriter->writeEndElement();
                m_xmlWriter->writeEndDocument();
            }

            if (m_modelFile != nullptr) {
                if (m_modelFile->isOpen()) {
                    m_modelFile->close();
                }
                if (m_modelFile != nullptr) {
                    delete m_modelFile;
                    m_modelFile = nullptr;
                }
            }
            if (m_quaZip != nullptr) {
                if (m_quaZip->isOpen()) {
                    m_quaZip->close();
                }
                if (m_quaZip != nullptr) {
                    delete m_quaZip;
                    m_quaZip = nullptr;
                }
            }
            if (m_xmlWriter != nullptr) {
                delete m_xmlWriter;
                m_xmlWriter = nullptr;
            }
        } else {
            if (m_modelFile != nullptr) {
                delete m_modelFile;
                m_modelFile = nullptr;
            }
            if (m_quaZip != nullptr) {
                delete m_quaZip;
                m_quaZip = nullptr;
            }
            if (m_xmlReader != nullptr) {
                delete m_xmlReader;
                m_xmlReader = nullptr;
            }

            /*for (auto& model : m_models) {
                if (model != nullptr) {
                    delete model;
                    model = nullptr;
                }
            }*/
            m_models.clear();
        }
    }

    void Cx3dFileProject::setSlicerType(const QString& type)
    {
        if (m_contentXml != nullptr) {
            m_contentXml->slicerType = type;
        }
    }

    void Cx3dFileProject::setFieldData(const FieldName& name, const QByteArray& data)
    {
        m_datas.emplace(name, data);
    }

    void Cx3dFileProject::setModelData(const QList<creative_kernel::ModelN*>& models)
    {
        for (const auto& m : models) {
            m_models.push_back(m);
        }
    }

    QString Cx3dFileProject::getSlicerType() const
    {
        if (m_contentXml != nullptr) {
            return m_contentXml->slicerType;
        }
        return QString();
    }

    QString Cx3dFileProject::getEngineType() const
    {
        QString ret;
        auto engine_type = creative_kernel::getEngineType();
        switch (engine_type)
        {
        case creative_kernel::EngineType::ET_CURA:
        {
            ret = "cura";
        }
            break;
        case creative_kernel::EngineType::ET_ORCA:
        {
            ret = "orca";
        }
            break;
        default:
            break;
        }
        return ret;
    }

    QString Cx3dFileProject::getEngineVersion() const
    {
        return creative_kernel::getEngineVersion().split("-").last();
    }

    void Cx3dFileProject::getAllEngineTypes(std::vector<QString>& types)
    {
        for (auto itr = m_engineSet.begin(); itr != m_engineSet.end(); ++itr) {
            types.emplace_back(*itr);
        }
    }

    void Cx3dFileProject::getFieldData(const FieldName& name, QByteArray& data)const
    {
        auto itr = m_datas.find(name);
        if (itr != m_datas.end()) {
            data = itr->second;
        }
    }

    void Cx3dFileProject::getModelData(QList<creative_kernel::ModelN*>& models)const
    {
        for (const auto& m : m_models) {
            models.push_back(m);
        }
    }

    void Cx3dFileProject::saveModelMesh(const QString& strName, const QString& strPathName, creative_kernel::ModelN* modeln)
    {
        if (m_modelFile->open(QIODevice::WriteOnly | QIODevice::Truncate, QuaZipNewInfo(strName, strPathName))) {
            trimesh::TriMesh* mesh = modeln->mesh();
            char header[80];
            memset(header, ' ', 80);
            m_modelFile->write(header, 80);//stl 头 80字节
            int nfaces = mesh->faces.size();
            m_modelFile->write((const char*)&nfaces, 4);//面片数
            for (size_t i = 0; i < mesh->faces.size(); i++) {
                float fbuf[12];
                trimesh::vec tn = mesh->trinorm(i);
                normalize(tn);
                fbuf[0] = tn[0];
                fbuf[1] = tn[1];
                fbuf[2] = tn[2];
                trimesh::ivec3 fn = mesh->faces[i];
                fbuf[3] = mesh->vertices[fn[0]][0];
                fbuf[4] = mesh->vertices[fn[0]][1];
                fbuf[5] = mesh->vertices[fn[0]][2];
                fbuf[6] = mesh->vertices[fn[1]][0];
                fbuf[7] = mesh->vertices[fn[1]][1];
                fbuf[8] = mesh->vertices[fn[1]][2];
                fbuf[9] = mesh->vertices[fn[2]][0];
                fbuf[10] = mesh->vertices[fn[2]][1];
                fbuf[11] = mesh->vertices[fn[2]][2];

                m_modelFile->write((const char*)&fbuf, 48);//数据
                const char end[2] = { 0, 0 };
                m_modelFile->write(end, 2);//00结束符
            }
            m_modelFile->close();
        }
    }

    void Cx3dFileProject::saveFieldData(const QByteArray& data, const QString& fieldFileName)
    {
        if (m_modelFile->open(QIODevice::WriteOnly | QIODevice::Truncate, QuaZipNewInfo(fieldFileName))) {
            m_modelFile->write(data);
            m_modelFile->close();
        }
    }

    void Cx3dFileProject::saveProjectXML(const QString& strName, const QString& strPathName)
    {
        if (m_modelFile->open(QIODevice::WriteOnly | QIODevice::Truncate, QuaZipNewInfo(strName, strPathName))) {
            //构造m_xmlWriter对象
            m_xmlWriter->setDevice(m_modelFile);//绑定输入输出设备
            m_xmlWriter->setCodec("utf-8");//设置格式：utf-8
            m_xmlWriter->setAutoFormatting(true);//自动格式化

            //开始写入xml内容
            m_xmlWriter->writeStartDocument("1.0", true);//开始文档 xml声明
            m_xmlWriter->writeComment("cx3d project manager");
            // 获取当前日期和时间
            QDateTime currentDateTime = QDateTime::currentDateTime();
            // 提取日期和时间,将日期和时间转换为 QString
            QString dateString = currentDateTime.toString("yyyy-MM-dd");
            QString timeString = currentDateTime.toString("hh:mm:ss");
            m_xmlWriter->writeComment("Time: " + dateString + " " + timeString);
            m_xmlWriter->writeComment("Author: ");
            m_xmlWriter->writeComment("***************parameter declaration***************");
            m_xmlWriter->writeComment("PARAM: including EXCLUDER、MACHINE、MATERIAL、PROCESS");
            m_xmlWriter->writeComment("MODLE: including MESH,SPREAD,SUPPORT,LAYER,PROCESSSETTING");
            m_xmlWriter->writeComment("PLATE: including ");
            m_xmlWriter->writeComment("***************************************************");
            m_xmlWriter->writeComment("***************engine type declaration***************");
            m_xmlWriter->writeComment("This is all the possible engine types currently known Provided by Wang Jiang:");
            m_xmlWriter->writeComment("{creality, cura, orca, prusa, bambu, ideamaker, superslicer, ffslicer, simplify, craftware}");
            m_xmlWriter->writeComment("***************************************************");
            m_xmlWriter->writeProcessingInstruction("xml-stylesheet type=\"text/css\" href=\"style.css\"");//处理指令
            m_xmlWriter->writeStartElement("workspace");//根元素workspace
            m_xmlWriter->writeAttribute("Version", m_contentXml->version);//根元素内添加一个属性
            m_xmlWriter->writeStartElement("resources");//子元素resources

            //添加 切片类型：FDM or DLP
            m_xmlWriter->writeStartElement("slicer");//slicertype
            m_xmlWriter->writeAttribute("type", m_contentXml->slicerType);
            {
                //根据不同的切片引擎添加挤出机、机型、耗材、工艺配置等参数
                m_xmlWriter->writeStartElement("Metadata");//Metadata
                const auto& engineInfo = m_contentXml->engineInfo;
                {
                    m_xmlWriter->writeStartElement(getEngineType());//orca
                    //添加 引擎版本配置
                    m_xmlWriter->writeTextElement("version", getEngineVersion());
                    //添加 excluder配置 machine配置 process配置 materials 配置
                    m_xmlWriter->writeStartElement("profile");//profile
                    m_xmlWriter->writeAttribute("url", engineInfo.profileUrl);
                    m_xmlWriter->writeEndElement();//结束profile
                    m_xmlWriter->writeEndElement();//结束orca
                }
                m_xmlWriter->writeEndElement();//结束Metadata
            }
            m_xmlWriter->writeEndElement();//结束slicertype 

            //添加 场景数据
            m_xmlWriter->writeStartElement("scene");//scene
            {
                //添加几何数据
                m_xmlWriter->writeStartElement("geometries");//geometries
                for (const auto& mptr : m_meshPtrs) {
                    m_xmlWriter->writeStartElement("geometry");//geometry
                    int index = mptr.second;
                    QString saveMeshPath = m_meshUrls[index];
                    m_xmlWriter->writeAttribute("id", QString::number(index));
                    m_xmlWriter->writeAttribute("url", saveMeshPath);
                    m_xmlWriter->writeEndElement();
                }
                m_xmlWriter->writeEndElement();//结束meshes

                //添加models数据
                m_xmlWriter->writeStartElement("models");//models
                const auto& modelInfos = m_contentXml->modelInfos;
                const int modelNums = modelInfos.size();
                for (size_t i = 0; i < modelNums; ++i) {
                    m_xmlWriter->writeStartElement("model");//model
                    const auto& modelTempInfo = modelInfos.at(i);
                    m_xmlWriter->writeStartElement("element");//element
                    m_xmlWriter->writeAttribute("name", modelTempInfo.modelName);
                    m_xmlWriter->writeAttribute("id", QString::number(i));
                    {
                        m_xmlWriter->writeStartElement("geometry");//geometry
                        m_xmlWriter->writeAttribute("id", QString::number(modelTempInfo.mesh.meshIndex));
                        m_xmlWriter->writeAttribute("value", QString::fromStdString(modelTempInfo.mesh.unidId));
                        m_xmlWriter->writeEndElement();//结束geometry 

                        m_xmlWriter->writeStartElement("transform");//transform
                        m_xmlWriter->writeStartElement("position");//position
                        QString X = QString::number(modelTempInfo.matrix.Position.x(), 'f', 7);
                        QString Y = QString::number(modelTempInfo.matrix.Position.y(), 'f', 7);
                        QString Z = QString::number(modelTempInfo.matrix.Position.z(), 'f', 7);
                        m_xmlWriter->writeAttribute("X", X);
                        m_xmlWriter->writeAttribute("Y", Y);
                        m_xmlWriter->writeAttribute("Z", Z);
                        m_xmlWriter->writeEndElement();//结束position 

                        m_xmlWriter->writeStartElement("scale");//Scale
                        X = QString::number(modelTempInfo.matrix.Scale.x(), 'f', 8);
                        Y = QString::number(modelTempInfo.matrix.Scale.y(), 'f', 8);
                        Z = QString::number(modelTempInfo.matrix.Scale.z(), 'f', 8);
                        m_xmlWriter->writeAttribute("X", X);
                        m_xmlWriter->writeAttribute("Y", Y);
                        m_xmlWriter->writeAttribute("Z", Z);
                        m_xmlWriter->writeEndElement();//结束Scale 

                        m_xmlWriter->writeStartElement("rotate");//Rotate
                        X = QString::number(modelTempInfo.matrix.Rotate.x(), 'f', 8);
                        Y = QString::number(modelTempInfo.matrix.Rotate.y(), 'f', 8);
                        Z = QString::number(modelTempInfo.matrix.Rotate.z(), 'f', 8);
                        QString scalar = QString::number(modelTempInfo.matrix.Rotate.scalar(), 'f', 8);
                        m_xmlWriter->writeAttribute("X", X);
                        m_xmlWriter->writeAttribute("Y", Y);
                        m_xmlWriter->writeAttribute("Z", Z);
                        m_xmlWriter->writeAttribute("scalar", scalar);
                        m_xmlWriter->writeEndElement();//结束Rotate 
                        m_xmlWriter->writeEndElement();//结束transform

                        m_xmlWriter->writeStartElement("colorIndex");//colorIndex
                        QString colorValue = QString::number(modelTempInfo.colorIndex);
                        m_xmlWriter->writeAttribute("value", colorValue);
                        m_xmlWriter->writeEndElement();//结束colorIndex 
                    }
                    m_xmlWriter->writeEndElement();//结束element
                    m_xmlWriter->writeStartElement("support");//support
                    m_xmlWriter->writeAttribute("url", modelTempInfo.support.supPath);
                    m_xmlWriter->writeAttribute("name", modelTempInfo.support.supName);
                    m_xmlWriter->writeEndElement();//结束support

                    m_xmlWriter->writeStartElement("spread");//spread
                    m_xmlWriter->writeAttribute("url", modelTempInfo.spread.sprPath);
                    m_xmlWriter->writeAttribute("name", modelTempInfo.spread.sprName);
                    m_xmlWriter->writeEndElement();//结束spread

                    m_xmlWriter->writeStartElement("adaptlayer");//layer
                    m_xmlWriter->writeAttribute("url", modelTempInfo.layer.layerPath);
                    m_xmlWriter->writeAttribute("name", modelTempInfo.layer.layerName);
                    m_xmlWriter->writeEndElement();//结束layer

                    m_xmlWriter->writeStartElement("objectset");//objectpara
                    m_xmlWriter->writeAttribute("url", modelTempInfo.setting.setPath);
                    m_xmlWriter->writeAttribute("name", modelTempInfo.setting.setName);
                    m_xmlWriter->writeEndElement();//结束objectpara
                    m_xmlWriter->writeEndElement();//结束model
                }
                m_xmlWriter->writeEndElement();//结束models

                //添加plates数据
                m_xmlWriter->writeStartElement("plates");//plates
                m_xmlWriter->writeStartElement("meta");//meta
                m_xmlWriter->writeAttribute("url", m_contentXml->platesUrl);
                m_xmlWriter->writeEndElement();//meta
                const auto& plateInfos = m_contentXml->plateInfos;
                for (const auto& plate : plateInfos) {
                    m_xmlWriter->writeStartElement("plate");//plate
                    m_xmlWriter->writeAttribute("plateName", "plateName");
                    m_xmlWriter->writeStartElement("para");//para
                    for (const auto& kvalue : plate.plateParameter) {
                        m_xmlWriter->writeStartElement(QString::fromStdString(kvalue.first));
                        m_xmlWriter->writeAttribute("plateName", QString::fromStdString(kvalue.second));
                        m_xmlWriter->writeEndElement(); //
                    }
                    m_xmlWriter->writeEndElement(); //para
                    /*m_xmlWriter->writeAttribute("plateName", plate.plateName);
                    m_xmlWriter->writeAttribute("cur_bed_type", plate.cur_bed_type);
                    m_xmlWriter->writeAttribute("print_sequence", plate.print_sequence);
                    m_xmlWriter->writeAttribute("first_layer_print_sequence", plate.first_layer_print_sequence);*/
                    m_xmlWriter->writeEndElement(); //plate
                }
                m_xmlWriter->writeEndElement();//plates
            }
            m_xmlWriter->writeEndElement();//结束secene

            m_xmlWriter->writeEndElement();//结束resources
            m_xmlWriter->writeEndElement();//结束根元素workspace
            m_xmlWriter->writeEndDocument();//结束文档
            m_modelFile->close();
        }
    }

    void Cx3dFileProject::loadProjectXML()
    {
        m_quaZip->setZipName(m_projectPath);
        if (m_quaZip->open(QuaZip::mdUnzip)) {
            m_contentXml->slicerType = "";
            m_quaZip->setCurrentFile("Content.xml");
            m_modelFile->setZip(m_quaZip);
            std::map<int, QString> meshMaps;
            if (m_modelFile->open(QIODevice::ReadOnly)) {
                QDomDocument doc;
                if (!doc.setContent(m_modelFile)) {
                    qDebug() << "parse xml error";
                    return;
                }

                auto docElem = doc.documentElement();
                if (!docElem.isNull() && "workspace" == docElem.tagName()) {
                    m_contentXml->version = docElem.attribute("Version");
                }

                QDomNode n = docElem.firstChild();
                while (!n.isNull()) {
                    auto e = n.toElement();
                    if (!e.isNull() && "resources" == e.tagName()) {
                        auto n = e.firstChild();
                        while (!n.isNull()) {
                            auto e = n.toElement();
                            if (!e.isNull() && "slicer" == e.tagName()) {
                                m_contentXml->slicerType = e.attribute("type");
                                auto n = e.firstChild();
                                while (!n.isNull()) {
                                    auto e = n.toElement();
                                    if (!e.isNull() && "Metadata" == e.tagName()) {
                                        auto n = e.firstChild();
                                        while (!n.isNull()) {
                                            auto e = n.toElement();
                                            QString engine = e.tagName();
                                            auto itr = m_engineSet.find(engine);
                                            if (!e.isNull() && itr != m_engineSet.end()) {
                                                auto n = e.firstChild();
                                                while (!n.isNull()) {
                                                    auto e = n.toElement();
                                                    if (!e.isNull() && "version" == e.tagName()) {
                                                    } else if (!e.isNull() && "profile" == e.tagName()) {
                                                        m_contentXml->engineInfo.profileUrl = e.attribute("url");
                                                    }
                                                    n = n.nextSibling();
                                                }
                                            }
                                            n = n.nextSibling();
                                        }
                                    }
                                    n = n.nextSibling();
                                }
                            } else if (!e.isNull() && "scene" == e.tagName()) {
                                auto n = e.firstChild();
                                while (!n.isNull()) {
                                    auto e = n.toElement();
                                    if (!e.isNull() && "geometries" == e.tagName()) {
                                        auto node_list = e.childNodes();
                                        for (size_t i = 0; i < node_list.size(); i++) {
                                            auto e = node_list.at(i).toElement();
                                            if (!e.isNull() && "geometry" == e.tagName()) {
                                                meshMaps.emplace(e.attribute("id").toInt(), e.attribute("url"));
                                            }
                                        }
                                    } else if (!e.isNull() && "models" == e.tagName()) {
                                        auto n = e.firstChild();
                                        while (!n.isNull()) {
                                            auto e = n.toElement();
                                            if (!e.isNull() && "model" == e.tagName()) {
                                                ModelInfo modelTempInfo;
                                                auto n = e.firstChild();
                                                while (!n.isNull()) {
                                                    auto e = n.toElement();
                                                    if (!e.isNull() && "element" == e.tagName()) {
                                                        QString elementName = e.attribute("name");
                                                        QString elementId = e.attribute("id");
                                                        modelTempInfo.modelName = elementName;
                                                        auto n = e.firstChild();
                                                        while (!n.isNull()) {
                                                            auto e = n.toElement();
                                                            if (!e.isNull() && "geometry" == e.tagName()) {
                                                                auto geometryId = e.attribute("id").toInt();
                                                                modelTempInfo.mesh.meshIndex = geometryId;
                                                                modelTempInfo.mesh.unidId = e.attribute("value").toStdString();
                                                                modelTempInfo.mesh.meshPath = meshMaps[geometryId];
                                                            } else if (!e.isNull() && "transform" == e.tagName()) {
                                                                auto n = e.firstChild();
                                                                while (!n.isNull()) {
                                                                    auto e = n.toElement();
                                                                    if (!e.isNull() && "position" == e.tagName()) {
                                                                        float X = e.attribute("X").toFloat();
                                                                        modelTempInfo.matrix.Position.setX(X);
                                                                        float Y = e.attribute("Y").toFloat();
                                                                        modelTempInfo.matrix.Position.setY(Y);
                                                                        float Z = e.attribute("Z").toFloat();
                                                                        modelTempInfo.matrix.Position.setZ(Z);
                                                                    } else if (!e.isNull() && "scale" == e.tagName()) {
                                                                        float X = e.attribute("X").toFloat();
                                                                        modelTempInfo.matrix.Scale.setX(X);
                                                                        float Y = e.attribute("Y").toFloat();
                                                                        modelTempInfo.matrix.Scale.setY(Y);
                                                                        float Z = e.attribute("Z").toFloat();
                                                                        modelTempInfo.matrix.Scale.setZ(Z);
                                                                    } else if (!e.isNull() && "rotate" == e.tagName()) {
                                                                        float X = e.attribute("X").toFloat();
                                                                        modelTempInfo.matrix.Rotate.setX(X);
                                                                        float Y = e.attribute("Y").toFloat();
                                                                        modelTempInfo.matrix.Rotate.setY(Y);
                                                                        float Z = e.attribute("Z").toFloat();
                                                                        modelTempInfo.matrix.Rotate.setZ(Z);
                                                                        float setScalar = e.attribute("scalar").toFloat();
                                                                        modelTempInfo.matrix.Rotate.setScalar(setScalar);
                                                                    }
                                                                    n = n.nextSibling();
                                                                }
                                                            }
                                                            else if (!e.isNull() && "colorIndex" == e.tagName()) {
                                                                auto colorIdx = e.attribute("value").toInt();
                                                                modelTempInfo.colorIndex = colorIdx;
                                                            }

                                                            n = n.nextSibling();
                                                        }
                                                    } else if (!e.isNull() && "support" == e.tagName()) {
                                                        modelTempInfo.support.supName = e.attribute("name");
                                                        modelTempInfo.support.supPath = e.attribute("url");
                                                    } else if (!e.isNull() && "spread" == e.tagName()) {
                                                        modelTempInfo.spread.sprName = e.attribute("name");
                                                        modelTempInfo.spread.sprPath = e.attribute("url");
                                                    } else if (!e.isNull() && "adaptlayer" == e.tagName()) {
                                                        modelTempInfo.layer.layerName = e.attribute("name");
                                                        modelTempInfo.layer.layerPath = e.attribute("url");
                                                    } else if (!e.isNull() && "objectset" == e.tagName()) {
                                                        modelTempInfo.setting.setName = e.attribute("name");
                                                        modelTempInfo.setting.setPath = e.attribute("url");
                                                    }
                                                    n = n.nextSibling();
                                                }
                                                m_contentXml->modelInfos.emplace_back(modelTempInfo);
                                            }
                                            n = n.nextSibling();
                                        }
                                    } else if (!e.isNull() && "plates" == e.tagName()) {
                                        auto n = e.firstChild();
                                        while (!n.isNull()) {
                                            auto e = n.toElement();
                                            if (!e.isNull() && "meta" == e.tagName()) {
                                                QString platesUrl = e.attribute("url");
                                                m_contentXml->platesUrl = platesUrl;
                                            }
                                            n = n.nextSibling();
                                        }
                                    }
                                    n = n.nextSibling();
                                }
                            }
                            n = n.nextSibling();
                        }
                    }
                    n = n.nextSibling();
                }
            }
            m_modelFile->close();
        }
        m_quaZip->close();
    }
    void cx3d::Cx3dFileProject::loadModelNData(const QString& dirctoryPath)
    {
        if (m_quaZip->open(QuaZip::mdUnzip)) {
            const auto& modelInfos = m_contentXml->modelInfos;
            const int modelNums = modelInfos.size();
            m_modelnDatas.reserve(modelNums);

            // geometry mesh data could possibly be reused
            loadGeometryMeshData(dirctoryPath);

            //加载网格信息
            for (int inx = 0; inx < modelNums; ++inx) 
            {
                const auto& tempModelInfo = modelInfos.at(inx);
                QString meshName = tempModelInfo.modelName;
                int geometryId = tempModelInfo.mesh.meshIndex;

                if (m_geometryMeshDatas.find(geometryId) == m_geometryMeshDatas.end())
                    continue;

                cxkernel::ModelNDataType type = cxkernel::ModelNDataType::mdt_project;
                cxkernel::ModelNDataPtr modelNData = cxkernel::createModelNData(m_geometryMeshDatas[geometryId], meshName, type);
                m_modelnDatas.push_back(modelNData);

                m_progress->progress(0.1 + 0.3 * (float)inx / (float)modelNums);

            }

            m_progress->progress(0.4);

            if (m_modelnDatas.isEmpty())
            {
                m_quaZip->close();
                return;
            }

            //加载涂抹信息
            for (int inx = 0; inx < modelNums; ++inx) {
                const auto& tempModelInfo = modelInfos.at(inx);
                cxkernel::ModelNDataPtr aModelNDataPtr = m_modelnDatas.at(inx);
                QString spreadPath = tempModelInfo.spread.sprPath;
                if (!m_quaZip->setCurrentFile(spreadPath)) {
                    QByteArray encodedPath = spreadPath.toLocal8Bit(); //将路径转换为本地编码的字节数组
                    QString filePath = QTextCodec::codecForLocale()->toUnicode(encodedPath); //使用本地编码进行转换
                    if (!m_quaZip->setCurrentFile(filePath))
                    {
                        continue;
                    }
                }
                m_modelFile->setZip(m_quaZip);
                std::vector<std::vector<std::string>> spreadDatas;
                deserializeStringData(*m_modelFile, spreadDatas);
                if (spreadDatas.size() == 3) {
                    aModelNDataPtr->colors = spreadDatas.at(0);
                    aModelNDataPtr->seams = spreadDatas.at(1);
                    aModelNDataPtr->supports = spreadDatas.at(2);

                } else {
                    spreadPath = dirctoryPath + "/Models/Spreads/" + tempModelInfo.spread.sprName;
                    if (!m_quaZip->setCurrentFile(spreadPath)) {
                        QByteArray encodedPath = spreadPath.toLocal8Bit(); //将路径转换为本地编码的字节数组
                        QString filePath = QTextCodec::codecForLocale()->toUnicode(encodedPath); //使用本地编码进行转换
                        m_quaZip->setCurrentFile(filePath);
                    }
                    m_modelFile->setZip(m_quaZip);
                    deserializeStringData(*m_modelFile, spreadDatas);
                    if (spreadDatas.size() == 3) {
                        aModelNDataPtr->colors = spreadDatas.at(0);
                        aModelNDataPtr->seams = spreadDatas.at(1);
                        aModelNDataPtr->supports = spreadDatas.at(2);
                    }
                }
                m_progress->progress(0.4 + 0.1 * (float)inx / (float)modelNums);
            }
            m_progress->progress(0.5);

            m_layerDatas.clear();

            //加载自适应层高信息
            for (int inx = 0; inx < modelNums; ++inx) {
                const auto& tempModelInfo = modelInfos.at(inx);
                QString adaptLayerPath = tempModelInfo.layer.layerPath;
                if (!m_quaZip->setCurrentFile(adaptLayerPath)) {
                    QByteArray encodedPath = adaptLayerPath.toLocal8Bit(); //将路径转换为本地编码的字节数组
                    QString filePath = QTextCodec::codecForLocale()->toUnicode(encodedPath); //使用本地编码进行转换
                    if (!m_quaZip->setCurrentFile(filePath))
                    {
                        continue;
                    }
                }
                m_modelFile->setZip(m_quaZip);
                std::vector<std::vector<double>> layerDatas;
                deserializeDoubleData(*m_modelFile, layerDatas);
                if (layerDatas.size() == 1) {
                    m_layerDatas.push_back(layerDatas.at(0));
                } else {
                    adaptLayerPath = dirctoryPath + "/Models/Layers/" + tempModelInfo.layer.layerName;
                    if (!m_quaZip->setCurrentFile(adaptLayerPath)) {
                        QByteArray encodedPath = adaptLayerPath.toLocal8Bit(); //将路径转换为本地编码的字节数组
                        QString filePath = QTextCodec::codecForLocale()->toUnicode(encodedPath); //使用本地编码进行转换
                        m_quaZip->setCurrentFile(filePath);
                    }
                    m_modelFile->setZip(m_quaZip);
                    deserializeDoubleData(*m_modelFile, layerDatas);
                    if (layerDatas.size() == 1) {
                        m_layerDatas.push_back(layerDatas.at(0));
                    }
                }
                m_progress->progress(0.5 + 0.1 * (float)inx / (float)modelNums);
            }
            m_progress->progress(0.6);

            m_objectSettings.clear();

            //加载对象参数设置
            for (int inx = 0; inx < modelNums; ++inx) {
                const auto& tempModelInfo = modelInfos.at(inx);
                QString objectSetPath = tempModelInfo.setting.setPath;
                std::shared_ptr<us::USettings> objectSet = std::make_shared<us::USettings>();
                objectSet->loadDefault(objectSetPath);
                if (!objectSet->isEmpty()) {
                    m_objectSettings.push_back(objectSet.get());
                } else {
                    objectSetPath = dirctoryPath + "/Models/ObjectSets/" + tempModelInfo.setting.setName;
                    objectSet->loadDefault(objectSetPath);
                    if (!objectSet->isEmpty()) {
                        m_objectSettings.push_back(objectSet.get());
                    }
                }
                m_progress->progress(0.6 + 0.1 * (float)inx / (float)modelNums);
            }

            m_quaZip->close();
        }
    }

    void cx3d::Cx3dFileProject::loadFieldData()
    {
        if (m_quaZip->open(QuaZip::mdUnzip)) {
            m_quaZip->setCurrentFile(m_contentXml->engineInfo.profileUrl + ".default");
            m_modelFile->setZip(m_quaZip);
            if (m_modelFile->open(QIODevice::ReadOnly)) {
                QByteArray dataSettings = m_modelFile->readAll();
                m_datas.emplace(FieldName::PARAM, dataSettings);
            }
            m_modelFile->close();

            m_quaZip->setCurrentFile(m_contentXml->engineInfo.profileUrl);
            if (m_modelFile->open(QIODevice::ReadOnly)) {
                QByteArray dataSettings = m_modelFile->readAll();
                m_datas.emplace(FieldName::PARAM, dataSettings);
            }
            m_modelFile->close();

            //加载多盘信息
            m_quaZip->setCurrentFile(m_contentXml->platesUrl + ".default");
            m_modelFile->setZip(m_quaZip);
            if (m_modelFile->open(QIODevice::ReadOnly)) {
                QByteArray dataSettings = m_modelFile->readAll();
                m_datas.emplace(FieldName::PARAM, dataSettings);
            }
            m_modelFile->close();

            m_quaZip->setCurrentFile(m_contentXml->platesUrl);
            if (m_modelFile->open(QIODevice::ReadOnly)) {
                QByteArray dataSettings = m_modelFile->readAll();
                m_datas.emplace(FieldName::PLATE, dataSettings);
            }
            m_modelFile->close();
        }
        m_quaZip->close();
    }

    void Cx3dFileProject::loadGeometryMeshData(const QString& directoryPath)
    {
        m_geometryMeshDatas.clear();

        const auto& modelInfos = m_contentXml->modelInfos;
        const int modelNums = modelInfos.size();

        //加载网格信息
        for (int inx = 0; inx < modelNums; ++inx) 
        {
            const auto& tempModelInfo = modelInfos.at(inx);
            QString meshPath = tempModelInfo.mesh.meshPath;
            QString meshName = tempModelInfo.modelName;
            int geometryId = tempModelInfo.mesh.meshIndex;

            // mesh data could possibly be reused
            if (m_geometryMeshDatas.find(geometryId) != m_geometryMeshDatas.end())
                continue;

            std::shared_ptr<trimesh::TriMesh> mesh(new trimesh::TriMesh());

            if (!m_quaZip->setCurrentFile(meshPath)) 
            {
                QByteArray encodedPath = meshPath.toLocal8Bit(); //将路径转换为本地编码的字节数组
                QString filePath = QTextCodec::codecForLocale()->toUnicode(encodedPath); //使用本地编码进行转换
                if (!m_quaZip->setCurrentFile(filePath))
                {
                    continue;
                }
            }

            m_modelFile->setZip(m_quaZip);
            if (m_modelFile->open(QIODevice::ReadOnly)) 
            {
                char head[80];
                m_modelFile->read(head, 80);
                int ifacets = 0;
                m_modelFile->read((char*)&ifacets, sizeof(int));
                mesh->faces.reserve(ifacets);
                mesh->vertices.reserve(3 * ifacets);
                for (int i = 0; i < ifacets; ++i) 
                {
                    float fbuf[48];
                    m_modelFile->read((char*)fbuf, 48);
                    int v = mesh->vertices.size();
                    mesh->vertices.push_back(trimesh::point(fbuf[3], fbuf[4], fbuf[5]));
                    mesh->vertices.push_back(trimesh::point(fbuf[6], fbuf[7], fbuf[8]));
                    mesh->vertices.push_back(trimesh::point(fbuf[9], fbuf[10], fbuf[11]));
                    mesh->faces.push_back(trimesh::TriMesh::Face(v, v + 1, v + 2));
                    char end[2];
                    m_modelFile->read(end, 2);//00结束符
                }

                m_modelFile->close();
            }
            else 
            {
                meshPath = directoryPath + "/Models/Geometrys/" + meshName;
                if (!QFile(meshPath).exists()) {
                    continue;
                }
                cxkernel::ModelNDataSerial serial;
                serial.load(meshPath, nullptr);
                mesh = serial.getData()->mesh;
            }

            m_geometryMeshDatas[geometryId] = mesh;

        }
    }

    bool compareVersions(const QString& current, const QString& version = QStringLiteral("1.0.0"))
    {
        QStringList parts1 = current.split('.');
        QStringList parts2 = version.split('.');
        int size = qMax(parts1.size(), parts2.size());
        for (int i = 0; i < size; ++i) {
            int part1 = (i < parts1.size()) ? parts1[i].toInt() : 0;
            int part2 = (i < parts2.size()) ? parts2[i].toInt() : 0;
            if (part1 < part2) {
                return false;
            } else if (part1 > part2) {
                return true;
            }
        }
        return true;
    }

    void Cx3dFileProject::saveCx3dProject(const QString& version)
    {
        // To Do!!
        return;

        if (m_modelFile == nullptr) return;
        if (!m_backup) m_progress->progress(0.0);
        if (compareVersions(version) && m_quaZip->isOpen()) {
            m_contentXml->version = version;
            QString engineType = "Metadata/" + getEngineType() + getEngineVersion(); //使用引擎名+版本号命名文件夹
            QString paramsFile = qtuser_core::getOrCreateAppDataLocation("tmpProject/" + engineType + "/Parameters");
            QString platesFile = qtuser_core::getOrCreateAppDataLocation("tmpProject/" + engineType + "/Plates");

            m_contentXml->engineInfo.profileUrl = "./" + engineType + "/Parameters/params.default";
            m_contentXml->platesUrl = "./" + engineType + "/Plates/plates.default";
            for (const auto& mdata : m_datas) {
                const FieldName& name = mdata.first;
                const QByteArray& data = mdata.second;
                switch (name) {
                    //挤出机 机型 工艺 耗材等数据
                case FieldName::PARAM:
                {
                    QString parasPath = paramsFile + "/" + "params.default";
                    QFileInfo paraInfo(parasPath);
                    if (paraInfo.exists()) {
                        saveFieldData(data, QString("./" + engineType + "/Parameters/params.default"));
                    }
                    if (m_backup) {
                        saveFieldData(data, QString("./" + engineType + "/Parameters/params.default"));
                    } else {
                        saveFieldData(data, m_contentXml->engineInfo.profileUrl);
                    }
                }
                break;
                //多盘场景的数据
                case FieldName::PLATE:
                {
                    QString platesPath = platesFile + "/" + "plates.default";
                    QFileInfo plateInfo(platesPath);
                    if (plateInfo.exists()) {
                        saveFieldData(data, QString("./" + engineType + "/Plates/plates.default"));
                    }
                    if (m_backup) {
                        saveFieldData(data, QString("./" + engineType + "/Plates/plates.default"));
                    } else {
                        saveFieldData(data, m_contentXml->platesUrl);
                    }
                }
                break;
                default:
                    break;
                }
            }
            if (!m_backup) m_progress->progress(0.1);

            const int modelNums = m_models.size();
            QString geometrysFile = qtuser_core::getOrCreateAppDataLocation("tmpProject/Models/Geometrys");
            QString supportsFile = qtuser_core::getOrCreateAppDataLocation("tmpProject/Models/Supports");
            QString spreadsFile = qtuser_core::getOrCreateAppDataLocation("tmpProject/Models/Spreads");
            QString layersFile = qtuser_core::getOrCreateAppDataLocation("tmpProject/Models/Layers");
            QString objectSetsFile = qtuser_core::getOrCreateAppDataLocation("tmpProject/Models/ObjectSets");

            for (int inx = 0; inx < modelNums; ++inx) {
                ModelInfo modelTempInfo;
                const auto& model = m_models[inx];
                modelTempInfo.matrix.Position = model->localPosition();
                modelTempInfo.matrix.Scale = model->localScale();
                modelTempInfo.matrix.Rotate = model->localQuaternion();
                modelTempInfo.colorIndex = model->defaultColorIndex();

                QString strName = model->objectName();
                QFileInfo fileInfo(strName);
                QString suffixName = fileInfo.completeSuffix();
                QString baseName = fileInfo.completeBaseName();
                QString meshName = suffixName != ".stl" ? baseName + ".stl" : strName;
                QString meshPath = fileInfo.path() + QString("/Models/Geometrys/");
                QString filePathName = meshPath + meshName;
                QString directPath = m_projectPath.left(m_projectPath.lastIndexOf("/"));
                //网格数据
                //判断是否存在内存地址一样的mesh,避免重复存储。
                auto meshPtr = model->meshptr();
                std::stringstream ss;
                ss << meshPtr; bool hasFound = false;
                std::string address = ss.str();
                auto foundItr = m_meshPtrs.find(address);
                if (foundItr != m_meshPtrs.end()) {
                    hasFound = true;
                    modelTempInfo.mesh.unidId = foundItr->first;
                    modelTempInfo.mesh.meshIndex = foundItr->second;
                } else {
                    modelTempInfo.mesh.unidId = address;
                    modelTempInfo.mesh.meshIndex = m_meshPtrs.size();
                    m_meshPtrs.emplace(address, m_meshPtrs.size());
                    m_meshUrls.emplace(m_meshPtrs.size() - 1, filePathName);
                }
                if (!m_backup) {
                    modelTempInfo.mesh.meshPath = filePathName;
                    modelTempInfo.modelName = meshName;
                    if (!hasFound) {
                        saveModelMesh(filePathName, meshName, model);
                    }
                } else {
                    filePathName = "./Models/Geometrys/" + meshName;
                    modelTempInfo.mesh.meshPath = filePathName;
                    modelTempInfo.modelName = meshName;
                    if (!hasFound) {
                        saveModelMesh(filePathName, meshName, model);
                    }
                }

                //支撑数据
                QString supName = suffixName != ".sup" ? baseName + ".sup" : strName;
                QString supPath = fileInfo.path() + QString("/Models/Supports/");
                modelTempInfo.support.supPath = supPath + supName;
                modelTempInfo.support.supName = supName;

                //涂抹数据
                QString spreadName = suffixName != ".spr" ? baseName + ".spr" : strName;
                QString spreadPath = fileInfo.path() + QString("/Models/Spreads/");
                modelTempInfo.spread.sprPath = spreadPath + spreadName;
                modelTempInfo.spread.sprName = spreadName;
                const std::vector<std::string>& facetColors = model->getColors2Facets();
                const std::vector<std::string>& facetSeams = model->getSeam2Facets();
                const std::vector<std::string>& facetSupports = model->getSupport2Facets();
                std::vector<std::vector<std::string>> spreadDatas;
                spreadDatas.emplace_back(facetColors);
                spreadDatas.emplace_back(facetSeams);
                spreadDatas.emplace_back(facetSupports);
                auto IsEmpty = [&](const std::vector<std::string>& strs) {
                    bool result = true;
                    for (const auto& str : strs) {
                        if (!str.empty()) {
                            result = false;
                            break;
                        }
                    }
                    return result;
                };
                if (!m_backup) {
                    QString spreadPathName = spreadPath + spreadName;
                    if (!IsEmpty(facetColors) || !IsEmpty(facetSeams) || !IsEmpty(facetSupports)) {
                        serializeStringData(*m_modelFile, spreadDatas, spreadPathName);
                    }
                } else {
                    QString spreadPathName = "./Models/Spreads/" + spreadName;
                    if (!IsEmpty(facetColors) || !IsEmpty(facetSeams) || !IsEmpty(facetSupports)) {
                        serializeStringData(*m_modelFile, spreadDatas, spreadPathName);
                    }
                }

                //可变层高
                QString adaptName = suffixName != ".lay" ? baseName + ".lay" : strName;
                QString adaptPath = fileInfo.path() + QString("/Models/Layers/");
                modelTempInfo.layer.layerPath = adaptPath + adaptName;
                modelTempInfo.layer.layerName = adaptName;
                const std::vector<double>& adaptLayers = model->layerHeightProfile();
                std::vector<std::vector<double>> layerDatas;
                layerDatas.emplace_back(adaptLayers);
                if (!m_backup) {
                    QString adaptPathName = adaptPath + adaptName;
                    if (!adaptLayers.empty()) {
                        serializeDoubleData(*m_modelFile, layerDatas, adaptPathName);
                    }
                } else {
                    QString adaptPathName = "./Models/Layers/" + adaptName;
                    if (!adaptLayers.empty()) {
                        serializeDoubleData(*m_modelFile, layerDatas, adaptPathName);
                    }
                }

                //对象参数设置
                QString setName = suffixName != ".set" ? baseName + ".set" : strName;
                QString setPath = fileInfo.path() + QString("/Models/ObjectSets/");
                modelTempInfo.setting.setPath = setPath + setName;
                modelTempInfo.setting.setName = setName;
                us::USettings* objectSet = model->setting();
                if (!m_backup) {
                    QString setPathName = setPath + setName;
                    objectSet->saveAsDefault(setPathName);
                } else {
                    QString setPathName = "./Models/ObjectSets/" + setName;
                    objectSet->saveAsDefault(setPathName);
                }
                m_contentXml->modelInfos.push_back(modelTempInfo);
                if (!m_backup) m_progress->progress(0.1f + 0.7f * (float)inx / (float)modelNums);
            }
            if (!m_backup) m_progress->progress(0.8);
            QString contentName = "Content.xml";
            QFileInfo fileInfo(m_projectPath);
            QString zipPathName = fileInfo.path();
            QString contentPath = zipPathName + "/" + contentName;
            saveProjectXML(contentName, contentPath);
            m_quaZip->close();
        }
        if (!m_backup) m_progress->progress(1.0);
    }

    void Cx3dFileProject::readCx3dProject()
    {
        m_progress->progress(0.0);
        loadProjectXML();
        if (!m_contentXml->slicerType.isEmpty()) {

            /*if (!creative_kernel::checkMachineExist(m_contentXml->machine.replace(".default", ""))) {
                if (m_progress) {
                    m_progress->failed("No matching printer found, cannot import project file");
                }
            }*/
            m_progress->progress(0.1);
            QString directFile = m_projectPath.left(m_projectPath.lastIndexOf("/"));
            QFileInfo fileInfo(m_projectPath);
            //获取工程文件名
            QString fileName = fileInfo.completeBaseName();
            QString directPath = directFile + "/" + fileName;
            //加载modelN数据
            loadModelNData(directPath);
            m_progress->progress(0.9);
            //加载界面配置参数及多盘数据
            loadFieldData();
        }
        else
        {
            m_progress->failed("");
        }
        m_progress->progress(1.0);
    }

}