#ifndef QTUSER_CORE_CXHANDLERBASE_H
#define QTUSER_CORE_CXHANDLERBASE_H
#include "qtusercore/qtusercoreexport.h"
#include <QtCore/QString>

namespace qtuser_core
{
	class QTUSER_CORE_API CXHandleBase
	{
	public:
		CXHandleBase();
		virtual ~CXHandleBase();

	public:
		virtual QString filter();     //format "XXXX (*.sufix1 *.sufix2 *.sufix3)"
		virtual void cancelHandle();
		virtual void handle(const QString& fileName);
		virtual void handle(const QStringList& fileNames);
		virtual QString filterKey();  // for manager  , "base" for default

		QStringList suffixesFromFilter();
	private:
	};
}

#endif // QTUSER_CORE_CXHANDLERBASE_H


