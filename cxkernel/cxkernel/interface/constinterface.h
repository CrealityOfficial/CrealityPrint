/**
 * @file constinterface.h
 * @author zenggui (anoob@sina.cn)
 * @brief 
 * @version 0.1
 * @date 2023-06-07
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef XCKERNEL_APP_SETTING_INTERFACE_1593676766939_H
#define XCKERNEL_APP_SETTING_INTERFACE_1593676766939_H
#include "cxkernel/cxkernelinterface.h"
#include <QColor>
#include <QString>

namespace cxkernel
{
	CXKERNEL_API QString version();
	CXKERNEL_API bool isReleaseVersion();

	CXKERNEL_API QString writableLocation(const QString& subDir, const QString& subSubDir = QString());
}

#endif // XCKERNEL_APP_SETTING_INTERFACE_1593676766939_H