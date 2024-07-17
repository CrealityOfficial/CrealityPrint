#include "cx3dwriter.h"
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

using namespace creative_kernel;

bool isBig()
{
    int a = 1;
    char* p = (char*)&a;
    if (*p == 1)
        return false;//小端
    else
        return true;//大端 
}

Cx3dWriter::Cx3dWriter(const QList<creative_kernel::ModelN*>& allModelns, QString path,qtuser_core::Progressor* progressor,QObject *parent, bool isBackup)
    : m_progress(progressor),QObject(parent)
{
    m_contentxml.sliceType = "FDM";
    m_quaZip = new QuaZip();
    m_quaZip->setZipName(path);//设置zip 文件名
    if (!m_quaZip->open(QuaZip::mdCreate))//创建zip
    {
        qDebug() << "open QuaZip::mdCreate error!" << endl;
        m_quaZip->close();
        delete(m_quaZip);
        m_quaZip = nullptr;
        m_pModelFile = nullptr;
        return;
    }
    if (m_quaZip)
    {
        m_quaZip->close();
        if (!m_quaZip->open(QuaZip::mdAdd))//打开ZIP文件，以便在存档中添加文件。
        {
            qDebug() << "open QuaZip::mdAdd error!" << endl;
            m_quaZip->close();
            delete(m_quaZip);
            m_quaZip = nullptr;
            m_pModelFile = nullptr;
        }
    }
    m_pModelFile = new QuaZipFile(m_quaZip);
    QFileInfo fileInfo(path);
    saveProfile(fileInfo.path());
    //m_contentxml.iniPathName = strPathName;

    for (size_t i = 0; i < allModelns.size(); i++)
    {
        modelMatrix mMatrixTemp;
        mMatrixTemp.Position = allModelns.at(i)->localPosition();
        mMatrixTemp.Scale = allModelns.at(i)->localScale();
        mMatrixTemp.Rotate = allModelns.at(i)->localQuaternion();
		/* mMatrixTemp.supName = allModelns.at(i)->objectName();
		 mMatrixTemp.supName.replace(".stl",".sup");*/
		QString strName = allModelns.at(i)->objectName();
        QFileInfo fileInfo(strName);
		mMatrixTemp.supName = fileInfo.suffix()!=".stl"? strName + ".sup" :fileInfo.baseName() + ".sup";
        m_contentxml.meshMatrix.push_back(mMatrixTemp);

        //QString strName = allModelns.at(i)->objectName();
        QString croped_fileName = fileInfo.suffix() != ".stl" ? strName:fileInfo.baseName() + ".stl";
        if (isBackup)
        {
            QFileInfo fileInfo(allModelns.at(i)->getSerialName());
            QString filename = fileInfo.fileName();
            m_contentxml.meshPathName.push_back(filename);
            m_contentxml.meshName.push_back(croped_fileName);
        }
        else {
            m_contentxml.meshPathName.push_back(croped_fileName);
            m_contentxml.meshName.push_back(croped_fileName);
        }
       

        QString strPathName = fileInfo.path() + "/" + croped_fileName;

        QByteArray cdata = strPathName.toLocal8Bit();
        std::string strSTLPath(cdata);
        //allModelns.at(i)->mesh()->write(strSTLPath.c_str());
        if (!isBackup)
        {
            saveSTL(croped_fileName, strPathName, allModelns.at(i));
        }
    }

    //strName = fileInfo.fileName().replace(".zip", ".XML");
    QString strName = "Content.xml";
    QString strPathName = fileInfo.path() + "/" + strName;
    saveQXML(strName, strPathName);
}
Cx3dWriter::~Cx3dWriter()
{
    if (m_quaZip != NULL && m_quaZip->isOpen())
    {
        m_xmlWriter->writeEndElement();
        m_xmlWriter->writeEndDocument();
    }
    
    if (m_pModelFile != NULL && m_pModelFile->isOpen())
    {
        m_pModelFile->close();
        if (m_pModelFile != NULL)
        {
            delete m_pModelFile;
            m_pModelFile = NULL;
        }
        
    }
    if (m_quaZip != NULL && m_quaZip->isOpen())
    {
        m_quaZip->close();
        if (m_quaZip != NULL)
        {
            delete m_quaZip;
            m_quaZip = NULL;
        }
    }  
}

//start lisugui 2020-11-5 解决_cx3d 问题，暂时不加机型
void Cx3dWriter::saveProfile(QString strPath)
{
    QString strMachinesPath = qtuser_core::getOrCreateAppDataLocation("") + "/Machines";
    QString strProfilesPath = qtuser_core::getOrCreateAppDataLocation("") + "/Profiles";
    QString strMaterialsPath = qtuser_core::getOrCreateAppDataLocation("") + "/Materials";
    QString strExtrudersPath = qtuser_core::getOrCreateAppDataLocation("") + "/Extruders";
    QString currentMachine = currentMachineName();
    QString currentProfile = creative_kernel::currentProfile();
    QStringList currentProfiles = creative_kernel::currentProfiles();
    QString strName = currentMachine.replace("_CX3D", "") + ".default";
    QString name = currentProfile + ".default";

    //
    QStringList currentExtruders = creative_kernel::currentExtruders();

    //
    QStringList currentMaterials = creative_kernel::currentMaterials();


    m_contentxml.machine = strName;
    m_contentxml.profile = name;

    //wangwenbin add 20211109
    //hemiao add 20230321
    //save machine
    saveSettings("Machines/"+strName, strMachinesPath + "/" + strName);
    //save extruder
    Q_FOREACH(QString extruder, currentExtruders)
    {
        QFileInfo extruderInfo(extruder);
        if (extruderInfo.exists())
        {
            saveSettings("Extruders/" + extruderInfo.fileName(), extruder);
        }
        QFileInfo coverextruderInfo(extruder+".cover");
        if (coverextruderInfo.exists())
        {
            saveSettings("Extruders/" + extruderInfo.fileName() + ".cover", extruder);
        }
    }
    //save profiles
    Q_FOREACH(QString profile, currentProfiles)
    {
        QString profilePath = strProfilesPath + "/" + currentMachine + "/" + profile + ".default";
        QFileInfo profileInfo(profilePath);
        if (profileInfo.exists())
        {
            saveSettings("Profiles/" + currentMachine + "/" + profile + ".default", profilePath);
        }
        QString coverProfilePath = strProfilesPath + "/" + currentMachine + "/" + profile + ".default.cover";
        QFileInfo coverProfileInfo(coverProfilePath);
        if (coverProfileInfo.exists())
        {
            saveSettings("Profiles/"+ currentMachine+"/"+profile+".default.cover", coverProfilePath);
        }
    }
    //save materials
    Q_FOREACH(QString material, currentMaterials)
    {
        QString profilePath = strMaterialsPath + "/" + currentMachine + "/" + material + ".default";
        QFileInfo profileInfo(profilePath);
        if (profileInfo.exists())
        {
            saveSettings("Materials/" + currentMachine + "/" + material + ".default", profilePath);
        }
        QString coverProfilePath = strMaterialsPath + "/" + currentMachine + "/" + material + ".default.cover";
        QFileInfo coverProfileInfo(coverProfilePath);
        if (coverProfileInfo.exists())
        {
            saveSettings("Materials/" + currentMachine + "/" + material + ".default.cover", coverProfilePath);
        }
        m_contentxml.materials += material+";";
    }
    

}
//end

void Cx3dWriter::saveSettings(QString strName, QString strPathName)
{
    QFile sourceFile;
    sourceFile.setFileName(strPathName);
	if (!sourceFile.open(QIODevice::ReadOnly)) 
    {
		qDebug() << "error ....";
        return ;
	}

	if (m_pModelFile->open(QIODevice::WriteOnly | QIODevice::Truncate, QuaZipNewInfo(strName, strPathName)))
    {
        m_pModelFile->write(sourceFile.readAll());
        m_pModelFile->close();
	}
    sourceFile.close();
}

void Cx3dWriter::saveSTL(QString strName, QString strPathName, creative_kernel::ModelN* modeln)
{
    if (m_pModelFile->open(QIODevice::WriteOnly | QIODevice::Truncate, QuaZipNewInfo(strName, strPathName)))
    {
        trimesh::TriMesh* mesh = modeln->mesh();
        char header[80];
	    memset(header, ' ', 80);
        m_pModelFile->write(header, 80);//stl 头 80字节

        int nfaces = mesh->faces.size();
        m_pModelFile->write((const char*)&nfaces, 4);//面片数

        for (size_t i = 0; i < mesh->faces.size(); i++)
        {
            float fbuf[12];
            trimesh::vec tn = mesh->trinorm(i);
            normalize(tn);
            fbuf[0] = tn[0];
            fbuf[1] = tn[1];
            fbuf[2] = tn[2];
            fbuf[3] = mesh->vertices[mesh->faces[i][0]][0];
            fbuf[4] = mesh->vertices[mesh->faces[i][0]][1];
            fbuf[5] = mesh->vertices[mesh->faces[i][0]][2];
            fbuf[6] = mesh->vertices[mesh->faces[i][1]][0];
            fbuf[7] = mesh->vertices[mesh->faces[i][1]][1];
            fbuf[8] = mesh->vertices[mesh->faces[i][1]][2];
            fbuf[9] = mesh->vertices[mesh->faces[i][2]][0];
            fbuf[10] = mesh->vertices[mesh->faces[i][2]][1];
            fbuf[11] = mesh->vertices[mesh->faces[i][2]][2];

            m_pModelFile->write((const char*)&fbuf, 48);//数据
            const char end[2] = { 0, 0 };
            m_pModelFile->write(end, 2);//00结束符
        }
        m_pModelFile->close();
    }
}

void Cx3dWriter::saveQXML(QString strName, QString strPathName)
{
    if (m_pModelFile->open(QIODevice::WriteOnly | QIODevice::Truncate, QuaZipNewInfo(strName, strPathName)))
    {
        //构造m_xmlWriter对象
        m_xmlWriter = new QXmlStreamWriter();
        m_xmlWriter->setDevice(m_pModelFile);//绑定输入输出设备
        m_xmlWriter->setCodec("utf-8");//设置格式：utf-8
        m_xmlWriter->setAutoFormatting(true);//自动格式化

        //开始写入xml内容
        m_xmlWriter->writeStartDocument("1.0", true);//开始文档 xml声明
        m_xmlWriter->writeComment("wwb 2020-09-05");
        m_xmlWriter->writeProcessingInstruction("xml-stylesheet type=\"text/css\" href=\"style.css\"");//处理指令
        m_xmlWriter->writeStartElement("workspace");//根元素workspace
        m_xmlWriter->writeAttribute("Version", "1.0");//根元素内添加一个属性
        m_xmlWriter->writeStartElement("resources");//子元素resources

        //添加 切片类型：FDM or DLP
        m_xmlWriter->writeStartElement("slice");//slicetype
        m_xmlWriter->writeAttribute("type", m_contentxml.sliceType);
        m_xmlWriter->writeEndElement();//结束slicetype 
        
        //添加 machine配置
        m_xmlWriter->writeStartElement("machine");//machine
        m_xmlWriter->writeAttribute("url", m_contentxml.machine);
        m_xmlWriter->writeEndElement();//结束machine 

        //添加 profile 配置
        m_xmlWriter->writeStartElement("profile");//profile
        m_xmlWriter->writeAttribute("url", m_contentxml.profile);
        m_xmlWriter->writeEndElement();//结束profile 

         //添加 materials 配置
        m_xmlWriter->writeStartElement("materials");//profile
        m_xmlWriter->writeAttribute("url", m_contentxml.materials);
        m_xmlWriter->writeEndElement();//结束profile 
        //添加mesh 数据
        m_xmlWriter->writeStartElement("object");//object
        m_xmlWriter->writeAttribute("type", "meshs");
        for (size_t i = 0; i < m_contentxml.meshMatrix.size(); i++)
        {
            m_xmlWriter->writeStartElement("mesh");//mesh

            m_xmlWriter->writeStartElement("meshPathName");//meshPathName
            m_xmlWriter->writeAttribute("url", m_contentxml.meshPathName.at(i));
            m_xmlWriter->writeAttribute("name", m_contentxml.meshName.at(i));
            m_xmlWriter->writeEndElement();//结束meshPathName 

            m_xmlWriter->writeStartElement("position");//position
            QString X = QString::number(m_contentxml.meshMatrix.at(i).Position.x(), 'f', 7);
            QString Y = QString::number(m_contentxml.meshMatrix.at(i).Position.y(), 'f', 7);
            QString Z = QString::number(m_contentxml.meshMatrix.at(i).Position.z(), 'f', 7);
            m_xmlWriter->writeAttribute("X", X);
            m_xmlWriter->writeAttribute("Y", Y);
            m_xmlWriter->writeAttribute("Z", Z);
            m_xmlWriter->writeEndElement();//结束position 

            m_xmlWriter->writeStartElement("scale");//Scale
            X = QString::number(m_contentxml.meshMatrix.at(i).Scale.x(), 'f', 8);
            Y = QString::number(m_contentxml.meshMatrix.at(i).Scale.y(), 'f', 8);
            Z = QString::number(m_contentxml.meshMatrix.at(i).Scale.z(), 'f', 8);
            m_xmlWriter->writeAttribute("X", X);
            m_xmlWriter->writeAttribute("Y", Y);
            m_xmlWriter->writeAttribute("Z", Z);
            m_xmlWriter->writeEndElement();//结束Scale 

            m_xmlWriter->writeStartElement("rotate");//Rotate
            X = QString::number(m_contentxml.meshMatrix.at(i).Rotate.x(), 'f', 8);
            Y = QString::number(m_contentxml.meshMatrix.at(i).Rotate.y(), 'f', 8);
            Z = QString::number(m_contentxml.meshMatrix.at(i).Rotate.z(), 'f', 8);
            QString scalar = QString::number(m_contentxml.meshMatrix.at(i).Rotate.scalar(), 'f', 8);
            m_xmlWriter->writeAttribute("X", X);
            m_xmlWriter->writeAttribute("Y", Y);
            m_xmlWriter->writeAttribute("Z", Z);
            m_xmlWriter->writeAttribute("scalar", scalar);
            m_xmlWriter->writeEndElement();//结束Rotate 

            m_xmlWriter->writeStartElement("support");//support
            m_xmlWriter->writeAttribute("name", m_contentxml.meshMatrix.at(i).supName);
            m_xmlWriter->writeEndElement();//结束support 

            m_xmlWriter->writeEndElement();//结束 mesh
        }
        m_xmlWriter->writeEndElement();//结束object 
        m_xmlWriter->writeEndElement();//结束resources
        m_xmlWriter->writeEndElement();//结束根元素workspace
        m_xmlWriter->writeEndDocument();//结束文档
        m_pModelFile->close();
    }
}

int Cx3dWriter::addMesh()
{
    int modelid = 0;
    m_xmlWriter->writeStartElement("object");
    m_xmlWriter->writeAttribute("id",QString::number(modelid));
    m_xmlWriter->writeAttribute("type","model");
	trimesh::TriMesh* mesh = nullptr;
    mesh2DocObject(mesh);

    m_xmlWriter->writeStartElement("components");
    {
		int id = 0;
		QMatrix4x4 m;
        m_xmlWriter->writeStartElement("component");
        m_xmlWriter->writeAttribute("objectid",QString::number(id));
        m_xmlWriter->writeAttribute("transform",convertMatrixToString(m));
        m_xmlWriter->writeEndElement();

    }
    m_xmlWriter->writeEndElement();
    m_xmlWriter->writeEndElement();
    return modelid;
}
QString Cx3dWriter::convertMatrixToString(QMatrix4x4 matrix)
{
    QString ms;
    const float *f = matrix.constData();
    for(int i=0;i<16;i++)
    {
        ms.append(QString::number(f[i]));
        ms.append(" ");
    }
    return ms;
}
void Cx3dWriter::mesh2DocObject(trimesh::TriMesh *mesh)
{
    float progress=0;
    float orgProcess=0;
    m_progress->progress(0);
    m_xmlWriter->writeStartElement("mesh");
    m_xmlWriter->writeStartElement("vertices");
    long verticesize = mesh->vertices.size();
    for (size_t i = 0; i < verticesize; i++) {
        m_xmlWriter->writeStartElement("vertex");
        m_xmlWriter->writeAttribute("x",QString::number(mesh->vertices[i][0]));
        m_xmlWriter->writeAttribute("y",QString::number(mesh->vertices[i][1]));
        m_xmlWriter->writeAttribute("z",QString::number(mesh->vertices[i][2]));
        m_xmlWriter->writeEndElement();

        if(i%1000==0)
        {
            progress = i*0.5 / verticesize;
            if (m_progress->interrupt())
            {
                break;
            }
            m_progress->progress(progress);
        }
    }
    m_xmlWriter->writeEndElement();

    m_progress->progress(0.5);
    m_xmlWriter->writeStartElement("triangles");
    long facesize = mesh->faces.size();
    for (size_t i = 0; i < facesize; i++) {
        m_xmlWriter->writeStartElement("triangle");
        m_xmlWriter->writeAttribute("v1",QString::number(mesh->faces[i][0]));
        m_xmlWriter->writeAttribute("v2",QString::number(mesh->faces[i][1]));
        m_xmlWriter->writeAttribute("v3",QString::number(mesh->faces[i][2]));
        m_xmlWriter->writeEndElement();
        if(i%1000==0)
        {
            progress = i*0.5 / facesize + 0.5 ;
            if (m_progress->interrupt())
            {
                break;
            }
            m_progress->progress(progress);
        }

    }
    m_xmlWriter->writeEndElement();
    m_xmlWriter->writeEndElement();
    m_progress->progress(1.0);
}