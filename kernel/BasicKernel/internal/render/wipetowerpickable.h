#ifndef WIPE_TOWER_PICKABLE_202401221903_H
#define WIPE_TOWER_PICKABLE_202401221903_H

#include "basickernelexport.h"
#include "qtuser3d/module/pickable.h"

namespace creative_kernel {

	class BASIC_KERNEL_API WipeTowerPickable : public qtuser_3d::Pickable
	{
		Q_OBJECT
	public:
		WipeTowerPickable(QObject* parent = nullptr);
		virtual ~WipeTowerPickable();

	};
}

#endif // !WIPE_TOWER_PICKABLE_202401221903_H


