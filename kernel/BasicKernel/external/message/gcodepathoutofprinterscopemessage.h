#ifndef GCODEPATH_OUTOF_PRINTER_SCOPE_MESSAGE_202405271526_H
#define GCODEPATH_OUTOF_PRINTER_SCOPE_MESSAGE_202405271526_H

#include <QObject>
#include "basickernelexport.h"
#include "external/message/messageobject.h"

namespace creative_kernel
{
	class GCodePathOutOfPrinterScopeMessage : public MessageObject
	{
		Q_OBJECT

	public:
		GCodePathOutOfPrinterScopeMessage(QObject* parent = NULL);
		~GCodePathOutOfPrinterScopeMessage();

	protected:
		virtual int codeImpl() override;
		virtual int levelImpl() override;
		virtual QString messageImpl() override;
		virtual QString linkStringImpl() override;
		virtual void handleImpl() override;

	private:

	};

};

#endif // GCODEPATH_OUTOF_PRINTER_SCOPE_MESSAGE_202405271526_H