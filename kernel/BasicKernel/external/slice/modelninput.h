#ifndef CREATIVE_KERNEL_MODELNINPUT_1677497847181_H
#define CREATIVE_KERNEL_MODELNINPUT_1677497847181_H
#include "modelinput.h"
#include "data/modeln.h"

namespace creative_kernel
{
	class Printer;
	class ModelNInput : public ModelInput
	{
	public:
		ModelNInput(ModelN* model, Printer* printer, QObject* parent = nullptr);
		virtual ~ModelNInput();

		std::vector<double> layerHeightProfile();
	protected:
		ModelN* m_model;
	};
}

#endif // CREATIVE_KERNEL_MODELNINPUT_1677497847181_H