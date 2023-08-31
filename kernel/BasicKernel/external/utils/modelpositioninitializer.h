#ifndef _creative_kernel_MODELPOSITIONINITIALIZER_1589275563011_H
#define _creative_kernel_MODELPOSITIONINITIALIZER_1589275563011_H
#include "basickernelexport.h"
#include "data/modeln.h"
#include "qtusercore/module/progressor.h"

namespace creative_kernel
{
	class ModelSpace;
	class BASIC_KERNEL_API ModelPositionInitializer: public QObject
	{
	public:
		ModelPositionInitializer(QObject* parent = nullptr);
		virtual ~ModelPositionInitializer();

		static void layout(ModelN* model, qtuser_core::Progressor* progressor,bool bAdaption = true);
		static void layoutBelt(ModelN* model, qtuser_core::Progressor* progressor, bool bAdaption = true);

		static bool nestLayout(ModelN* model);
	};
}
#endif // _creative_kernel_MODELPOSITIONINITIALIZER_1589275563011_H
