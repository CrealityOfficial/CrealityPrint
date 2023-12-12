#ifndef CREATIVE_KERNEL_INTERFACE_1595470868902_H
#define CREATIVE_KERNEL_INTERFACE_1595470868902_H
#include "data/header.h"
#include "data/kernelenum.h"
#include "qtuser3d/math/box3d.h"
#include "qtusercore/module/progressor.h"

namespace creative_kernel
{
	class UIVisualTracer
	{
	public:
		virtual ~UIVisualTracer() {};

		virtual void onThemeChanged(ThemeCategory category) = 0;
		virtual void onLanguageChanged(MultiLanguage language) = 0;
		//virtual void name() = 0;
	};

	class ModelN;
	class SpaceTracer
	{
	public:
		virtual ~SpaceTracer() {}

		virtual void onBoxChanged(const qtuser_3d::Box3D& box) = 0;
		virtual void onSceneChanged(const qtuser_3d::Box3D& box) = 0;

		virtual void onModelToAdded(ModelN* model) {}
		virtual void onModelToRemoved(ModelN* model) {}

		virtual void onModelAdded(ModelN* model) {}
		virtual void onModelRemoved(ModelN* model) {}
	};

	class MeshIOInterface
	{
	public:
		virtual ~MeshIOInterface() {}

		virtual QString description() = 0;
		virtual QStringList saveSupportExtension() = 0;  // for example "stl ply"
		virtual QStringList loadSupportExtension() = 0;  // for example "stl"

		virtual trimesh::TriMesh* load(const QString& name, qtuser_core::Progressor* progressor) = 0;
		virtual void save(trimesh::TriMesh* mesh, const QString& name, qtuser_core::Progressor* progressor) = 0;
	};

	class KernelPhase
	{
	public:
		virtual ~KernelPhase() {}

		virtual void onStartPhase() = 0;
		virtual void onStopPhase() = 0;
	};

	class CloseHook
	{
	public:
		virtual ~CloseHook() {}

		virtual void onWindowClose() = 0;
	};
}
#endif // CREATIVE_KERNEL_INTERFACE_1595470868902_H