#include "worldindicatorentity.h"
#include "indicatorpickable.h"

#include "qtuser3d/utils/texturecreator.h"
#include "qtuser3d/camera/cameracontroller.h"
#include "qtuser3d/geometry/worldindicatorgeometry.h"
#include "qtuser3d/refactor/xeffect.h"

#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QAttribute>

#include <QtCore/qmath.h>

namespace qtuser_3d {

	WorldIndicatorEntity::WorldIndicatorEntity(Qt3DCore::QNode* parent)
		: qtuser_3d::PickXEntity(parent),
		m_cameraController(nullptr),
		m_animation(nullptr),
		m_lambda(0.0f),
        m_lambdax(0.0f),
		m_theme(-1),
        m_length(100),
        m_tracer(nullptr)
	{
        IndicatorPickable *pickable = new IndicatorPickable(this);
        pickable->setPickableEntity(this);
        
        bindPickable(pickable);

        qtuser_3d::XRenderPass* viewPass = new qtuser_3d::XRenderPass("sceneindicator", this); 
        viewPass->addFilterKeyMask("view", 0);

        qtuser_3d::XRenderPass* pickPass = new qtuser_3d::XRenderPass("pickindicator", this);
        pickPass->setPassCullFace(Qt3DRender::QCullFace::Back);
        pickPass->setPassDepthTest(Qt3DRender::QDepthTest::Less);
        pickPass->addFilterKeyMask("pick", 0);
        pickPass->setPassBlend(Qt3DRender::QBlendEquationArguments::One, Qt3DRender::QBlendEquationArguments::Zero);

        qtuser_3d::XEffect* effect = new qtuser_3d::XEffect(this);
        effect->addRenderPass(viewPass);
        effect->addRenderPass(pickPass);
        setEffect(effect);

        Qt3DRender::QGeometry* g = qtuser_3d::createWorldIndicatorGeometry();
        setGeometry(g);

		//setScreenPos(QPoint(250, 250));
	}

	WorldIndicatorEntity::~WorldIndicatorEntity()
	{
		/*if (m_cameraController)
		{
			m_cameraController->screenCamera()->removeCameraObserver(this);
		}*/
	}

    /*qtuser_3d::Pickable* WorldIndicatorEntity::pickable() const
    {
        return m_pickable;
    }*/

    void WorldIndicatorEntity::setHighlightDirections(int dirs)
    {
        setParameter("highlightFaces", float(dirs));
    }

    void WorldIndicatorEntity::setSelectedDirections(int dirs)
    {
        adaptCamera(dirs);
    }

    void WorldIndicatorEntity::setLength(float length)
    {
        m_length = length;
    }

    void WorldIndicatorEntity::setScreenPos(const QPoint& p)
    {
        m_showOnPoint = p;

        if (m_cameraController)
        {
            QSize size = m_cameraController->screenCamera()->size();
            float x = float(p.x()) / size.width();
            float y = float(p.y()) / size.height();

            float w = m_length / size.width();
            float h = m_length / size.height();
            setViewport(x, y, w, h);
        }
    }
    void WorldIndicatorEntity::resetCameraAngle(const int angle)
    {
        adaptCamera(angle);
    }
    void WorldIndicatorEntity::resetCameraAngle() {
        constexpr int TOP{ 1 << 5 };
        constexpr int FRONT{ 1 << 0 };
        constexpr int DEFAUT_ANGLE{ TOP + FRONT};
        adaptCamera(DEFAUT_ANGLE);
    }

    void WorldIndicatorEntity::topCameraAngle() {
        constexpr int TOP{ 1 << 5 };
        adaptCamera(TOP);
    }
    void WorldIndicatorEntity::bottomCameraAngle() {
        constexpr int BOTTOM{ 1 << 4 };
        adaptCamera(BOTTOM);
    }
    void WorldIndicatorEntity::leftCameraAngle() {
        constexpr int LEFT{ 1 << 2 };
        adaptCamera(LEFT);
    }
     void WorldIndicatorEntity::rightCameraAngle() {
        constexpr int RIGHT{ 1 << 3 };
        adaptCamera(RIGHT);
    }
     void WorldIndicatorEntity::frontCameraAngle() {
        constexpr int FRONT{ 1 << 0 };
        adaptCamera(FRONT);
    }
     void WorldIndicatorEntity::backCameraAngle() {
        constexpr int BACK{ 1 << 1 };
        adaptCamera(BACK);
    }


    void WorldIndicatorEntity::setViewport(float x, float y, float w, float h)
    {
        QMatrix4x4 viewportMatrix;
        QMatrix4x4 m1, m2, m3;
        m1.translate(1.0f, 1.0f, 0.0f);
        m2.scale(w, h, 1.0f);
        m3.translate(2.0f * x - 1.0f, 2.0f * y - 1.0f);
        viewportMatrix = m3 * m2 * m1;
        setParameter("viewport_matrix", viewportMatrix);
    }

    void WorldIndicatorEntity::setCameraController(qtuser_3d::CameraController* cc)
    {
        if (m_cameraController)
        {
            m_cameraController->screenCamera()->removeCameraObserver(this);
            disconnect(m_cameraController->screenCamera()->camera(), SIGNAL(aspectRatioChanged(float)), this, SLOT(aspectRatioChanged(float)));
        }

        m_cameraController = cc;

        if (m_cameraController)
        {
            qtuser_3d::ScreenCamera* sc = m_cameraController->screenCamera();
            sc->addCameraObserver(this);
            connect(sc->camera(), SIGNAL(aspectRatioChanged(float)), this, SLOT(aspectRatioChanged(float)));
            onCameraChanged(sc);
        }
    }

    void WorldIndicatorEntity::setWorldIndicatorTracer(WorldIndicatorTracer* t)
    {
        m_tracer = t;
    }

    void WorldIndicatorEntity::adaptCamera(int dirs)
    {
        if (!m_cameraController)
            return;

        int  ifront = 1 << 0;
        int  iback = 1 << 1;
        int  ileft = 1 << 2;
        int  iright = 1 << 3;
        int  ibottom = 1 << 4;
        int  itop = 1 << 5;

        QVector3D vec(0.0f, 0.0f, 0.0f);

        if (dirs & ifront)
        {
            qDebug() << "front";
            vec += QVector3D(0.0f, -1.0f, 0.0f);
        }

        if (dirs & iback)
        {
            qDebug() << "back";
            vec += QVector3D(0.0f, 1.0f, 0.0f);
        }

        if (dirs & ileft)
        {
            qDebug() << "left";
            vec += QVector3D(-1.0f, 0.0f, 0.0f);
        }

        if (dirs & iright)
        {
            qDebug() << "right";
            vec += QVector3D(1.0f, 0.0f, 0.0f);
        }

        if (dirs & ibottom)
        {
            qDebug() << "bottom";
            vec += QVector3D(0.0f, 0.0f, -1.0f);
        }

        if (dirs & itop)
        {
            qDebug() << "top";
            vec += QVector3D(0.0f, 0.0f, 1.0f);
        }

        QVector3D viewDir = -vec;
        QVector3D up = QVector3D(0.0f, 0.0f, 1.0f);

        if (viewDir == up)
        {
            //m_cameraController->viewFromBottom();
            viewDir = QVector3D(0.0f, 0.0f, 1.0f);
            QVector3D right(1.0f, 0.0f, 0.0f);
            up = QVector3D::crossProduct(right, viewDir).normalized();
        }
        else if (viewDir == -up) {
            //m_cameraController->viewFromTop();
            viewDir = QVector3D(0.0f, 0.0f, -1.0f);
            QVector3D right(1.0f, 0.0f, 0.0f);
            up = QVector3D::crossProduct(right, viewDir).normalized();
        }
        else {
            /*QVector3D right = QVector3D::crossProduct(viewDir, up);
            m_cameraController->view(viewDir, right);*/
        }

        qtuser_3d::ScreenCamera* sc = m_cameraController->screenCamera();
        Qt3DRender::QCamera* camera = sc->camera();
        m_startDir = camera->viewVector();
        m_startUp = camera->upVector();

        m_endDir = viewDir;
        m_endUp = up;

        /* 把相机中心点设为箱体的底部中心，即返回初始位置 */
        m_startCenter = m_cameraController->getviewCenter();
        
        qtuser_3d::Box3D box;
        if (m_tracer)
        {
            box = m_tracer->resetCameraWithNewBox(this);
        }
        else {
            box = sc->box();
        }
        m_endCenter = box.center();
        m_endCenter.setZ(0);

        if (m_animation == nullptr)
        {
            QPropertyAnimation* animation = new QPropertyAnimation(this);
            animation->setTargetObject(this);
            animation->setEasingCurve(QEasingCurve::InOutQuad);
            animation->setPropertyName("lambda");
            animation->setStartValue(QVariant::fromValue(0.0f));
            animation->setEndValue(QVariant::fromValue(1.0f));
            animation->setDuration(500);
            animation->setLoopCount(1);

            m_animation.reset(animation);
        }
        m_animation->start();
    }

    void WorldIndicatorEntity::onCameraChanged(qtuser_3d::ScreenCamera* sc)
    {
        Qt3DRender::QCamera* camera = sc->camera();

        QVector3D entityPos = QVector3D(0.0, 0.0, 0.0);

        QVector3D viewDir = camera->viewVector();
        QVector3D newPosition = entityPos + viewDir.normalized() * -2.0;

        QMatrix4x4 model;
        model.translate(entityPos);
        model.rotate(90.0f, QVector3D(1.0, 0.0, 0.0));

        setParameter("model_matrix", model);
        
        QMatrix4x4 view;
        view.lookAt(newPosition, entityPos, camera->upVector());
        setParameter("view_matrix", QVariant(view));

        QMatrix4x4 projection;
        //projection.perspective(60.0f, 1.0, 1.0, 1000.0);
        projection.ortho(-1, 1, -1, 1, 1.0, 1000.0);

        setParameter("projection_matrix", QVariant(projection));
        setScreenPos(m_showOnPoint);
    }

    void WorldIndicatorEntity::setLambda(float lambda)
    {
        m_lambda = lambda;
        qDebug() << "setLambda:" << lambda;

        if (!m_cameraController)
            return;

        QVector3D up(0.0f, 0.0f, 1.0f);
        QQuaternion q1up = QQuaternion::rotationTo(up, m_startUp);
        QQuaternion q2up = QQuaternion::rotationTo(up, m_endUp);
        QQuaternion qup = QQuaternion::slerp(q1up, q2up, lambda);
        QVector3D newUp = qup * up;

        QVector3D dir(0.0f, 1.0f, 0.0f);
        QQuaternion q1dir = QQuaternion::rotationTo(dir, m_startDir);
        QQuaternion q2dir = QQuaternion::rotationTo(dir, m_endDir);
        QQuaternion qdir = QQuaternion::slerp(q1dir, q2dir, lambda);
        QVector3D newDir = qdir * dir;

        QVector3D newCenter = (m_endCenter - m_startCenter) * lambda + m_startCenter;

        QVector3D right = QVector3D::crossProduct(newDir, newUp);
        m_cameraController->view(newDir, right, &newCenter);

        onCameraChanged(m_cameraController->screenCamera());
    }

    float WorldIndicatorEntity::lambda() const
    {
        return m_lambda;
    }

    void WorldIndicatorEntity::setLambdax(float lambdax)
    {
        m_lambdax = lambdax;
        qDebug() << "setLambdax:" << lambdax;

        if (!m_cameraController)
            return;

        QVector3D up(0.0f, 0.0f, 1.0f);
        QQuaternion q1up = QQuaternion::rotationTo(up, m_startUp);
        QQuaternion q2up = QQuaternion::rotationTo(up, m_endUp);
        QQuaternion qup = QQuaternion::slerp(q1up, q2up, lambdax);
        QVector3D newUp = qup * up;

        QVector3D dir(0.0f, 1.0f, 0.0f);
        QQuaternion q1dir = QQuaternion::rotationTo(dir, m_startDir);
        QQuaternion q2dir = QQuaternion::rotationTo(dir, m_endDir);
        QQuaternion qdir = QQuaternion::slerp(q1dir, q2dir, lambdax);
        QVector3D newDir = qdir * dir;

        QVector3D newCenter = (m_endCenter - m_startCenter) * lambdax + m_startCenter;

        m_cameraController->viewEx(newDir, newUp, m_homePosition, m_homeViewCenter, newCenter);

        onCameraChanged(m_cameraController->screenCamera());
    }

    float WorldIndicatorEntity::lambdax() const
    {
        return m_lambdax;
    }

    void WorldIndicatorEntity::aspectRatioChanged(float aspectRatio)
    {
        onCameraChanged(m_cameraController->screenCamera());
    }

    /*void WorldIndicatorEntity::setupBasicTexture(const QUrl& url)
    {
        Qt3DRender::QTexture2D* t = createFromSource(url);
        setParameter("colorMap", QVariant::fromValue(t));
    }*/

    void WorldIndicatorEntity::setTheme(int theme)
    {
        m_theme = theme;
        updateBasicTexture();
    }

    void WorldIndicatorEntity::setupLightTexture(const QUrl& url)
    {
        if (m_lightTextureUrl == url) return;

        m_lightTextureUrl = url;
        updateBasicTexture();
    }

    void WorldIndicatorEntity::setupDarkTexture(const QUrl& url)
    {
        if (m_darkTextureUrl == url) return;

        m_darkTextureUrl = url;
        updateBasicTexture();
    }

    void WorldIndicatorEntity::updateBasicTexture()
    {
        if (m_theme == -1) return;

        switch (m_theme)
        {
        case 0:
        {
            if (m_darkTextureUrl.isEmpty() == false)
            {
                setParameter("colorMap", 0);
                Qt3DRender::QTexture2D* t = qtuser_3d::createFromSource(m_darkTextureUrl);
                if (t)
                {
                    setParameter("colorMap", QVariant::fromValue(t));
                }
                
            }
			setParameter("diffuseLowerLimit", 0.6);
			setParameter("diffuseUpperLimit", 1.1);
        }
        break;

        case 1:
        {
            if (m_lightTextureUrl.isEmpty() == false)
            {
                setParameter("colorMap", 0);
                Qt3DRender::QTexture2D* t = qtuser_3d::createFromSource(m_lightTextureUrl);
                if (t)
                {
                    setParameter("colorMap", QVariant::fromValue(t));
                }
            }
			setParameter("diffuseLowerLimit", 0.95);
			setParameter("diffuseUpperLimit", 1.1);
        }
        break;

        default:
            break;
        }

    }

    void WorldIndicatorEntity::freshTextures()
    {

#ifdef Q515
        setTheme(m_theme);
#endif
    }

    void WorldIndicatorEntity::resetHomeCamera(const QVector3D& homeDir, const QVector3D& homeUp, const QVector3D& homePosition, const QVector3D& homeViewCenter)
    {
        if (!m_cameraController)
            return;

        m_homePosition = homePosition;
        m_homeViewCenter = homeViewCenter;

        qtuser_3d::ScreenCamera* sc = m_cameraController->screenCamera();
        Qt3DRender::QCamera* camera = sc->camera();

        m_startDir = camera->viewVector();
        m_startUp = camera->upVector();

        m_endDir = homeDir;
        m_endUp = homeUp;

        /* 把相机中心点设为箱体的底部中心，即返回初始位置 */
        qtuser_3d::Box3D box = sc->box();
        m_startCenter = m_cameraController->getviewCenter();
        m_endCenter = box.center();

        if (m_animation == nullptr)
        {
            QPropertyAnimation* animation = new QPropertyAnimation(this);
            animation->setTargetObject(this);
            animation->setEasingCurve(QEasingCurve::InOutQuad);
            animation->setPropertyName("lambdax");
            animation->setStartValue(QVariant::fromValue(0.0f));
            animation->setEndValue(QVariant::fromValue(1.0f));
            animation->setDuration(500);
            animation->setLoopCount(1);

            m_animation.reset(animation);
        }
        m_animation->start();
    }

}