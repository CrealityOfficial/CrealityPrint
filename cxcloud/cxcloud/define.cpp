#include "cxcloud/define.h"

#include <QtCore/QLocale>

#include "cxcloud/tool/function.h"

namespace cxcloud {

  const ServerType DEFAULT_SERVER_TYPE = QLocale{}.country() == QLocale::Country::China
                                             ? ServerType::MAINLAND_CHINA
                                             : ServerType::OTHERS;

  const RealServerType DEFAULT_REAL_SERVER_TYPE = ServerTypeToRealServerType(DEFAULT_SERVER_TYPE);

  const ModelRestriction DEFAULT_MODEL_RESTRICTION = ModelRestriction::BLUR;

}  // namespace cxcloud
