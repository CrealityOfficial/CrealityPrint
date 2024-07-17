#ifndef CREATIVE_KERNEL_PREVIEW_EMPTY_MESSAGE_202312131652_H
#define CREATIVE_KERNEL_PREVIEW_EMPTY_MESSAGE_202312131652_H

#include <QObject>
#include "basickernelexport.h"
#include "external/message/messageobject.h"

namespace creative_kernel
{
	class PreviewEmptyMessage : public MessageObject
	{
		Q_OBJECT

	public:
		PreviewEmptyMessage(QObject* parent = NULL);
		~PreviewEmptyMessage();

	protected:
		virtual int codeImpl() override;
		virtual int levelImpl() override;
		virtual QString messageImpl() override;
		virtual QString linkStringImpl() override;
		virtual void handleImpl() override;

	};

};

#endif // CREATIVE_KERNEL_PREVIEW_EMPTY_MESSAGE_202312131652_H