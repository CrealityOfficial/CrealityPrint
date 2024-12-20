#ifndef CREATIVE_KERNEL_SLICE_ENGINE_FAIL_MESSAGE_202402281024_H
#define CREATIVE_KERNEL_SLICE_ENGINE_FAIL_MESSAGE_202402281024_H

#include <QObject>
#include "basickernelexport.h"
#include "external/message/messageobject.h"

namespace creative_kernel
{
	class SliceEngineFailMessage : public MessageObject
	{
		Q_OBJECT

	public:
		SliceEngineFailMessage(const QString& failReason, QObject* parent = NULL);
		~SliceEngineFailMessage();

		void setLevel(int level);
		int64_t getRelatedSceneObjectId();
	protected:
		virtual int codeImpl() override;
		virtual int levelImpl() override;
		virtual QString messageImpl() override;
		virtual QString linkStringImpl() override;
		virtual void handleImpl() override;

	private:
		int m_level;
		QString m_failReasonDesc;
		int64_t m_sceneObjectId;
	};

	class Crslice2InfoMessage : public MessageObject
	{
		Q_OBJECT
	public:
		Crslice2InfoMessage(const QString& info, QObject* parent = NULL);
		~Crslice2InfoMessage();

		void setLevel(int level);
	protected:
		virtual int codeImpl() override;
		virtual int levelImpl() override;
		virtual QString messageImpl() override;
		virtual QString linkStringImpl() override;
		virtual void handleImpl() override;
	private:
		int m_level;
		QString m_info;
	};
};


#endif // !CREATIVE_KERNEL_SLICE_ENGINE_FAIL_MESSAGE_202402281024_H