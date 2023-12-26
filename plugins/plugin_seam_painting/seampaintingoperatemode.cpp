#include "seampaintingoperatemode.h"
#include "interface/selectorinterface.h"
#include "data/modeln.h"
#include "interface/eventinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/spaceinterface.h"
	
SeamPaintingOperateMode::SeamPaintingOperateMode(QObject* parent)
    : PaintOperateMode(parent)
{
	m_colorMethod = 1;

    std::vector<trimesh::vec> colors;
    colors.push_back(trimesh::vec(0.87, 0.87, 0.87));
    colors.push_back(trimesh::vec(0.32, 0.76, 0.32));
    colors.push_back(trimesh::vec(0.76, 0.32, 0.32));

    setColorsList(colors);
}

SeamPaintingOperateMode::~SeamPaintingOperateMode()
{

}

void SeamPaintingOperateMode::restore(creative_kernel::ModelN* model, const std::vector<std::string>& data)
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

void SeamPaintingOperateMode::onAttach() 
{	
    auto selections = creative_kernel::selectionms();
	if (selections.isEmpty())
		return;
    m_lastSpreadData = selections.at(0)->getSeam2Facets();
    
    PaintOperateMode::onAttach();
}

void SeamPaintingOperateMode::onDettach() 
{
    auto oldData = m_originModel->getSeam2Facets();
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
        m_originModel->setSeam2Facets(newData);
        creative_kernel::dirtyModelSpace();
    }


    PaintOperateMode::onDettach();
}

void SeamPaintingOperateMode::onLeftMouseButtonPress(QMouseEvent* event)
{
    setColorIndex(1);
    PaintOperateMode::onLeftMouseButtonPress(event);
}

void SeamPaintingOperateMode::onRightMouseButtonPress(QMouseEvent* event)
{
    if (isHoverModel(event->pos()))
    {
        m_pressed = true;
        setColorIndex(2);
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
