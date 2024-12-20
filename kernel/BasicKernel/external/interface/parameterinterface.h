#pragma once

#ifndef EXTERNAL_INTERFACE_PARAMETERINTERFACE_H_
#define EXTERNAL_INTERFACE_PARAMETERINTERFACE_H_

#include <set>

#include "basickernelexport.h"

namespace creative_kernel {

  BASIC_KERNEL_API
  auto GetProcessSupportParameterKeys() -> std::set<QString>;

  QString getExtruderValue(const QString& key, const QString& defaultValue);
  QString getPrintMachineValue(const QString& key, const QString& defaultValue);

  QString getPrintProfileValue(const QString& key, const QString& defaultValue);
  void setPrintProfileValue(const QString& key, const QString& value);

}  // namespace creative_kernel

#endif  // EXTERNAL_INTERFACE_PARAMETERINTERFACE_H_
