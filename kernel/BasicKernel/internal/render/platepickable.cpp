#include "platepickable.h"

namespace creative_kernel
{
	PlatePickable::PlatePickable(Printer* printer, QObject* parent)
		:Pickable(parent),
		 m_printer(printer)
	{
		setObjectName("PlatePickable");
		setNoPrimitive(true);
		setEnableSelect(true);
	}
	
	PlatePickable::~PlatePickable()
	{
	}

	Printer* PlatePickable::printer()
	{
		return m_printer;
	}

	void PlatePickable::setPrinter(Printer* printer)
	{
		m_printer = printer;
	}

}