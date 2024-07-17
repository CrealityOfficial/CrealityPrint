#include "cx3drenderex.h"
#include <QtCore/QUrl>
#include "trimesh2/TriMesh.h"
#include <QStandardPaths>
#include <qdir.h>
#include "interface/machineinterface.h"
#include "cxkernel/data/modelndataserial.h"

Cx3dRenderEx::Cx3dRenderEx(QString zipPathName, qtuser_core::Progressor* progressor,QObject* parent)
	:QObject(parent)
{
    m_zip = new QuaZip();
    m_pModelFile = new QuaZipFile();
    m_pXmlReader = new  QXmlStreamReader();
    m_contentxml = new contentXML();
    m_zipPathName = zipPathName;
    m_progressor = progressor;
    isFDM = true;
}

Cx3dRenderEx::~Cx3dRenderEx()
{
    if (m_zip)
    {
        delete m_zip;
        m_zip = nullptr;
    }
    if (m_pModelFile)
    {
        delete m_pModelFile;
        m_pModelFile = nullptr;
    }
    if (m_pXmlReader)
    {
        delete m_pXmlReader;
        m_pXmlReader = nullptr;
    }
    if (m_contentxml)
    {
        delete m_contentxml;
        m_contentxml = nullptr;
    }
}


void Cx3dRenderEx::readContents()
{
    m_zip->setZipName(m_zipPathName);
    if (m_zip->open(QuaZip::mdUnzip))
    {
        m_zip->setCurrentFile("Content.xml");
        m_pModelFile->setZip(m_zip);
        if (m_pModelFile->open(QIODevice::ReadOnly))
        {
            m_pXmlReader->setDevice(m_pModelFile);//绑定输入输出设备
            QXmlStreamReader::TokenType type;
            modelMatrix  mMatrixTemp;
            while (!m_pXmlReader->atEnd())
            {
                type = m_pXmlReader->readNext();
                switch (type) {
                case QXmlStreamReader::NoToken://空行
                    break;
                case QXmlStreamReader::Invalid://无效的标记 出错了
                    qDebug() << "Invalid";
                    break;
                case QXmlStreamReader::StartDocument://xml文档开始
                {
                    QString str;
                    str += m_pXmlReader->documentVersion();
                    str += "    ";
                    str += m_pXmlReader->documentEncoding();
                    str += "    ";
                    str += m_pXmlReader->isStandaloneDocument() ? "yes" : "no";
                }
                break;
                case QXmlStreamReader::EndDocument://xml文档结束
                    break;
                case QXmlStreamReader::StartElement://元素(节点)开始
                {
                    if ("slice" == m_pXmlReader->name().toString())
                    {
                        //获取元素中的属性
                        QXmlStreamAttributes attributes = m_pXmlReader->attributes();
                        if (!attributes.isEmpty()) {
                            foreach(QXmlStreamAttribute attribute, attributes) {
                                m_contentxml->sliceType = attribute.value().toString();
                            }
                        }
                    }
                    else if ("machine" == m_pXmlReader->name().toString())
                    {
                        //获取元素中的属性
                        QXmlStreamAttributes attributes = m_pXmlReader->attributes();
                        if (!attributes.isEmpty()) {
                            foreach(QXmlStreamAttribute attribute, attributes) {
                                m_contentxml->machine = attribute.value().toString().replace(".default", "");
                            }
                        }
                    }
                    else if ("profile" == m_pXmlReader->name().toString())
                    {
                        //获取元素中的属性
                        QXmlStreamAttributes attributes = m_pXmlReader->attributes();
                        if (!attributes.isEmpty()) {
                            foreach(QXmlStreamAttribute attribute, attributes) {
                                m_contentxml->profile = attribute.value().toString();
                            }
                        }
                    }
                    else if ("materials" == m_pXmlReader->name().toString())
                    {
                        //获取元素中的属性
                        QXmlStreamAttributes attributes = m_pXmlReader->attributes();
                        if (!attributes.isEmpty()) {
                            foreach(QXmlStreamAttribute attribute, attributes) {
                                m_contentxml->materials = attribute.value().toString();
                            }
                        }
                    }
                    else if ("meshPathName" == m_pXmlReader->name().toString())
                    {
                        //获取元素中的属性
                        QXmlStreamAttributes attributes = m_pXmlReader->attributes();
                        if (!attributes.isEmpty()) {
                            foreach(QXmlStreamAttribute attribute, attributes) {
                                QString key = attribute.name().toString();
                                if (key == "url")
                                {
                                    m_contentxml->meshPathName.push_back(attribute.value().toString());
                                }
                                else {
                                    m_contentxml->meshName.push_back(attribute.value().toString());
                                }
                                
                            }
                        }
                    }
                    else if ("position" == m_pXmlReader->name().toString())
                    {
                        //获取元素中的属性
                        QXmlStreamAttributes attributes = m_pXmlReader->attributes();
                        if (attributes.size() == 3)
                        {
                            float X = attributes.at(0).value().toFloat();
                            mMatrixTemp.Position.setX(X);
                            float Y = attributes.at(1).value().toFloat();
                            mMatrixTemp.Position.setY(Y);
                            float Z = attributes.at(2).value().toFloat();
                            mMatrixTemp.Position.setZ(Z);
                        }
                    }
                    else if ("scale" == m_pXmlReader->name().toString())
                    {
                        //获取元素中的属性
                        QXmlStreamAttributes attributes = m_pXmlReader->attributes();
                        if (attributes.size() == 3)
                        {
                            float X = attributes.at(0).value().toFloat();
                            mMatrixTemp.Scale.setX(X);
                            float Y = attributes.at(1).value().toFloat();
                            mMatrixTemp.Scale.setY(Y);
                            float Z = attributes.at(2).value().toFloat();
                            mMatrixTemp.Scale.setZ(Z);
                        }
                    }
                    else if ("rotate" == m_pXmlReader->name().toString())
                    {
                        //获取元素中的属性
                        QXmlStreamAttributes attributes = m_pXmlReader->attributes();
                        if (attributes.size() == 4)
                        {
                            float X = attributes.at(0).value().toFloat();
                            mMatrixTemp.Rotate.setX(X);
                            float Y = attributes.at(1).value().toFloat();
                            mMatrixTemp.Rotate.setY(Y);
                            float Z = attributes.at(2).value().toFloat();
                            mMatrixTemp.Rotate.setZ(Z);
                            float setScalar = attributes.at(3).value().toFloat();
                            mMatrixTemp.Rotate.setScalar(setScalar);
                        }
                    }
                    else if ("support" == m_pXmlReader->name().toString())
                    {
                        //获取元素中的属性
                        QXmlStreamAttributes attributes = m_pXmlReader->attributes();
                        if (attributes.size())
                        {
                            mMatrixTemp.supName = attributes.at(0).value().toString();
                        }
                        m_contentxml->meshMatrix.push_back(mMatrixTemp);
                    }
                }
                break;
                case QXmlStreamReader::EndElement://元素(节点)结束
                {
                    QString str;
                    str += "</";
                    str += m_pXmlReader->name().toString();
                    str += ">";
                }
                break;
                case QXmlStreamReader::Comment://注释
                {
                    QString str;
                    str += "<!--";
                    str += m_pXmlReader->text().toString();//读取注释文本内容
                    str += "-->";
                }
                break;
                case QXmlStreamReader::EntityReference://表示无法解析的部分
                    break;
                case QXmlStreamReader::ProcessingInstruction://表示一个处理指令
                    break;
                }
            }
        }
        m_pModelFile->close();
    }
    m_zip->close();
}

void Cx3dRenderEx::readDefault()
{
	if (m_zip->open(QuaZip::mdUnzip))
	{
		m_zip->setCurrentFile(m_contentxml->machine + ".default");
		m_pModelFile->setZip(m_zip);
		if (m_pModelFile->open(QIODevice::ReadOnly))
		{
            m_machineSettings = m_pModelFile->readAll();
		}
		m_pModelFile->close();

		m_zip->setCurrentFile(m_contentxml->profile);
		if (m_pModelFile->open(QIODevice::ReadOnly))
		{
			m_profileSettings = m_pModelFile->readAll();
		}
		m_pModelFile->close();
	}
	m_zip->close();
}

//start lisugui 2020-11-5 解决_cx3d 问题，暂时不加机型
void Cx3dRenderEx::loadZip()
{
    QByteArray machineBytes;
    QByteArray profileBytes;
    if (m_zip->open(QuaZip::mdUnzip))
    {
        m_zip->setCurrentFile(m_contentxml->machine + ".default");
        m_pModelFile->setZip(m_zip);
        if (m_pModelFile->open(QIODevice::ReadOnly))
        {
            machineBytes = m_pModelFile->readAll();
        }
        m_pModelFile->close();

        m_zip->setCurrentFile(m_contentxml->profile);
        if (m_pModelFile->open(QIODevice::ReadOnly))
        {
            profileBytes = m_pModelFile->readAll();
        }
        m_pModelFile->close();
    }
    m_zip->close();
}
//end

//void Cx3dRenderEx::loadZip()
//{
//    QByteArray machineBytes;
//    QByteArray profileBytes;
//    if (m_zip->open(QuaZip::mdUnzip))
//    {
//        m_zip->setCurrentFile(m_contentxml->machine);
//        m_pModelFile->setZip(m_zip);
//        if (m_pModelFile->open(QIODevice::ReadOnly))
//        {
//            machineBytes = m_pModelFile->readAll();
//        }
//        m_pModelFile->close();
//     
//        m_zip->setCurrentFile(m_contentxml->profile);
//        if (m_pModelFile->open(QIODevice::ReadOnly))
//        {
//            profileBytes = m_pModelFile->readAll();
//        }
//        m_pModelFile->close();
//    }
//    m_zip->close();
//
//
//    QString strPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
//    QString strMachinePathName = strPath + "/Machines/" + m_contentxml->machine;
//
//    QString strProfilePath = strPath + "/Profiles/" + m_contentxml->machine.replace(".default","");
//    QDir dir(strProfilePath);
//    if (!dir.exists(strProfilePath)) 
//    {
//        dir.mkpath(strProfilePath);
//    }
//    QString strProfilePathName = strProfilePath + "/" + m_contentxml->profile;
//      
//    QFile qfile;
//    qfile.setFileName(strMachinePathName);
//    if (qfile.open(QIODevice::WriteOnly))
//    {
//        qfile.write(machineBytes);
//    }
//    qfile.close();
//
//    qfile.setFileName(strProfilePathName);
//    if (qfile.open(QIODevice::WriteOnly))
//    {
//        qfile.write(profileBytes);
//    }
//    qfile.close();
//}


void Cx3dRenderEx::meshs()
{
    if (m_zip->open(QuaZip::mdUnzip))
    {
        int i = 0;
        Q_FOREACH (QString meshPath, m_contentxml->meshPathName)
        {
            QString str = meshPath;
            trimesh::TriMesh* mesh = new trimesh::TriMesh();

            if (!m_zip->setCurrentFile(str))
            {
                QString fname = QTextCodec::codecForLocale()->toUnicode(QByteArray::fromStdString(str.toStdString()));
                m_zip->setCurrentFile(fname);
            }
            m_pModelFile->setZip(m_zip);
            if (m_pModelFile->open(QIODevice::ReadOnly))
            {
                char head[80];
                m_pModelFile->read(head, 80);
                int ifacets = 0;
                m_pModelFile->read((char*)&ifacets, sizeof(int));
                mesh->faces.reserve(ifacets);
                mesh->vertices.reserve(3 * ifacets);
                for (int i = 0; i < ifacets; i++)
                {
                    float fbuf[48];
                    m_pModelFile->read((char*)fbuf, 48);
                    int v = mesh->vertices.size();
                    mesh->vertices.push_back(trimesh::point(fbuf[3], fbuf[4], fbuf[5]));
                    mesh->vertices.push_back(trimesh::point(fbuf[6], fbuf[7], fbuf[8]));
                    mesh->vertices.push_back(trimesh::point(fbuf[9], fbuf[10], fbuf[11]));
                    mesh->faces.push_back(trimesh::TriMesh::Face(v, v + 1, v + 2));
                    char end[2];
                    m_pModelFile->read(end, 2);//00结束符
                }
                m_meshs.push_back(mesh);
            }
            else
            {
                auto fileName = m_zipPathName.left(m_zipPathName.lastIndexOf("/")) + "/models/" + str;
                if (!QFile(fileName).exists())
                {
                    continue;
                }
                cxkernel::ModelNDataSerial serial;
                serial.load(fileName, nullptr);
                trimesh::TriMesh* mesh = new trimesh::TriMesh(*serial.getData()->mesh);
                m_meshs.push_back(mesh);
            }
            m_pModelFile->close();
            i++;
        }
        m_zip->close();
    }
}

void Cx3dRenderEx::build()
{
    readContents();
    if (!creative_kernel::checkMachineExist(m_contentxml->machine.replace(".default", "")))
    {
        if (m_progressor)
        {
            m_progressor->failed("No matching printer found, cannot import project file");
        }
    }
    loadZip();
    meshs();
    readDefault();
}

contentXML* Cx3dRenderEx::getContentXML()
{
    contentXML* t = m_contentxml;
    m_contentxml = nullptr;
    return t;
}

std::vector<trimesh::TriMesh*> Cx3dRenderEx::getMeshs()
{
    return m_meshs;
}

QByteArray Cx3dRenderEx::getMachineDefault()
{
    return m_machineSettings;
}

QByteArray Cx3dRenderEx::getProfileDefault()
{
    return m_profileSettings;
}

