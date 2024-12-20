#include "cxhandlerbase.h"

namespace qtuser_core
{
	CXHandleBase::CXHandleBase()
	{
	}

	CXHandleBase::~CXHandleBase()
	{

	}

	QString CXHandleBase::filter()
	{
		return QString("CXHandleBase ()");
	}

	void CXHandleBase::cancelHandle()
	{
	
	}

	void CXHandleBase::handle(const QString& fileName)
	{
	}

	void CXHandleBase::handle(const QStringList& fileNames)
	{
		if (fileNames.size() == 1)
			handle(fileNames.at(0));
	}

	QString CXHandleBase::filterKey()
	{
		return "base";
	}

	QStringList CXHandleBase::suffixesFromFilter()
	{
		QStringList suffixes;
		QString line = filter();
		int i1 = line.indexOf("(");
		int i2 = line.lastIndexOf(")");
		if (i1 >= 0 && i2 >= 0)
		{
			QString line1 = line.mid(i1 + 1, i2 - i1 - 1);
			QStringList lines = line1.split(" ");
			for (const QString& l : lines)
			{
				int i = l.lastIndexOf(".");
				if (i >= 0)
				{
					QString suf = l.mid(i + 1);
					suffixes.append(suf);
				}
			}
		}
		return suffixes;
	}
}