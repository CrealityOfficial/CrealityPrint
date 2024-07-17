#include "adaptivelayermode.h"
#include "interface/eventinterface.h"
#include <interface/selectorinterface.h>
#include "qtuser3d/module/pickable.h"
#include "data/modeln.h"
#include "slice/adaptiveslice.h"
#include "interface/modelinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/spaceinterface.h"
#include <interface/printerinterface.h>
#include "entity/indicatorpickable.h"
#include "interface/modelinterface.h"
#include "us/usettings.h"
#include "kernel/kernel.h"
#include "adaptiveundocommand.h"
#include "data/modelspaceundo.h"
#include "kernel/kernelui.h"

AdaptiveLayerMode::AdaptiveLayerMode(QObject* parent)
    : MoveOperateMode(parent)
    , popup_visible_(false)
{
}

AdaptiveLayerMode::~AdaptiveLayerMode()
{

}


void AdaptiveLayerMode::onAttach() 
{	

    QList<creative_kernel::ModelN*> msels = creative_kernel::selectionms();
    if (msels.size() == 1)
    {
        creative_kernel::ModelN* model = msels.first();
        //保持上次自适应后的渲染
        if(model->layerHeightProfile().size() > 0)
            model->useVariableLayerHeightEffect();
        else
        {
            model->setLayerHeightProfile({});
            model->useVariableLayerHeightEffect();
        }
    }
    popup_visible_ = true;
    installEventHandlers();
    creative_kernel::addSelectTracer(this);
}

void AdaptiveLayerMode::onDettach() 
{
    //if (m_originModel)
    //{
    //    disconnect(m_originModel, &creative_kernel::ModelN::defaultColorIndexChanged, this, &AdaptiveLayerMode::onDefaultColorIndexChanged);
    //    disconnect(m_originModel, &creative_kernel::ModelN::prepareCheckMaterial, this, &AdaptiveLayerMode::onApplyColor);

    //    onApplyColor();
    //    PaintOperateMode::onDettach();
    //}
    popup_visible_ = false;
    creative_kernel::removeSelectorTracer(this);
    uninstallEventHandlers();
    QList<creative_kernel::ModelN*> msels = creative_kernel::selectionms();
    if (msels.size() == 1)
    {
        creative_kernel::ModelN* model = msels.first();
        model->useDefaultModelEffect();
    }
}
void AdaptiveLayerMode::adapted(const float percentage)
{
    QList<creative_kernel::ModelN*> msels = creative_kernel::selectionms();
    if (msels.size() == 1)
    {
        //const creative_kernel::ModelN* oldmodel = msels.first();
        creative_kernel::ModelN* model = msels.first();
        auto oldLayer = model->layerHeightProfile();
        auto layers = creative_kernel::getLayerHeightProfileAdaptive(model->globalMesh().get(), percentage);
        model->setLayerHeightProfile(layers);
        model->useVariableLayerHeightEffect();

        //校验
        bool bRet = creative_kernel::_checkModelInsideVisibleLayers(true);
        //add stack
        stackUndo(model, oldLayer, layers);
    }
}

void AdaptiveLayerMode::smooth(const int radiusVal, bool keepMin)
{
    qDebug() << " smooth radiusVal =" << radiusVal << "  keepMin ="<< keepMin;
    QList<creative_kernel::ModelN*> msels = creative_kernel::selectionms();
    if (msels.size() == 1)
    {
        creative_kernel::ModelN* model = msels.first();
        if (model->layerHeightProfile().size() == 0)return;
        auto oldLayer = model->layerHeightProfile();
        auto layers = creative_kernel::smoothHeightProfile(model->layerHeightProfile(), model->globalMesh().get(), radiusVal, keepMin);
        model->setLayerHeightProfile(layers);
        model->useVariableLayerHeightEffect();

        //add stack
        stackUndo(model, oldLayer, layers);
    }
}
void AdaptiveLayerMode::stackUndo(creative_kernel::ModelN* model, const std::vector<double>& oldLayerData, const std::vector<double>& newLayerData)
{
    creative_kernel::ModelSpaceUndo* stack = creative_kernel::getKernel()->modelSpaceUndo();
    AdaptiveUndoCommand* command = new AdaptiveUndoCommand();
    command->setObjects(this, model);
    command->setData(oldLayerData, newLayerData);

    stack->push(command);
}
void AdaptiveLayerMode::restore(creative_kernel::ModelN* model, const std::vector<double>& layersData)
{
    selectOne(model);
    getKernelUI()->switchMode(32);
    model->setLayerHeightProfile(layersData);
    model->useVariableLayerHeightEffect();
    //校验
    bool bRet = creative_kernel::_checkModelInsideVisibleLayers(true);
}
void AdaptiveLayerMode::reset()
{
    qDebug() << "AdaptiveLayerMode::reset()";
    QList<creative_kernel::ModelN*> msels = creative_kernel::selectionms();
    if (msels.size() == 1)
    {
        creative_kernel::ModelN* model = msels.first();
        if (model->layerHeightProfile().size() > 0)
        {
            auto oldLayer = model->layerHeightProfile();
            model->setLayerHeightProfile({});
            model->useVariableLayerHeightEffect();
            bool bRet = creative_kernel::_checkModelInsideVisibleLayers(true);

            //add stack
            stackUndo(model, oldLayer, {});
        }   
    }
}
bool AdaptiveLayerMode::isPopupVisible() {
    return popup_visible_;
}

void AdaptiveLayerMode::installEventHandlers()
{
	creative_kernel::prependLeftMouseEventHandler(this);
    //creative_kernel::prependRightMouseEventHandler(this);
    //creative_kernel::addLeftMouseEventHandler(this);
}

void AdaptiveLayerMode::uninstallEventHandlers()
{
	creative_kernel::removeLeftMouseEventHandler(this);
    //creative_kernel::removeRightMouseEventHandler(this);
}


void AdaptiveLayerMode::onLeftMouseButtonPress(QMouseEvent* event)
{
    using namespace creative_kernel;
    ModelN* model = nullptr;
    qtuser_3d::Pickable* pickable = nullptr;
    int pID = -1;
    pickable = checkPickable(event->pos(), &pID);
    m_mode = TMode::null;
    m_tempLocal = QVector3D();
    if (pickable)
    {
        model = qobject_cast<ModelN*>(pickable);
    }

    if (model)
    {
        QList<ModelN*> msels = selectionms();
        if (!msels.isEmpty() && model != msels.at(0))
        {
            m_lastSelectModelPt = msels.at(0);
            selectOne(model);
        }
        setMode(TMode::sp);
        resetSpacePoint(event->pos());
    }

    m_leftPressStatus = false;
    (void)event;
}
void AdaptiveLayerMode::onLeftMouseButtonRelease(QMouseEvent* event)
{
    if (m_mode == TMode::null || m_tempLocal.isNull())
        return;

    auto models = creative_kernel::selectionms();
    if (models.isEmpty())
        return;

    if (!m_tempLocal.isNull())
    {
        QList<QVector3D> starts;
        QList<QVector3D> ends;

        for (auto m : models)
        {
            QVector3D originLocalPosition = m->localPosition() - m_tempLocal;

            starts << originLocalPosition;

            ends << m->localPosition();
        }

        moveModels(models, starts, ends, true);
    }

    m_mode = TMode::null;
    m_leftReleaseStatus = true;
    (void)event;
}
void AdaptiveLayerMode::onLeftMouseButtonMove(QMouseEvent* event)
{
    using namespace creative_kernel;
    if (m_mode == TMode::null)
        return;

    auto models = creative_kernel::selectionms();
    if (models.isEmpty())
        return;

    QVector3D p = process(event->pos());
    QVector3D delta = clip(p - m_spacePoint);
    QVector3D moveDelta = delta - m_tempLocal;
    m_tempLocal = delta;

    for (auto m : models)
    {
        QVector3D newLocalPosition = m->localPosition() + moveDelta;
        moveModel(m, m->localPosition(), newLocalPosition, false);
    }
    requestVisUpdate(true);
    m_leftMoveStatus = true;
}

void AdaptiveLayerMode::onLeftMouseButtonClick(QMouseEvent* event)
{
    using namespace creative_kernel;
    qtuser_3d::Pickable* pickable = nullptr;
    int pID = -1;
    pickable = checkPickable(event->pos(), &pID);

    //选中右上角视图。需要响应click.
    if (pickable)
    {
        QString typeName = QStringLiteral("%1").arg(typeid(*pickable).name());
        if (typeName.contains("qtuser_3d::IndicatorPickable"))
        {
            m_leftClickStatus = true;
            return;
        }
    }
    
    //禁用click。点击空白不退出功能
    m_leftClickStatus = false;

}

void AdaptiveLayerMode::onSelectionsChanged()
{
    qDebug() << "AdaptiveLayerMode::onSelectionsChanged()";
    QList<creative_kernel::ModelN*> msels = creative_kernel::selectionms();
  
    if (msels.size() == 1)
    {
        if (m_lastSelectModelPt != msels.first())
        {
            if (!m_lastSelectModelPt.isNull())
                m_lastSelectModelPt->useDefaultModelEffect();
            if (msels.first()->layerHeightProfile().size() > 0)
            {
                msels.first()->useVariableLayerHeightEffect();
            }
            else
            {
                msels.first()->setLayerHeightProfile({});
                msels.first()->useVariableLayerHeightEffect();
            }
        }
    }

}
void AdaptiveLayerMode::selectChanged(qtuser_3d::Pickable* pickable)
{
    qDebug() << "AdaptiveLayerMode::selectChanged()";
}
