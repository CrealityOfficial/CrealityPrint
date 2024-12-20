#ifndef PLATE_PICKABLE_202401221903_H
#define PLATE_PICKABLE_202401221903_H

#include "basickernelexport.h"
#include "qtuser3d/module/pickable.h"

namespace creative_kernel {

	class Printer;
	class BASIC_KERNEL_API PlatePickable : public qtuser_3d::Pickable
	{
		Q_OBJECT
	public:
		PlatePickable(Printer* printer, QObject* parent = nullptr);
		virtual ~PlatePickable();

		Printer* printer();
		void setPrinter(Printer* printer);

	private:
		Printer* m_printer {NULL};

	};
}

#endif // !PLATE_PICKABLE_202401221903_H


