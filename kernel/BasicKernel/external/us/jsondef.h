#ifndef _US_JSONDEF_1589439749372_H
#define _US_JSONDEF_1589439749372_H
#include "basickernelexport.h"
#include "us/settinggroupdef.h"
#include <QtCore/QMap>

namespace us
{
	class BASIC_KERNEL_API JsonDef: public QObject
	{
	public:
		JsonDef(QObject* parent = nullptr);
		virtual ~JsonDef();

		void parse(const QString& file, QMap<QString, SettingGroupDef*>& defs, QObject* parent);
	};
}
#endif // _US_JSONDEF_1589439749372_H
