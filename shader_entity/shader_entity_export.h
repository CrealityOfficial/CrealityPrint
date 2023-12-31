#ifndef _NULLSPACE_SHADER_ENTITY_EXPORT_202309201707_H
#define _NULLSPACE_SHADER_ENTITY_EXPORT_202309201707_H
#include "ccglobal/qexport.h"

#if USE_SHADER_ENTITY_DLL
    #define SHADER_ENTITY_API CC_DECLARE_IMPORT
#elif USE_SHADER_ENTITY_STATIC
    #define SHADER_ENTITY_API CC_DECLARE_STATIC
#else
    #if SHADER_ENTITY_DLL
        #define SHADER_ENTITY_API CC_DECLARE_EXPORT
    #else
        #define SHADER_ENTITY_API CC_DECLARE_IMPORT
    #endif
#endif

#define SHADERENTITY_API SHADER_ENTITY_API
#endif // _NULLSPACE_SHADER_ENTITY_EXPORT_202309201707_H
