#pragma once

#ifndef CXCLOUD_INTERFACE_HPP_
#define CXCLOUD_INTERFACE_HPP_

#include <ccglobal/export.h>

#if USE_CXCLOUD_DLL
#  define CXCLOUD_API CC_DECLARE_IMPORT
#elif USE_CXCLOUD_STATIC
#  define CXCLOUD_API CC_DECLARE_STATIC
#else
#  if CXCLOUD_DLL
#    define CXCLOUD_API CC_DECLARE_EXPORT
#  else
#    define CXCLOUD_API CC_DECLARE_STATIC
#  endif
#endif

#endif // CXCLOUD_INTERFACE_HPP_
