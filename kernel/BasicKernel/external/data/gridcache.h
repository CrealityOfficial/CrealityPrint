#ifndef CREATIVE_KERNEL_GRIDCACHE_1594112048273_H
#define CREATIVE_KERNEL_GRIDCACHE_1594112048273_H
#include "basickernelexport.h"
#include "trimesh2/Vec.h"
#include <QtCore/QMap>

namespace mmesh
{
	class Box2DGrid;
}

namespace creative_kernel
{
	class ModelN;
	class BASIC_KERNEL_API GridCache : public QObject
	{
		Q_OBJECT
	public:
		GridCache(QObject* parent = nullptr);
		virtual ~GridCache();

		void build(QList<ModelN*>& models, bool buildGlobal = false);
		void clear();

		int size() const;

		mmesh::Box2DGrid* grid(ModelN* model);
		ModelN* check(trimesh::vec2 xy);
	protected:
		QMap<ModelN*, mmesh::Box2DGrid*> m_grids;
	};
}
#endif // CREATIVE_KERNEL_GRIDCACHE_1594112048273_H