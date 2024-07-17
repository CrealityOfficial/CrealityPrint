#ifndef _NULLSPACE_LASERPLUGIN_1589979999085_H
#define _NULLSPACE_LASERPLUGIN_1589979999085_H
#include "qtusercore/module/creativeinterface.h"
#include <QtQml/QQmlComponent>
#include "laserscene.h"
#include "drawobject.h"
class LaserDispatch;
class ZSliderInfo;
class LaserPlugin: public QObject, public CreativeInterface
{
	Q_OBJECT
	//Q_PLUGIN_METADATA(IID "creative.InfoPlugin")
	//Q_INTERFACES(CreativeInterface)
public:
    LaserPlugin(QObject* parent = nullptr);
    virtual ~LaserPlugin();

	QString name() override;
	QString info() override;

	void initialize() override;
	void uninitialize() override;
public slots:
    void onNewDrawObject(DrawObject::SHAPETYPE shaptype);
protected:


   LaserScene* m_lasterscene;
   LaserScene* m_plotterscene;

};
#endif // _NULLSPACE_LASERPLUGIN_1589979999085_H
