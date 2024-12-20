#ifndef _INFOPROVIDER_1657009017064_H
#define _INFOPROVIDER_1657009017064_H
#include "data/interface.h"
#include <QtCore/QObject>
#include <QtGui/QVector3D>

namespace creative_kernel
{
	class InfoProvider : public QObject 
		, public SpaceTracer
		, public ModelNSelectorTracer
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

		Q_INVOKABLE bool isNeedRepair();
		Q_INVOKABLE void repairModel();
		

	protected:
		void onSelectionsChanged() override;
		void onBoxChanged(const trimesh::dbox3& box) override;
		void onSceneChanged(const trimesh::dbox3& box) override;
		
	signals:
		void infoChanged();
		void repairSuccess();

	private:
		int m_layerCount = 0;
	};
}

#endif // _INFOPROVIDER_1657009017064_H