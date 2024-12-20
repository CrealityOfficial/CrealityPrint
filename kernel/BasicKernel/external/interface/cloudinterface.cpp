#include "external/interface/cloudinterface.h"

#include <cxcloud/service_center.h>

#include "external/kernel/kernel.h"

namespace creative_kernel {

QPointer<cxcloud::ServiceCenter> CloudService() {
	return getKernel()->cxcloud();
}

QString GetCloudUrl() {
  return CloudService()->getWebUrl();
}

QString LoadCloudModelGroupUrl(const QString& group_id) {
  return CloudService()->loadModelGroupUrl(group_id);
}

std::weak_ptr<cxcloud::AccountService> CloudAccountService() {
  return CloudService()->getAccountService();
}

bool IsCloudLogined() {
  return CloudService()->getAccountService().lock()->isLogined();
}

const cxcloud::UserBaseInfo& CloudUserBaseInfo() {
  return CloudService()->getAccountService().lock()->userBaseInfo();
}

const cxcloud::UserDetailInfo& CloudUserDetailInfo() {
  return CloudService()->getAccountService().lock()->userDetailInfo();
}

}  // namespace creative_kernel
