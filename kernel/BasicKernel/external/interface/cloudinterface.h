#pragma once

#ifndef EXTERNAL_INTERFACE_CLOUDINTERFACE_H_
#define EXTERNAL_INTERFACE_CLOUDINTERFACE_H_

#include <QtCore/QPointer>

#include <cxcloud/define.h>
#include <cxcloud/service_center.h>

#include "external/basickernelexport.h"

namespace creative_kernel {

BASIC_KERNEL_API QPointer<cxcloud::ServiceCenter> CloudService();
BASIC_KERNEL_API QString GetCloudUrl();
BASIC_KERNEL_API QString LoadCloudModelGroupUrl(const QString& group_id);

BASIC_KERNEL_API std::weak_ptr<cxcloud::AccountService> CloudAccountService();
BASIC_KERNEL_API bool IsCloudLogined();
BASIC_KERNEL_API const cxcloud::UserBaseInfo& CloudUserBaseInfo();
BASIC_KERNEL_API const cxcloud::UserDetailInfo& CloudUserDetailInfo();

}  // namespace creative_kernel

#endif  // EXTERNAL_INTERFACE_CLOUDINTERFACE_H_
