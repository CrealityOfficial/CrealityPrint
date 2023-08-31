#pragma once

#ifndef EXTERNAL_INTERFACE_CLOUDINTERFACE_H_
#define EXTERNAL_INTERFACE_CLOUDINTERFACE_H_
#include "basickernelexport.h"
#include <QtCore/QPointer>

#if USE_CXCLOUD
#include <cxcloud/define.hpp>
#include <cxcloud/service_center.h>
#endif


namespace creative_kernel {

#if USE_CXCLOUD
BASIC_KERNEL_API QPointer<cxcloud::ServiceCenter> CloudService();
BASIC_KERNEL_API std::weak_ptr<cxcloud::AccountService> CloudAccountService();
BASIC_KERNEL_API const cxcloud::UserBaseInfo& CloudUserBaseInfo();
BASIC_KERNEL_API const cxcloud::UserDetailInfo& CloudUserDetailInfo();
#endif

BASIC_KERNEL_API QString GetCloudUrl();
BASIC_KERNEL_API QString LoadCloudModelGroupUrl(const QString& group_id);
BASIC_KERNEL_API bool IsCloudLogined();

}  // namespace creative_kernel

#endif  // EXTERNAL_INTERFACE_CLOUDINTERFACE_H_
