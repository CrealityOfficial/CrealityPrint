#include "translateop.h"

#include "qtuser3d/math/space3d.h"
#include "qtuser3d/trimesh2/conv.h"
#include "pickop.h"

#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/camerainterface.h"
#include "interface/spaceinterface.h"
#include "interface/eventinterface.h"
#include "kernel/kernelui.h"

namespace creative_kernel
{
    PickOp::PickOp(QObject* parent)
        :SceneOperateMode(parent)
        , m_mode(TMode::null)
        , m_selectedModel(nullptr)
    {
        m_helperEntity = new qtuser_3d::TranslateHelperEntity(nullptr, HELPERTYPE_AXIS_ALL);
        moveEnable = false;
    }

    PickOp::~PickOp()
    {
    }

    void PickOp::onAttach()
    {

        addModelNSelectorTracer(this);
        onSelectionsChanged();

        addLeftMouseEventHandler(this);
        addKeyEventHandler(this);
        requestVisUpdate(true);
    }

    void PickOp::onDettach()
    {

        //setSelectedModel(nullptr);
        m_selectedModels.clear();
        removeModelNSelectorTracer(this);

        removeLeftMouseEventHandler(this);
        removeKeyEventHandler(this);

        requestVisUpdate(true);
    }

    void PickOp::onLeftMouseButtonPress(QMouseEvent* event)
    {
        moveEnable = false;
        m_mode = TMode::null;
        m_saveLocalPositions.clear();
        for (size_t i = 0; i < m_selectedModels.size(); i++)
        {
            qtuser_3d::Pickable* pickable = checkPickable(event->pos(), nullptr);

            if (pickable == m_selectedModels[i])
            {
                moveEnable = true;
            }

            if (pickable == m_helperEntity->xArrowPickable()) m_mode = TMode::x;
            else if (pickable == m_helperEntity->yArrowPickable()) m_mode = TMode::y;
            else if (pickable == m_helperEntity->zArrowPickable()) m_mode = TMode::z;
            else if (pickable != nullptr) m_mode = TMode::sp;

            qDebug() << "TMode:" << (int)m_mode;

            m_spacePoint = process(event->pos());
            m_saveLocalPositions.push_back(m_selectedModels[i]->localPosition());
        }


    }

    void PickOp::onLeftMouseButtonRelease(QMouseEvent* event)
    {
        if (!moveEnable)
        {
            return;
        }

        for (size_t i = 0; i < m_selectedModels.size(); i++)
        {
            if (m_mode != TMode::null)
            {	//
                QVector3D p = process(event->pos());
                QVector3D delta = clip(p - m_spacePoint);
                QVector3D newLocalPosition = m_saveLocalPositions[i] + delta;
                moveModel(m_selectedModels[i], m_saveLocalPositions[i], newLocalPosition, true);
                requestVisUpdate(true);
                emit positionChanged();
            }
            //m_saveLocalPositions[i] = m_selectedModels[i]->localPosition();
        }

        //if (m_selectedModel && (m_mode != TMode::null))
        //{	//
        //	QVector3D p = process(event->pos());
        //	QVector3D delta = clip(p - m_spacePoint);
        //	QVector3D newLocalPosition = m_saveLocalPosition + delta;

        //	moveModel(m_selectedModel, m_saveLocalPosition, newLocalPosition, true);

        //	requestVisUpdate(true);
        //	emit positionChanged();
        //}
    }

    void PickOp::onLeftMouseButtonMove(QMouseEvent* event)
    {
        if (!moveEnable)
        {
            return;
        }

        for (size_t i = 0; i < m_selectedModels.size(); i++)
        {
            if (m_mode != TMode::null)
            {
                QVector3D p = process(event->pos());
                QVector3D delta = clip(p - m_spacePoint);

                moveModel(m_selectedModels[i], m_saveLocalPositions[i], m_saveLocalPositions[i] + delta);

                requestVisUpdate(true);
                emit positionChanged();
            }
        }
        //if (m_selectedModel && (m_mode != TMode::null))
        //{
        //	QVector3D p = process(event->pos());
        //	QVector3D delta = clip(p - m_spacePoint);
        //
        //	moveModel(m_selectedModel, m_saveLocalPosition, m_saveLocalPosition + delta);

        //	requestVisUpdate(true);
        //	emit positionChanged();
        //}
    }

    void PickOp::onLeftMouseButtonClick(QMouseEvent* event)
    {

    }

    void PickOp::onKeyPress(QKeyEvent* event)
    {
        QList<creative_kernel::ModelN*> selections = selectionms();
        if (selections.size() == 0)
            return;

        QVector3D delta;
        bool needUpdate = false;

        //当用户按下-->键时
        if (event->key() == Qt::Key_Right)
        {
            delta = QVector3D(1.0f, 0.0f, 0.0f);
            needUpdate = true;
        }
        //当用户按下<--键时
        else if (event->key() == Qt::Key_Left)
        {
            delta = QVector3D(-1.0f, 0.0f, 0.0f);
            needUpdate = true;
        }
        //当用户按下up键时
        else if (event->key() == Qt::Key_Up)
        {
            delta = QVector3D(0.0f, 1.0f, 0.0f);
            needUpdate = true;
        }
        //当用户按下Down键时
        else if (event->key() == Qt::Key_Down)
        {
            delta = QVector3D(0.0f, -1.0f, 0.0f);
            needUpdate = true;
        }

        if (needUpdate)
        {
            for (size_t i = 0; i < selections.size(); i++)
            {
                QVector3D localPosition = selections.at(i)->localPosition();
                moveModel(selections.at(i), localPosition, localPosition + delta);
            }
            requestVisUpdate(true);
            emit positionChanged();
        }
    }

    void PickOp::onKeyRelease(QKeyEvent* event)
    {
    }

    void PickOp::onSelectionsChanged()
    {
        QList<creative_kernel::ModelN*> selections = selectionms();
        setSelectedModel(selections);
    }

    void PickOp::setSelectedModel(creative_kernel::ModelN* model)
    {
        //trace selected node
        m_selectedModel = model;

#if 0
        TranslateMode* mode = qobject_cast<TranslateMode*>(parent());
        if (!m_selectedModel)
        {
            mode->setSource("qrc:/kernel/qml/NoSelect.qml");
        }
        else {
            mode->setSource("qrc:/kernel/qml/MovePanel.qml");
        }

        qtuser_qml::ToolCommandCenter* left = getUILCenter();
        left->changeCommand(mode);
#endif
        buildFromSelections();
    }

    void PickOp::setSelectedModel(QList<creative_kernel::ModelN*> models)
    {
        m_selectedModels = models;
        m_saveLocalPositions.clear();
        for (size_t i = 0; i < m_selectedModels.size(); i++)
        {
            m_saveLocalPositions.push_back(m_selectedModels[i]->localPosition());

        }
        buildFromSelections();
    }

    void PickOp::buildFromSelections()
    {

    }
    void PickOp::center()
    {
        for (size_t i = 0; i < m_selectedModels.size(); i++)
        {
            if (m_selectedModels[i])
            {
                QVector3D oldLocalPosition = m_selectedModels[i]->localPosition();
                qtuser_3d::Box3D box = qtuser_3d::triBox2Box3D(getBaseBoundingBox());
                QVector3D size = box.size();
                QVector3D center = box.center();
                center.setZ(center.z() - size.z() / 2.0f);

                moveModel(m_selectedModels[i], oldLocalPosition, m_selectedModels[i]->mapGlobal2Local(center), true);

                updateHelperEntity();
                requestVisUpdate(true);

                emit positionChanged();
            }
        }


    }
    void PickOp::reset()
    {

    }

    QVector3D PickOp::position()
    {

        if (1 == m_selectedModels.size())
        {
            return m_selectedModels[0]->localPosition();

        }
        return QVector3D(0.0f, 0.0f, 0.0f);
    }

    void PickOp::setPosition(QVector3D position)
    {
        if (1 == m_selectedModels.size())
        {
            moveModel(m_selectedModels[0], QVector3D(), m_selectedModels[0]->mapGlobal2Local(position));
            updateHelperEntity();
            requestVisUpdate(true);
            emit positionChanged();
        }
        else
        {
            for (size_t i = 0; i < m_selectedModels.size(); i++)
            {
                moveModel(m_selectedModels[i], m_selectedModels[i]->localPosition(), m_selectedModels[i]->localPosition() + position);
            }
            if (m_selectedModels.size())
            {
                updateHelperEntity();
                requestVisUpdate(true);
                emit positionChanged();
            }
        }
    }

    void PickOp::setBottom()
    {
        if (m_selectedModels.size())
        {
            qtuser_3d::Box3D box = m_selectedModels[m_selectedModels.size() - 1]->globalSpaceBox();
            QVector3D zoffset = QVector3D(0.0f, 0.0f, -box.min.z());
            QVector3D localPosition = m_selectedModels[m_selectedModels.size() - 1]->localPosition();

            moveModel(m_selectedModels[m_selectedModels.size() - 1], localPosition, localPosition + zoffset, true);

            updateHelperEntity();
            requestVisUpdate(true);
            emit positionChanged();
        }
    }

    QVector3D PickOp::process(const QPoint& point)
    {
        qtuser_3d::Ray ray = visRay(point);

        QVector3D planeCenter;
        QVector3D planeDir;
        getProperPlane(planeCenter, planeDir, ray);

        QVector3D c;
        qtuser_3d::lineCollidePlane(planeCenter, planeDir, ray, c);
        if (c.x() > PosMax) { c.setX(PosMax); }
        if (c.y() > PosMax) { c.setY(PosMax); }
        if (c.z() > PosMax) { c.setZ(PosMax); }
        return c;
    }

    void PickOp::getProperPlane(QVector3D& planeCenter, QVector3D& planeDir, qtuser_3d::Ray& ray)
    {
        planeCenter = QVector3D(0.0f, 0.0f, 0.0f);
        qtuser_3d::Box3D box = m_selectedModels[m_selectedModels.size() - 1]->globalSpaceBox();
        planeCenter = box.center();

        QList<QVector3D> dirs;
        if (m_mode == TMode::x)  // x
        {
            dirs << QVector3D(0.0f, 1.0f, 0.0f);
            dirs << QVector3D(0.0f, 1.0f, 1.0f);
        }
        else if (m_mode == TMode::y)
        {
            dirs << QVector3D(1.0f, 0.0f, 0.0f);
            dirs << QVector3D(0.0f, 0.0f, 1.0f);
        }
        else if (m_mode == TMode::z)
        {
            dirs << QVector3D(1.0f, 0.0f, 0.0f);
            dirs << QVector3D(0.0f, 1.0f, 0.0f);
        }
        else if (m_mode == TMode::sp)
        {
            dirs << QVector3D(0.0f, 0.0f, 1.0f);
            dirs << QVector3D(0.0f, 0.0f, 1.0f);
        }

        float d = -1.0f;
        for (QVector3D& n : dirs)
        {
            float dd = QVector3D::dotProduct(n, ray.dir);
            if (qAbs(dd) > d)
            {
                planeDir = n;
            }
        }
    }

    QVector3D PickOp::clip(const QVector3D& delta)
    {
        QVector3D clipDelta = delta;
        if (m_mode == TMode::x)  // x
        {
            clipDelta.setY(0.0f);
            clipDelta.setZ(0.0f);
        }
        else if (m_mode == TMode::y)
        {
            clipDelta.setX(0.0f);
            clipDelta.setZ(0.0f);
        }
        else if (m_mode == TMode::z)
        {
            clipDelta.setY(0.0f);
            clipDelta.setX(0.0f);
        }

        return clipDelta;
    }

    void PickOp::updateHelperEntity()
    {


    }

    bool PickOp::shouldMultipleSelect()
    {
        return true;
    }
}

