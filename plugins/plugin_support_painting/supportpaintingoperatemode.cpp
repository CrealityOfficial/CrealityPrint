#include "supportpaintingoperatemode.h"
#include "interface/selectorinterface.h"
#include "data/modeln.h"
#include "interface/eventinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/spaceinterface.h"

SupportPaintingOperateMode::SupportPaintingOperateMode(QObject* parent)
    : PaintOperateMode(parent)
{
	m_colorMethod = 1;
    
    std::vector<trimesh::vec> colors;
    colors.push_back(trimesh::vec(0.87, 0.87, 0.87));
    colors.push_back(trimesh::vec(0.32, 0.76, 0.32));
    colors.push_back(trimesh::vec(0.76, 0.32, 0.32));

    setColorsList(colors);
}

SupportPaintingOperateMode::~SupportPaintingOperateMode()
{

}

void SupportPaintingOperateMode::restore(creative_kernel::ModelN* model, const std::vector<std::string>& data)
{
    if (!m_originModel)
	{	// 切换到涂抹场景
        creative_kernel::selectOne(model);
        setWaitForLoadModel(true);
        creative_kernel::setVisOperationMode(this);
        setWaitForLoadModel(false);
	}

    PaintOperateMode::restore(model, data);
}

void SupportPaintingOperateMode::onAttach() 
{	
    auto selections = creative_kernel::selectionms();
	if (selections.isEmpty())
		return;
    m_lastSpreadData = selections.at(0)->getSupport2Facets();
    
    PaintOperateMode::onAttach();
}

void SupportPaintingOperateMode::onDettach() 
{
    auto oldData = m_originModel->getSupport2Facets();
    auto newData = getPaintData();

    bool isChanged = false;
    if (oldData.size() != newData.size())
    {
        isChanged = true;
    }
    else 
    {
        for (int i = 0, count = oldData.size(); i < count; ++i)
        {
            if (oldData[i] != newData[i])
            {
                isChanged = true;
                break;
            }
        }
    }

    if (isChanged)
    {
        m_originModel->setSupport2Facets(newData);
        creative_kernel::dirtyModelSpace();
    }

    PaintOperateMode::onDettach();
}

void SupportPaintingOperateMode::onLeftMouseButtonPress(QMouseEvent* event)
{
    setColorIndex(1);
    PaintOperateMode::onLeftMouseButtonPress(event);
}

void SupportPaintingOperateMode::onRightMouseButtonPress(QMouseEvent* event)
{
    if (isHoverModel(event->pos()))
    {
        m_pressed = true;
        setColorIndex(2);
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