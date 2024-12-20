#include "qtusercore/string/urlutils.h"

namespace qtuser_core
{
	QString QMLUrl2FileString(const QUrl& url)
	{
		if (url.toString().mid(0, 8) == "file:///")
		{
			return url.toString().mid(8);
		}
		return url.toString();
	}

}
