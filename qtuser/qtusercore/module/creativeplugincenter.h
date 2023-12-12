#ifndef _NULLSPACE_CREATIVEPLUGINCENTER_1589530786572_H
#define _NULLSPACE_CREATIVEPLUGINCENTER_1589530786572_H
#include "qtusercore/qtusercoreexport.h"
#include "qtusercore/module/creativeinterface.h"
#include <QtCore/QObject>
#include <QtCore/QMap>

namespace qtuser_core
{
	class QTUSER_CORE_API CreativePluginCenter : public QObject
	{
		Q_OBJECT
	public:
		CreativePluginCenter(QObject* parent = nullptr);
		virtual ~CreativePluginCenter();

		void load();
		void initialize();
		void uninitialize();
		CreativeInterface* getPluginByName(QString name);
	protected:
		QMap<QString, CreativeInterface*> m_interfaces;
	};
}
#endif // _NULLSPACE_CREATIVEPLUGINCENTER_1589530786572_H
