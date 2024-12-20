#ifndef CREATIVE_KERNEL_ADAPTIVE_LAYER_MESSAGE_202403121900_H
#define CREATIVE_KERNEL_ADAPTIVE_LAYER_MESSAGE_202403121900_H

#include <QObject>
#include "basickernelexport.h"
#include "external/message/messageobject.h"

namespace creative_kernel
{
	class AdaptiveLayerMessage : public MessageObject
	{
		Q_OBJECT

	public:
		AdaptiveLayerMessage(bool treeSupport = false,QObject* parent = NULL);
		~AdaptiveLayerMessage();
	protected:
		virtual int codeImpl() override;
		virtual int levelImpl() override;
		virtual QString messageImpl() override;
		virtual QString linkStringImpl() override;
		virtual void handleImpl() override;
	private : 
		bool m_treesupport;
	};

};


#endif // !CREATIVE_KERNEL_ADAPTIVE_LAYER_MESSAGE_202403121900_H