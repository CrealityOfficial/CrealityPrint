#ifndef SLICE_WARNING_MESSAGE_202405281433_H
#define SLICE_WARNING_MESSAGE_202405281433_H

#include <QObject>
#include <QtCore/QMap>
#include <QtCore/QPair>

#include "basickernelexport.h"
#include "external/message/messageobject.h"

namespace creative_kernel
{
	class SliceWarningMessage : public MessageObject
	{
		Q_OBJECT

	public:
		SliceWarningMessage(const QMap<QString, QPair<QString, int64_t> >& warningDetails, QObject* parent = NULL);
		~SliceWarningMessage();

		int64_t getRelatedSceneObjectId();

	protected:
		virtual int codeImpl() override;
		virtual int levelImpl() override;
		virtual QString messageImpl() override;
		virtual QString linkStringImpl() override;
		virtual void handleImpl() override;

		QString pathConflictImpl(const QString& msg);
		QString sliceNeedSupportImpl(const QString& msg);

	private:
		QMap<QString, QPair<QString, int64_t> > m_warningDetails;

		int64_t m_sceneObjId;

		bool m_needJumpTo;
	};

};

#endif // SLICE_WARNING_MESSAGE_202405281433_H