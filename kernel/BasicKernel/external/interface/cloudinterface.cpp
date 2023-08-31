#include "interface/cloudinterface.h"
#include "kernel/kernel.h"

#if USE_CXCLOUD
#include <cxcloud/service_center.h>
#endif

namespace creative_kernel {

#if USE_CXCLOUD
QPointer<cxcloud::ServiceCenter> CloudService() {
	return getKernel()->cxcloud();
}


const cxcloud::UserBaseInfo& CloudUserBaseInfo() {
	return CloudService()->getAccountService().lock()->userBaseInfo();
}

const cxcloud::UserDetailInfo& CloudUserDetailInfo() {
	return CloudService()->getAccountService().lock()->userDetailInfo();
}

std::weak_ptr<cxcloud::AccountService> CloudAccountService() {
	return CloudService()->getAccountService();
}
#endif

QString GetCloudUrl() {
#if USE_CXCLOUD
  return CloudService()->getUrl();
#else
	return QString();
#endif
}

QString LoadCloudModelGroupUrl(const QString& group_id) {
#if USE_CXCLOUD
  return CloudService()->loadModelGroupUrl(group_id);
#else
	return QString();
#endif
}

bool IsCloudLogined() {
#if USE_CXCLOUD
  return CloudService()->getAccountService().lock()->isLogined();
#else
	return false;
#endif
}


}  // namespace creative_kernel

