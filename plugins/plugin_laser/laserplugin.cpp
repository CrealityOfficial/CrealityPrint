#include "laserplugin.h"
#include "laserdispatch.h"
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlProperty>
#include "interface/camerainterface.h"
#include "interface/selectorinterface.h"
#include "interface/spaceinterface.h"

#include "qtuser3d/camera/cameracontroller.h"
#include "qtusercore/property/qmlpropertysetter.h"

using namespace creative_kernel;
using namespace qtuser_3d;
using namespace qtuser_qml;

LaserPlugin::LaserPlugin(QObject* parent)
	: QObject(parent)
    , m_lasterscene(nullptr)
    , m_plotterscene(nullptr)

{
}

LaserPlugin::~LaserPlugin()
{
}

QString LaserPlugin::name()
{
	return "LaserPlugin";
}

QString LaserPlugin::info()
{
	return "";
}

void LaserPlugin::onNewDrawObject(DrawObject::SHAPETYPE shaptype)
{
}

void LaserPlugin::initialize()
{
    m_lasterscene = new LaserScene(this, LaserScene::SCENETYPE::LASER);
    m_plotterscene = new LaserScene(this, LaserScene::SCENETYPE::PLOTTER);
}

void LaserPlugin::uninitialize()
{
    m_lasterscene->uninitialize();
    m_plotterscene->uninitialize();
}

