#ifndef CREATIVE_KERNEL_SUPPORT_GENERATOR_MESSAGE_202312131652_H
#define CREATIVE_KERNEL_SUPPORT_GENERATOR_MESSAGE_202312131652_H

#include <QObject>
#include "basickernelexport.h"
#include "external/message/messageobject.h"

namespace creative_kernel
{
    class ModelN;
	class SupportGeneratorMessage : public MessageObject
	{
		Q_OBJECT

	public:
		SupportGeneratorMessage(ModelN* supportGenerator, QObject* parent = NULL);
		~SupportGeneratorMessage();

	protected:
		virtual int codeImpl() override;
		virtual int levelImpl() override;
		virtual QString messageImpl() override;
		virtual QString linkStringImpl() override;
		virtual void handleImpl() override;

	private:
		ModelN* m_supportGenerator { NULL };

	};

};

#endif // CREATIVE_KERNEL_STAY_ON_BORDER_MESSAGE_202312131652_H