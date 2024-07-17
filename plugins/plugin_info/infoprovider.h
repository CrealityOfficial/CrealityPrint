#ifndef _INFOPROVIDER_1657009017064_H
#define _INFOPROVIDER_1657009017064_H
#include "data/interface.h"
#include "qtuser3d/module/pickableselecttracer.h"
#include <QtCore/QObject>
#include <QtGui/QVector3D>

namespace creative_kernel
{
	class InfoProvider : public QObject 
		, public SpaceTracer
		, public qtuser_3d::SelectorTracer
	{
		Q_OBJECT
		Q_PROPERTY(QVector3D size READ modelSize CONSTANT)
	public:
		InfoProvider(QObject* parent = nullptr);
		virtual ~InfoProvider();

		Q_INVOKABLE QString softVersion();
		Q_INVOKABLE QString buildTime();
		Q_INVOKABLE QString currentPrinter();
		Q_INVOKABLE QString modelName();
		Q_INVOKABLE QVector3D modelSize();
		Q_INVOKABLE void updateMachineInfo();
		Q_INVOKABLE int selectCount();
		Q_INVOKABLE int layerHeight();
		

	protected:
		void onSelectionsChanged() override;
		void selectChanged(qtuser_3d::Pickable* pickable) override;
		void onBoxChanged(const qtuser_3d::Box3D& box) override;
		void onSceneChanged(const qtuser_3d::Box3D& box) override;
		
	signals:
		void infoChanged();

	private:
		int m_layerCount = 0;
	};
}

#endif // _INFOPROVIDER_1657009017064_H