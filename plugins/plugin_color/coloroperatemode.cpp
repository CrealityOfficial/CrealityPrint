#include "coloroperatemode.h"
#include "interface/selectorinterface.h"
#include "data/modeln.h"
#include "interface/eventinterface.h"
#include "interface/visualsceneinterface.h"
#include "kernel/kernel.h"
#include "interface/machineinterface.h"
#include "kernel/kernelui.h"
	
ColorOperateMode::ColorOperateMode(QObject* parent)
    : PaintOperateMode(parent)
{

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
    auto oldData = m_originModel->getColors2Facets();
    auto newData = getPaintData();

    if (!paintDataEqual(oldData, newData))
    {
        m_originModel->updateRenderByMeshSpreadData(newData);
        m_originModel->dirty();
    }
}

void ColorOperateMode::onDefaultColorIndexChanged()
{
    setDefaultFlag(m_originModel->defaultColorIndex() + 1);
    refreshRender();
}

void ColorOperateMode::restore(creative_kernel::ModelN* model, const std::vector<std::string>& data)
{
    if (m_originModel)
	{	
        PaintOperateMode::restore(model, data);
	}
    else 
    {
        model->setColors2Facets(data);
    }
}

void ColorOperateMode::onAttach() 
{	
    auto selections = creative_kernel::selectionms();
	if (selections.isEmpty())
		return;
    m_lastSpreadData = selections.at(0)->getColors2Facets();
    setDefaultFlag(selections.at(0)->defaultColorIndex() + 1);
    
    PaintOperateMode::onAttach();

    connect(m_originModel, &creative_kernel::ModelN::defaultColorIndexChanged, this, &ColorOperateMode::onDefaultColorIndexChanged);
    connect(m_originModel, &creative_kernel::ModelN::prepareCheckMaterial, this, &ColorOperateMode::onApplyColor);

}

void ColorOperateMode::onDettach() 
{
    if (m_originModel)
    {
        disconnect(m_originModel, &creative_kernel::ModelN::defaultColorIndexChanged, this, &ColorOperateMode::onDefaultColorIndexChanged);
        disconnect(m_originModel, &creative_kernel::ModelN::prepareCheckMaterial, this, &ColorOperateMode::onApplyColor);

        onApplyColor();
        PaintOperateMode::onDettach();
    }
}

void ColorOperateMode::onKeyPress(QKeyEvent* e)
{
    if (e->modifiers() & Qt::ControlModifier)
    {
        int value = e->key() - Qt::Key_1;
        if (value >= 0 && value <= 8) 
        {
	        setColorIndex(value);
            m_keyPressStatus = false;
        }
    }
}

