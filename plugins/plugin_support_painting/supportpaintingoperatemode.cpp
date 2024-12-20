#include "supportpaintingoperatemode.h"
#include "interface/selectorinterface.h"
#include "data/modeln.h"
#include "interface/eventinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/spaceinterface.h"
#include "kernel/kernel.h"
#include "kernel/globalconst.h"
#include "kernel/kernelui.h"

using namespace creative_kernel;

SupportPaintingOperateMode::SupportPaintingOperateMode(QObject* parent)
    : PaintOperateMode(parent)
{
	m_colorMethod = 1;

    m_visibleTypes.clear();
	m_visibleTypes << (int)creative_kernel::ModelNType::negative_part <<
                        (int)creative_kernel::ModelNType::modifier <<
                        (int)creative_kernel::ModelNType::support_defender <<
                        (int)creative_kernel::ModelNType::support_generator;


    
    connect(this, &PaintOperateMode::enter, this, [=]()
    {
        initColors();
    });
}

SupportPaintingOperateMode::~SupportPaintingOperateMode()
{

}

void SupportPaintingOperateMode::initColors()
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

void SupportPaintingOperateMode::restore(int objectID, const QList<std::vector<std::string>>& data)
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
        getKernelUI()->switchMode(30);
        setWaitForLoadModel(false);
	}

    PaintOperateMode::restore(objectID, data);
}

void SupportPaintingOperateMode::onAttach() 
{	
	initValidModels();

    m_lastSpreadDatas.clear();
    for (auto m : m_originModels)
        m_lastSpreadDatas << m->getSupport2Facets();
    
    PaintOperateMode::onAttach();
}

void SupportPaintingOperateMode::onDettach() 
{
    if (isRunning())
    {
        QList<std::vector<std::string>> oldDatas;
        QList<std::vector<std::string>> newDatas;
        for (int i = 0, count = m_originModels.size(); i < count; ++i)
        {
            auto model = m_originModels[i];
            oldDatas << model->getSupport2Facets();
            newDatas << getPaintData(i);
        }

        for (int i = 0, count = m_originModels.size(); i < count; ++i)
        {
            if (!paintDataEqual(oldDatas[i], newDatas[i]))
            {
                m_originModels[i]->setSupport2Facets(newDatas[i]);
                m_originModels[i]->dirty();
            }
        }

        PaintOperateMode::onDettach();

        resetValidModels();
    }
}

void SupportPaintingOperateMode::onLeftMouseButtonPress(QMouseEvent* event)
{
    setColorIndex(m_paintIndex);
    PaintOperateMode::onLeftMouseButtonPress(event);
}

void SupportPaintingOperateMode::onRightMouseButtonPress(QMouseEvent* event)
{
    if (isHoverModel(event->pos()))
    {
        m_pressed = true;
        setColorIndex(m_blockIndex);
        PaintOperateMode::onLeftMouseButtonPress(event);
        m_rightPressStatus = false;
        m_rightClickStatus = false;
    }
    else 
    {
        PaintOperateMode::onRightMouseButtonPress(event);
    }
}

void SupportPaintingOperateMode::onRightMouseButtonMove(QMouseEvent* event)
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

void SupportPaintingOperateMode::onRightMouseButtonRelease(QMouseEvent* event)
{
    m_pressed = false;
    PaintOperateMode::onLeftMouseButtonRelease(event);
}

void SupportPaintingOperateMode::onSelectionsChanged()
{
	if (!isRunning())
		return;

	initValidModels();

    if (m_originModels.isEmpty()) 
    {
        creative_kernel::setVisOperationMode(NULL);
        return;
    }

    onDettach();

    setWaitForLoadModel(true);
    onAttach();
    setWaitForLoadModel(false);

    //m_lastSpreadDatas.clear();
    //for (auto m : m_originModels)
    //    m_lastSpreadDatas << m->getSupport2Facets();
    //
    //PaintOperateMode::onAttach();

    //initColors();
}