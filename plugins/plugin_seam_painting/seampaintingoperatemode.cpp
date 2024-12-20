#include "seampaintingoperatemode.h"
#include "interface/selectorinterface.h"
#include "data/modeln.h"
#include "data/modelgroup.h"
#include "interface/eventinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/spaceinterface.h"
#include "interface/selectorinterface.h"
#include "kernel/kernel.h"
#include "kernel/globalconst.h"
#include "kernel/kernelui.h"

using namespace creative_kernel;

SeamPaintingOperateMode::SeamPaintingOperateMode(QObject* parent)
    : PaintOperateMode(parent)
{
	m_colorMethod = 1;

    connect(this, &PaintOperateMode::enter, this, [=]()
    {
        initColors();
    });

}

SeamPaintingOperateMode::~SeamPaintingOperateMode()
{

}

void SeamPaintingOperateMode::initColors()
{
    creative_kernel::EngineType type = creative_kernel::getKernel()->globalConst()->getEngineType();
    if (type == creative_kernel::EngineType::ET_CURA) 
    {   /* ET_CURA */
        std::vector<trimesh::vec> colors;
        colors.push_back(trimesh::vec(0.87, 0.87, 0.87));
        colors.push_back(trimesh::vec(0.32, 0.76, 0.32));
        colors.push_back(trimesh::vec(0.76, 0.32, 0.32));
        setColorsList(colors);
        
        m_paintIndex = 1;
        m_blockIndex = 2;
        
         QList<int> flags;
         for (int i = 0, count = groupSize(); i < count; ++i)
         {
            flags << 1;
         }
         setDefaultFlag(flags);
    }
    else if (type == creative_kernel::EngineType::ET_ORCA)
    {   /* ET_ORCA */
         std::vector<trimesh::vec> colors;
         colors.push_back(trimesh::vec(0.32, 0.76, 0.32));
         colors.push_back(trimesh::vec(0.76, 0.32, 0.32));
         colors.push_back(trimesh::vec(0.87, 0.87, 0.87));
         setColorsList(colors);

         m_paintIndex = 0;
         m_blockIndex = 1;

         QList<int> flags;
         for (int i = 0, count = groupSize(); i < count; ++i)
         {
            flags << 3;
         }
         setDefaultFlag(flags);
    }
}

void SeamPaintingOperateMode::restore(int objectID, const QList<std::vector<std::string>>& data)
{
    if (!isRunning())
	{	// 切换到涂抹场景
        ModelGroup* group = getModelGroupByObjectId(objectID);
        if (group)
            selectGroup(group);
        else 
        {
            ModelN* model = getModelNByObjectId(objectID);
            selectModelN(model);
        }
    
        setWaitForLoadModel(true);
        getKernelUI()->switchMode(31);
        creative_kernel::setVisOperationMode(this);
        setWaitForLoadModel(false);
	}

    PaintOperateMode::restore(objectID, data);
}

void SeamPaintingOperateMode::onAttach() 
{	
    m_visibleTypes.clear();
	m_visibleTypes << (int)creative_kernel::ModelNType::negative_part <<
                        (int)creative_kernel::ModelNType::modifier <<
                        (int)creative_kernel::ModelNType::support_defender <<
                        (int)creative_kernel::ModelNType::support_generator;
	initValidModels();

    m_lastSpreadDatas.clear();
    for (auto model : m_originModels)
        m_lastSpreadDatas << model->getSeam2Facets();

    QList<int> flags;
    for (auto m : m_originModels)
    {
        m_lastSpreadDatas << m->getSeam2Facets();
    }

    m_highlightOverhangsDeg = 0;    // init
    PaintOperateMode::onAttach();
}

void SeamPaintingOperateMode::onDettach() 
{
    if (isRunning())
    {
        QList<std::vector<std::string>> oldDatas;
        QList<std::vector<std::string>> newDatas;
        for (int i = 0, count = m_originModels.size(); i < count; ++i)
        {
            auto model = m_originModels[i];
            oldDatas << model->getSeam2Facets();
            newDatas << getPaintData(i);
        }

        for (int i = 0, count = m_originModels.size(); i < count; ++i)
        {
            if (!paintDataEqual(oldDatas[i], newDatas[i]))
            {
                m_originModels[i]->setSeam2Facets(newDatas[i]);
                m_originModels[i]->dirty();
            }
        }

        PaintOperateMode::onDettach();

        resetValidModels();
    }
}

void SeamPaintingOperateMode::onLeftMouseButtonPress(QMouseEvent* event)
{
    setColorIndex(m_paintIndex);
    PaintOperateMode::onLeftMouseButtonPress(event);
}

void SeamPaintingOperateMode::onRightMouseButtonPress(QMouseEvent* event)
{
    if (isHoverModel(event->pos()))
    {
        m_pressed = true;
        setColorIndex(m_blockIndex);
        PaintOperateMode::onLeftMouseButtonPress(event);
        m_rightPressStatus = false;
    }
    else 
    {
        PaintOperateMode::onRightMouseButtonPress(event);
    }
}

void SeamPaintingOperateMode::onRightMouseButtonMove(QMouseEvent* event)
{
    if (m_pressed)
    {
        PaintOperateMode::onLeftMouseButtonMove(event);
        m_rightMoveStatus = false;
    }
    else 
    {
        PaintOperateMode::onRightMouseButtonMove(event);
    }
}

void SeamPaintingOperateMode::onRightMouseButtonRelease(QMouseEvent* event)
{
    m_pressed = false;
    PaintOperateMode::onLeftMouseButtonRelease(event);
}
