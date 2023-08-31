#include "previewbase.h"

#include "internal/menu/submenurecentfiles.h"
#include <QtCore/QDebug>

namespace creative_kernel
{
	PreviewBase::PreviewBase(QObject* parent)
		: Job(parent)
		, m_attain(nullptr)
	{
	}

	PreviewBase::~PreviewBase()
	{

	}

	void PreviewBase::setFileName(const QString& fileName)
	{
		m_fileName = fileName;
	}

	QString PreviewBase::name()
	{
		return "Preview";
	}

	QString PreviewBase::description()
	{
		return "";
	}

	void PreviewBase::failed()
	{
		qDebug() << "preview failed.";
		emit sliceFailed();
	}

	void PreviewBase::successed(qtuser_core::Progressor* progressor)
	{
		SubMenuRecentFiles::getInstance()->setMostRecentFile(m_fileName);

		qDebug() << "success";
		SliceAttain* attain = take();

		assert(attain);
		if (attain)
			emit sliceSuccess(attain);
	}

	SliceAttain* PreviewBase::take()
	{
		SliceAttain* attain = m_attain;
		m_attain = nullptr;
		return attain;
	}
}