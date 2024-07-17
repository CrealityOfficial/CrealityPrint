#include "wipetowerpickable.h"

namespace creative_kernel
{
	WipeTowerPickable::WipeTowerPickable(QObject* parent)
		:Pickable(parent)
	{
		setObjectName("WipeTowerPickable");
	}
	
	WipeTowerPickable::~WipeTowerPickable()
	{
	}


}