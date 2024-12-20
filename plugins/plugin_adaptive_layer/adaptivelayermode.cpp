#include "adaptivelayermode.h"
#include "interface/eventinterface.h"
#include <interface/selectorinterface.h>
#include "qtuser3d/module/pickable.h"
#include "data/modeln.h"
#include "slice/adaptiveslice.h"
#include "interface/visualsceneinterface.h"
#include "interface/spaceinterface.h"
#include <interface/printerinterface.h>
#include "entity/indicatorpickable.h"
#include "us/usettings.h"
#include "kernel/kernel.h"
#include "adaptiveundocommand.h"
#include "data/modelspaceundo.h"
#include "kernel/kernelui.h"
#include "data/spaceutils.h"
#include "slice/adaptiveslice.h"
#include "interface/machineinterface.h"

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
    attachToNewGroup();
    
    popup_visible_ = true;
    installEventHandlers();
    creative_kernel::addModelNSelectorTracer(this);
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
    creative_kernel::removeModelNSelectorTracer(this);
    uninstallEventHandlers();

    dettachLayerHeightProfileTo(m_lastSelectedGroup);
    m_lastSelectedGroup = nullptr;
    m_groupMeshes.clear();

}
void AdaptiveLayerMode::adapted(const float percentage)
{
    if (m_groupMeshes.size() == 0)
    {
        qDebug() << "error: group mesh empty!";
        return;
    }

    auto layers = creative_kernel::getLayerHeightProfileAdaptive(m_groupMeshes, percentage);
    setLayerHeightProfileTo(getSelectedGroup(), layers, m_groupMeshes);
}

void AdaptiveLayerMode::smooth(const int radiusVal, bool keepMin)
{
    qDebug() << " smooth radiusVal =" << radiusVal << "  keepMin ="<< keepMin;
    
    if (m_groupMeshes.size() == 0)
    {
        qDebug() << "error: group mesh empty!";
        return;
    }

    auto group = getSelectedGroup();
    auto layers = creative_kernel::smoothHeightProfile(group->layerHeightProfile(), m_groupMeshes, radiusVal, keepMin);

    setLayerHeightProfileTo(group, layers, m_groupMeshes);
}
void AdaptiveLayerMode::stackUndo(creative_kernel::ModelN* model, const std::vector<double>& oldLayerData, const std::vector<double>& newLayerData)
{
    creative_kernel::ModelSpaceUndo* stack = creative_kernel::getKernel()->modelSpaceUndo();
    AdaptiveUndoCommand* command = new AdaptiveUndoCommand();
    command->setObjects(this, model);
    command->setData(oldLayerData, newLayerData);

    stack->executeCommand(command);
}
void AdaptiveLayerMode::restore(creative_kernel::ModelN* model, const std::vector<double>& layersData)
{
    getKernelUI()->switchMode(32);
    model->setLayerHeightProfile(layersData);
    model->useVariableLayerHeightEffect();
    //校验
    bool bRet = creative_kernel::_checkModelInsideVisibleLayers(true);
}
void AdaptiveLayerMode::reset()
{
    qDebug() << "AdaptiveLayerMode::reset()";
    if (m_groupMeshes.size() == 0)
    {
        qDebug() << "error: group mesh empty!";
        return;
    }

    setLayerHeightProfileTo(getSelectedGroup(), {}, m_groupMeshes);
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
    
    ModelN* model = checkPickModel(event->pos());
    if (model)
    {
        auto list = selectedGroups();
        if (!list.contains(model->getModelGroup()))
        {
            //pointSelect(event);
            selectGroup(model->getModelGroup(), false);
        }
    }
    MoveOperateMode::onLeftMouseButtonPress(event);
    m_leftPressStatus = false;
}

void AdaptiveLayerMode::onLeftMouseButtonRelease(QMouseEvent* event)
{
    MoveOperateMode::onLeftMouseButtonRelease(event);
    m_leftMoveStatus = false;
}

void AdaptiveLayerMode::onLeftMouseButtonMove(QMouseEvent* event)
{
    MoveOperateMode::onLeftMouseButtonMove(event);
    m_leftMoveStatus = false;
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
    attachToNewGroup();
}

void AdaptiveLayerMode::attachToNewGroup()
{
    creative_kernel::ModelGroup* newGroup = getSelectedGroup();
    if (m_lastSelectedGroup != nullptr && m_lastSelectedGroup == newGroup)
    {
        return;
    }

    if (m_lastSelectedGroup)
    {
        dettachLayerHeightProfileTo(m_lastSelectedGroup);
    }

    m_lastSelectedGroup = newGroup;
    m_groupMeshes.clear();

    if (m_lastSelectedGroup)
    {
        // collect all models meshs in the group
        QList<creative_kernel::ModelN*> msels = m_lastSelectedGroup->normalModels();
        for (auto m : msels)
        {
            m_groupMeshes.push_back(m->globalMesh());
        }

        const std::vector<double>& layers = m_lastSelectedGroup->layerHeightProfile();
        attachLayerHeightProfileTo(m_lastSelectedGroup, layers, m_groupMeshes);
    }
}

creative_kernel::ModelGroup* AdaptiveLayerMode::getSelectedGroup()
{
    creative_kernel::ModelGroup* theGroup = nullptr;

    QList<creative_kernel::ModelGroup*> mselGroups = creative_kernel::selectedGroups();
    if (mselGroups.size() == 1) {
        theGroup = mselGroups.at(0);
    }
    else {

        QList<creative_kernel::ModelN*> mselModels = creative_kernel::selectedParts();
        QList<creative_kernel::ModelGroup*> groups = creative_kernel::findRelativeGroups(mselModels);
        if (groups.size() == 1) {
            theGroup = groups.at(0);
        }
    }

    return theGroup;
}

void AdaptiveLayerMode::attachLayerHeightProfileTo(creative_kernel::ModelGroup* target, const std::vector<double>& layers, const std::vector<TriMeshPtr>& meshes)
{
    if (target == nullptr)
    {
        return;
    }

    auto layerHeight = creative_kernel::generateObjectLayers(layers, meshes);

    float minLayerHeight = creative_kernel::currentMinLayerHeight();
    float maxLayerHeight = creative_kernel::currentMaxLayerHeight();
    float uniqueLayerHeight = creative_kernel::currentProcessLayerHeight();
    QImage image = generateLayerHeightImage(layerHeight, minLayerHeight, maxLayerHeight, uniqueLayerHeight);

    target->setLayerHeightProfile(layers);
    target->setLayersTexture(image);

    QList<creative_kernel::ModelN*> msels = target->normalModels();
    for (auto m : msels)
    {
        m->useVariableLayerHeightEffect();
    }
}

void AdaptiveLayerMode::dettachLayerHeightProfileTo(creative_kernel::ModelGroup* target)
{
    if (target == nullptr)
        return;

    QList<creative_kernel::ModelN*> msels = target->normalModels();
    for (auto m : msels)
    {
        m->useDefaultModelEffect();
    }
}

void AdaptiveLayerMode::setLayerHeightProfileTo(creative_kernel::ModelGroup* target, const std::vector<double>& layers, const std::vector<TriMeshPtr>& meshes)
{
    if (target == nullptr)
        return;
    
    attachLayerHeightProfileTo(target, layers, meshes);

    QList<creative_kernel::ModelN*> msels = target->normalModels();
    for (auto m : msels)
    {
        auto oldLayer = m->layerHeightProfile();
        //add stack
        stackUndo(m, oldLayer, layers);
    }
    //校验
    bool bRet = creative_kernel::_checkModelInsideVisibleLayers(true);
}

QImage AdaptiveLayerMode::generateLayerHeightImage(const std::vector<double>& layers, float minLayerHeight, float maxLayerHeight, float uniqueLayerHeight)
{
    if (layers.empty() || minLayerHeight <= 0.0f || maxLayerHeight <= 0.0f || uniqueLayerHeight <= 0.0f)
    {
        return QImage();
    }

    if (maxLayerHeight < minLayerHeight || minLayerHeight > uniqueLayerHeight || uniqueLayerHeight > maxLayerHeight)
    {
        return QImage();
    }

    QVector4D orange(1.000000f, 0.427451f, 0.000000f, 1.0f);
    QVector4D grey(0.427451f, 0.427451f, 0.427451f, 1.0f);
    QVector4D green(0.090196f, 0.800000f, 0.372549f, 1.0f);

    int pixelHeight = 4096;

    double maxHeight = layers.back();
    std::vector<double>::const_iterator dataPtr = layers.begin();

    QImage image(1, pixelHeight, QImage::Format::Format_RGB888);

    for (int i = 0; i < pixelHeight; i++) {

        double currentH = i * maxHeight / (pixelHeight - 1);
        if (*dataPtr <= currentH && currentH <= *(dataPtr + 1)) {
        }
        else {
            dataPtr += 2;
        }

        double bottom = (*dataPtr);
        double top = *(dataPtr + 1);
        double middle = (top + bottom) * 0.5;
        double layerHeight = top - bottom; //��ǰ���
        double diff = abs(currentH - middle) / layerHeight * 2.0;
        double light = 1.0 - diff * diff;
        //light = sqrt(light);
#if DEBUG
        printf("light = %f;  layerHeight = %f\n", light, layerHeight);
#endif // DEBUG

        light = light * 0.5 + 0.5;

        QVector4D interposeColor;

        if (abs(uniqueLayerHeight - layerHeight) < 0.0001)
        {
            interposeColor = grey;
        }
        else {
            if (layerHeight > uniqueLayerHeight)
            {
                float interpose = (layerHeight - uniqueLayerHeight) / (maxLayerHeight - uniqueLayerHeight);
                interpose = std::min(std::max(interpose, 0.0f), 1.0f);
                interposeColor = grey * (1.0 - interpose) + orange * interpose;
            }
            else if (layerHeight < uniqueLayerHeight)
            {
                float interpose = (layerHeight - minLayerHeight) / (uniqueLayerHeight - minLayerHeight);
                interpose = std::min(std::max(interpose, 0.0f), 1.0f);
                interposeColor = green * (1.0 - interpose) + grey * interpose;
            }
        }

        QVector4D diffuseLight = interposeColor * light;

        image.setPixelColor(0, i, QColor(diffuseLight.x() * 255, diffuseLight.y() * 255, diffuseLight.z() * 255));
    }
    return image;
}
