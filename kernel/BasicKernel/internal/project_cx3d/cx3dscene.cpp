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
	if (m_models.size())
	{
        m_models.clear();
	}
    if (m_contentXML != nullptr)
    {
        delete m_contentXML;
        m_contentXML = nullptr;
    }
}


void Cx3dScene::build(cx3d::Cx3dFileProject* reader, QThread* mainThread)
{
    m_contentXML = reader->m_contentXml;
    m_models.reserve(reader->m_models.size());
	for (const auto& model : reader->m_models) {
        m_models.emplace_back(model);
    }
	m_paramSettings = reader->m_datas[cx3d::FieldName::PARAM];

    for (creative_kernel::ModelN* modeln : m_models)
    {
        modeln->mesh()->need_bbox();
    }

    m_modelnDatas = reader->m_modelnDatas;
    m_layerDatas = reader->m_layerDatas;
    m_objectSettings = reader->m_objectSettings;
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
}

void Cx3dScene::setMeshs()
{
	const auto& modelInfos = m_contentXML->modelInfos;

    if (m_layerDatas.size() != m_contentXML->modelInfos.size())
    {
        qWarning() << Q_FUNC_INFO << "======== m_layerDatas.size() != m_contentXML->modelInfos.size() ========";
    }

    if (m_objectSettings.size() != m_contentXML->modelInfos.size())
    {
        qWarning() << Q_FUNC_INFO << "======== m_objectSettings.size() != m_contentXML->modelInfos.size() ========";
    }

    if (m_modelnDatas.size() == m_contentXML->modelInfos.size())
    {
        QList<ModelN*> adds;
        QList<ModelN*> removes;
        for (size_t i = 0; i < m_modelnDatas.size(); i++)
        {
            creative_kernel::ModelN* newModel = new creative_kernel::ModelN();
			newModel->setData(m_modelnDatas.at(i));

            if(i >= 0 && i < m_layerDatas.size())
                newModel->setLayerHeightProfile(m_layerDatas.at(i));

            if (i >= 0 && i < m_objectSettings.size())
                newModel->setsetting(m_objectSettings.at(i));

            const auto& tmpData = modelInfos.at(i);
			newModel->SetInitPosition(tmpData.matrix.Position);//for reset to zero
			newModel->setLocalPosition(tmpData.matrix.Position, true);
			newModel->setLocalScale(tmpData.matrix.Scale, true);
			newModel->setLocalQuaternion(tmpData.matrix.Rotate, true);

            newModel->setDefaultColorIndex(tmpData.colorIndex);
            newModel->updateMatrix();

            adds.push_back(newModel);
        }

        modifySpace(removes, adds, true);
    }
}

void Cx3dScene::setMachineSettings(QString& machineName)
{
}





