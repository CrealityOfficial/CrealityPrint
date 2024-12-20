#include "coloroperatemode.h"
#include "interface/selectorinterface.h"
#include "interface/spaceinterface.h"
#include "data/modeln.h"
#include "data/modelgroup.h"
#include "interface/eventinterface.h"
#include "interface/visualsceneinterface.h"
#include "kernel/kernel.h"
#include "interface/machineinterface.h"
#include "interface/printerinterface.h"
#include "kernel/kernelui.h"
	
ColorOperateMode::ColorOperateMode(QObject* parent)
    : PaintOperateMode(parent)
{
    connect(this, &PaintOperateMode::enter, this, [=]()
    {
		std::vector<trimesh::vec3> colors;
		handleColorsListChanged(colors);
		onColorsListChanged();
    });

    m_type = qtuser_3d::SceneOperateMode::OperateModeType::FixedMode;

}

ColorOperateMode::~ColorOperateMode()
{

}

void ColorOperateMode::handleColorsListChanged(const std::vector<trimesh::vec>& newColors)
{
    auto colors = creative_kernel::currentColors();
    PaintOperateMode::handleColorsListChanged(colors);

    if (colors.size() <= 1)
        getKernelUI()->setCommandIndex(-1);
        // getKernelUI()->switchMode(-1);
        //creative_kernel::setVisOperationMode(NULL);
}

void ColorOperateMode::onApplyColor()
{
	QList<std::vector<std::string>> oldDatas;
    QList<std::vector<std::string>> newDatas;
    for (int i = 0, count = m_originModels.size(); i < count; ++i)
    {
        auto model = m_originModels[i];
        oldDatas << model->getColors2Facets();
        newDatas << getPaintData(i);
    }

    for (int i = 0, count = m_originModels.size(); i < count; ++i)
    {
        if (!paintDataEqual(oldDatas[i], newDatas[i]))
        {
            m_originModels[i]->updateRenderByMeshSpreadData(newDatas[i]);
            m_originModels[i]->dirty();
        }
    }
}

void ColorOperateMode::onDefaultColorIndexChanged()
{
    QList<int> flags;
    for (auto m : m_originModels)
        flags << (m->defaultColorIndex() + 1);
    setDefaultFlag(flags);
    refreshRender();
}

void ColorOperateMode::restore(int objectID, const QList<std::vector<std::string>>& data)
{
    if (isRunning())
	{	
        PaintOperateMode::restore(objectID, data);
	}
    else 
    {
        creative_kernel::ModelGroup* group = creative_kernel::getModelGroupByObjectId(objectID);
        if (group)
        {
            selectGroup(group);
            QList<creative_kernel::ModelN*> models = group->models();
            for (int i = 0, count = models.count(); i < count; ++i)
            {
                if (models[i]->modelNType() == creative_kernel::ModelNType::normal_part)
                    models[i]->setColors2Facets(data[i]);
            }
        }
        else
        {
            creative_kernel::ModelN* model = creative_kernel::getModelNByObjectId(objectID);
            model->setColors2Facets(data.first());
        }
        creative_kernel::updateWipeTowers();
    }
}

void ColorOperateMode::onAttach() 
{	
    m_lastSpreadDatas.clear();
    m_originModels.clear();
    m_visibleTypes.clear();
	m_visibleTypes << (int)creative_kernel::ModelNType::modifier;
	initValidModels();
    QList<int> flags;
    for (auto m : m_originModels)
    {
        m_lastSpreadDatas << m->getColors2Facets();
        flags << (m->defaultColorIndex() + 1);
    }

    m_highlightOverhangsDeg = 0;    // init
    PaintOperateMode::onAttach();
    setDefaultFlag(flags);

    for (auto m : m_originModels)
    {
        connect(m, &creative_kernel::ModelN::defaultColorIndexChanged, this, &ColorOperateMode::onDefaultColorIndexChanged);
        connect(m, &creative_kernel::ModelN::prepareCheckMaterial, this, &ColorOperateMode::onApplyColor);
    }
}

void ColorOperateMode::onDettach() 
{
    if (isRunning())
    {
        for (auto m : m_originModels)
        {
            disconnect(m, &creative_kernel::ModelN::defaultColorIndexChanged, this, &ColorOperateMode::onDefaultColorIndexChanged);
            disconnect(m, &creative_kernel::ModelN::prepareCheckMaterial, this, &ColorOperateMode::onApplyColor);
        }

        onApplyColor();
        PaintOperateMode::onDettach();
    }
    resetValidModels();   
	creative_kernel::updateWipeTowers();
}

void ColorOperateMode::onKeyPress(QKeyEvent* e)
{
    // if (e->modifiers() & Qt::ControlModifier)
    // {
        int value = e->key() - Qt::Key_1;
        if (value >= 0 && value <= 8) 
        {
	        setColorIndex(value);
            m_keyPressStatus = false;
        }
    // }
}

