#ifndef CREATIVE_KERNEL_MODELGROUP_1592037009137_H
#define CREATIVE_KERNEL_MODELGROUP_1592037009137_H
#include "basickernelexport.h"
#include "qtuser3d/module/node3d.h"

namespace us
{
	class USettings;
}

namespace creative_kernel
{
	class ModelN;
	class BASIC_KERNEL_API ModelGroup : public qtuser_3d::Node3D
	{
		Q_OBJECT
	public:
		ModelGroup(QObject* parent = nullptr);
		virtual ~ModelGroup();

		void addModel(ModelN* model);
		void removeModel(ModelN* model);
		void setChildrenVisibility(bool visibility);

		QList<ModelN*> models();

		us::USettings* setting();
	protected:
		us::USettings* m_setting;
	};
}
#endif // CREATIVE_KERNEL_MODELGROUP_1592037009137_H