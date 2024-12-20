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
#include "interface/shareddatainterface.h"
#include "qtuser3d/trimesh2/conv.h"
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
    creative_kernel::requestVisUpdate(true);
}

void Cx3dScene::setMachineProfile()
{
}
ModelDataPtr convertModelNData2ModelData(cxkernel::ModelNDataPtr data)
{
    if (!data)
        return nullptr;

    ModelDataPtr model_data(new ModelData());
    model_data->defaultColor = data->defaultColor;
    model_data->colors = data->colors;
    model_data->supports = data->supports;
    model_data->seams = data->seams;
    cxkernel::MeshDataPtr mesh_data(new cxkernel::MeshData());
    mesh_data->mesh = data->mesh;
    mesh_data->hull = data->hull;
    mesh_data->offset = data->offset;
    mesh_data->faces = data->faces;
    model_data->mesh = mesh_data;
    model_data->name = data->input.name;

    return model_data;
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
        //QList<ModelN*> adds;
        //QList<ModelN*> removes;
        SceneCreateData m_scene_create_data;
        for (size_t i = 0; i < m_modelnDatas.size(); i++)
        {
            creative_kernel::ModelDataPtr data = convertModelNData2ModelData(m_modelnDatas.at(i));
            GroupCreateData group_data;
            ModelCreateData model_create_data;

            SharedDataID id = registerModelData(data);
            id(ModelType) = (int)ModelNType::normal_part;
            model_create_data.dataID = id;
            model_create_data.name = data->name;

            trimesh::dbox3 b = data->mesh->calculateBox();
            model_create_data.xf = trimesh::xform::trans(0.0, 0.0, 0.0);

            group_data.models.append(model_create_data);
            const auto& tmpData = modelInfos.at(i);
            QMatrix4x4 tmpMatrix;
            tmpMatrix.setToIdentity();
            tmpMatrix.translate(tmpData.matrix.Position);
            tmpMatrix.rotate(tmpData.matrix.Rotate);
            tmpMatrix.scale(tmpData.matrix.Scale);
            group_data.xf = qtuser_3d::qMatrix2Xform(tmpMatrix);
           
            m_scene_create_data.add_groups.append(group_data);
            
        }
        modifyScene(m_scene_create_data);
    }
}

void Cx3dScene::setMachineSettings(QString& machineName)
{
}





