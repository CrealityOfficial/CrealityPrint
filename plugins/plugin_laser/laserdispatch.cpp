#include "laserdispatch.h"
#include "trimesh2/TriMesh.h"

#include "qtuser3d/camera/cameracontroller.h"
#include "qtusercore/property/qmlpropertysetter.h"

#include "interface/camerainterface.h"
#include "interface/selectorinterface.h"

#include "data/modeln.h"
#include <QTimer>
using namespace creative_kernel;
using namespace qtuser_3d;
using namespace qtuser_qml;

LaserDispatch::LaserDispatch(QObject* parent)
	:QObject(parent)
	, m_object(nullptr)
{
}

LaserDispatch::~LaserDispatch()
{
}

void LaserDispatch::onSelectionsChanged()
{
    QList<ModelN*> selections = selectionms();

    if (selections.size() == 1)
    {
        creative_kernel::ModelN* model = selections[0];
        qtuser_3d::Box3D box = model->globalSpaceBox();//model->localBox();
        trimesh::TriMesh* mesh = model->mesh();
        int verticessize = mesh->vertices.size();
        int facesize = mesh->faces.size();
        writeProperty(m_object, "modelname", model->objectName());
        writeProperty(m_object, "verticessize", verticessize);

        //QQmlProperty::write(m_object, "facesize", facesize );
        writeProperty(m_object, "size", box.max - box.min);
        QTimer::singleShot(200, this, SLOT(enableVisible()));

    }
    else 
    {
        QTimer::singleShot(200, this, SLOT(disableVisible()));

    }
}
void LaserDispatch::enableVisible()
{

    writeProperty(m_object, "visible", true);
}
void LaserDispatch::disableVisible()
{
     writeProperty(m_object, "visible", false);

}
void LaserDispatch::selectChanged(Pickable* pickable)
{

}

void LaserDispatch::onBoxChanged(const qtuser_3d::Box3D& box)
{
}

void LaserDispatch::onSceneChanged(const qtuser_3d::Box3D& box)
{
    QList<ModelN*> selections = selectionms();
    if (selections.size() == 1)
    {
        creative_kernel::ModelN* model = selections[0];
        qtuser_3d::Box3D box = model->globalSpaceBox();
        trimesh::TriMesh* mesh = model->mesh();
        int verticessize = mesh->vertices.size();
        int facesize = mesh->faces.size();
        writeProperty(m_object, "modelname", model->objectName());
        writeProperty(m_object, "verticessize", verticessize);

        //QQmlProperty::write(m_object, "facesize", facesize );
        writeProperty(m_object, "size", box.max - box.min);
        QTimer::singleShot(200, this, SLOT(enableVisible()));
    }
    else
    {
        QTimer::singleShot(200, this, SLOT(disableVisible()));
    }

}
void LaserDispatch::setObject(QObject* object)
{
	m_object = object;

    QObject* controller = cameraController();
    writeObjectProperty(m_object, "camera", controller);
}

QString LaserDispatch::text()
{
	return m_text;
}

QVector3D LaserDispatch::size()
{
    return QVector3D(300,300,300);
}

void LaserDispatch::setText(const QString& text)
{

}

