#ifndef CXGCODE_GCODEVIEWEFFECT_1683805686161_H
#define CXGCODE_GCODEVIEWEFFECT_1683805686161_H
#include "cxgcode/interface.h"
#include "qtuser3d/refactor/xeffect.h"

namespace cxgcode
{
	class GCodeViewEffect : public qtuser_3d::XEffect
	{
	public:
		GCodeViewEffect(Qt3DCore::QNode* parent = nullptr);
		virtual ~GCodeViewEffect();

	protected:

	};
}

#endif // CXGCODE_GCODEVIEWEFFECT_1683805686161_H