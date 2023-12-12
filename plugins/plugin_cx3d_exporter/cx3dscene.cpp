#include "cx3dscene.h"
#include <Qt3DCore/QTransform>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlProperty>
#include <QtCore/QFile>
#include <QtCore/QDir>

#include "interface/spaceinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/reuseableinterface.h"
#include "interface/machineinterface.h"

#include <QtCore/QFileInfo>
#include "data/fdmsupportgroup.h"
#include <QStandardPaths>

#include "kernel/kernelui.h"
#include "cxkernel/wrapper/cryptfiledevice.h"
#include "qtusercore/string/resourcesfinder.h"

using namespace creative_kernel;
Cx3dScene::Cx3dScene(QObject* parent)
	: QObject(parent)
	, m_contentXML(nullptr)
{
}

Cx3dScene::~Cx3dScene()
{
	if (m_meshes.size())
	{
		m_meshes.clear();
	}
    if (m_contentXML)
    {
        delete m_contentXML;
        m_contentXML = nullptr;
    }
}


void Cx3dScene::build(Cx3dRenderEx* reader, QThread* mainThread)
{
    m_contentXML = reader->getContentXML();
    m_meshes = reader->getMeshs();
    m_machineSettings = reader->getMachineDefault();
    m_profileSettings = reader->getProfileDefault();

    for (trimesh::TriMesh* mesh : m_meshes)
    {
        mesh->need_bbox();
    }
    if (m_contentXML->sliceType == "FDM")
    {
        m_fdmSup = reader->getFDMSup();
        for (creative_kernel::FDMSupportGroup* fdmSup : m_fdmSup)
        {
            fdmSup->move2MainThread(mainThread);
        }
    }
}

void Cx3dScene::setScene()
{
    setMeshs();
    setMachineProfile();
    creative_kernel::checkModelRange();
    creative_kernel::requestVisUpdate(true);
}

void Cx3dScene::setMachineProfile()
{
	addMachine(m_contentXML->machine);
	if (!creative_kernel::checkMachineExist(m_contentXML->machine))
	{
		//�������ò�����
		creative_kernel::setCurrentMachine(creative_kernel::currentMachineName());
	}
	else {
		//���õ�ǰ����
		creative_kernel::setCurrentMachine(m_contentXML->machine);
		//���õ�ǰ�Ĳ�
		if (m_contentXML->materials != "")
		{
			QStringList materials = m_contentXML->materials.split(";");
			for (int index = 0; index < materials.size(); index++)
			{
				QString material = materials[index];
				if (material != "")
				{
					creative_kernel::setExtrudersMaterial(index, material);
				}			
			}
		}
		//���õ�ǰprofile
		creative_kernel::setCurrentProfile(m_contentXML->profile);
	}
   /* QString strPath = qtuser_core::getOrCreateAppDataLocation("") + "/Machines/";

	QStringList machines = machinesList();

	QObject* pTopBar = getKernelUI()->getUI("topbar");
    if (pTopBar)
    {
        QObject* pSelectMachine = pTopBar->findChild<QObject*>("selectMachine");
        if (pSelectMachine)
        {
            QObject* pCombox = pSelectMachine->findChild<QObject*>("machineCommbox");
			if (pCombox)
			{
				QQmlProperty::write(pCombox, "model", QVariant::fromValue(machines));
				int currentIndex = machines.indexOf(m_contentXML->machine);
				if (currentIndex < 0) currentIndex = 0;

				QQmlProperty::write(pCombox, "currentIndex", QVariant::fromValue(currentIndex));
				setMachineSettings(m_contentXML->machine);
			}

        }
    }*/
}

void Cx3dScene::OldsetMachineProfile()
{
	addMachine(m_contentXML->machine);
	QStringList machines = machinesList();
	QObject* rightobj = getKernelUI()->getUI("rightPanel");   //  getKernelUI()->rightPanel();
	if (rightobj)
	{
		QObject* sliceobj = rightobj->findChild<QObject*>("sliceUiobj");
		QObject* pRightPanel = sliceobj->findChild<QObject*>("RightInfoPanel");
		if (pRightPanel)
		{
			QObject* pSelectMachine = pRightPanel->findChild<QObject*>("selectMachine");
			if (pSelectMachine)
			{
				QObject* pCombox = pSelectMachine->findChild<QObject*>("machineCommbox");
				if (pCombox)
				{
					QQmlProperty::write(pCombox, "model", QVariant::fromValue(machines));
					int currentIndex = machines.indexOf(m_contentXML->machine);
					if (currentIndex < 0)
					{
						currentIndex = 0;
					}
					else
					{
						setCurrentMachine(m_contentXML->machine);
					}
					QQmlProperty::write(pCombox, "currentIndex", QVariant::fromValue(currentIndex));
					setMachineSettings(m_contentXML->machine.replace("_CX3D", ""));
				}
			}
		}
	}
}

void Cx3dScene::setMeshs()
{
    if (m_meshes.size() == m_contentXML->meshPathName.size()
        && m_meshes.size() == m_contentXML->meshMatrix.size())
    {
        for (size_t i = 0; i < m_meshes.size(); i++)
        {
            creative_kernel::ModelN* newModel = new creative_kernel::ModelN();
            TriMeshPtr ptr(m_meshes.at(i));

			cxkernel::ModelCreateInput input;
			input.mesh = ptr;
			input.name = i< m_contentXML->meshName.size()?m_contentXML->meshName[i]:m_contentXML->meshPathName.at(i);
			cxkernel::ModelNDataPtr data(cxkernel::createModelNData(input));

			newModel->setData(data);

			newModel->SetInitPosition(m_contentXML->meshMatrix.at(i).Position);//for reset to zero
			newModel->setLocalPosition(m_contentXML->meshMatrix.at(i).Position, true);
			newModel->setLocalScale(m_contentXML->meshMatrix.at(i).Scale, true);
			newModel->setLocalQuaternion(m_contentXML->meshMatrix.at(i).Rotate, true);

            addModel(newModel, true);

            if (m_fdmSup.size())
            {
                newModel->setFDMSup(m_fdmSup.at(i));
            }

            newModel->updateMatrix();
        }
    }
}

void Cx3dScene::setSupport(QString filePathName, creative_kernel::ModelN* model)
{
    QFile file(filePathName);
    if (file.open(QIODevice::ReadOnly))
    {
        QDataStream stream(&file);
    }
    file.close();
}


void Cx3dScene::setMachineSettings(QString& machineName)
{
	QString machinePath = qtuser_core::getOrCreateAppDataLocation("") + "/Machines";
	QString strMachinePath = machinePath + "/" + m_contentXML->machine + ".default";
	QDir createfile;
	bool exist = createfile.exists(strMachinePath);
	if (exist)
	{
		createfile.remove(strMachinePath);
	}
	cxkernel::CryptFileDevice pFile;
	pFile.setFileName(strMachinePath);
	if (pFile.open(QIODevice::WriteOnly))
	{
		pFile.write(m_machineSettings);
		pFile.close();
	}

	QString profilePath = qtuser_core::getOrCreateAppDataLocation("") + "/Profiles";
	QString strProfilePath = profilePath + "/" + m_contentXML->machine + "/" + m_contentXML->profile;
	exist = createfile.exists(strProfilePath);
	if (exist)
	{
		createfile.remove(strProfilePath);
	}
	pFile.setFileName(strProfilePath);
	if (pFile.open(QIODevice::WriteOnly))
	{
		pFile.write(m_profileSettings);
		pFile.close();
	}

	QString strPath = qtuser_core::getOrCreateAppDataLocation("") + "/Machines/";
    setCurrentMachine(machineName);

	int index = 1;
	if (m_contentXML->profile.contains("high"))
	{
		index = 0;
	}
	else if (m_contentXML->profile.contains("low"))
	{
		index = 2;
	}
}





