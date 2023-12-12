#ifndef CXKERNEL_UTILS_1692424902291_H
#define CXKERNEL_UTILS_1692424902291_H
#include "cxkernel/cxkernelinterface.h"
#include "cxkernel/data/header.h"

namespace cxkernel
{
	CXKERNEL_API void circleDirectory(const QString& directory, circleLoadFunc func);
	CXKERNEL_API void ansycBatch(const QString& directory, circleLoadFunc func);

	CXKERNEL_API void runAnonymous(anonymous_work_func workFunc, anonymous_func successFunc);
	CXKERNEL_API std::string qString2String(const QString& str);
	CXKERNEL_API void clearModelSerializeData();
	CXKERNEL_API QString getSerializeModelName(QString model_name);

}

#endif // CXKERNEL_UTILS_1692424902291_H