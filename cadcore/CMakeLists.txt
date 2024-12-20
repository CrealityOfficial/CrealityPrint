__cc_find(opencascade_static)
__cc_find(trimesh2)

__assert_target(trimesh2)
__assert_target(TKernel)

__files_group(${CMAKE_CURRENT_SOURCE_DIR}/cadcore/ SRCS)	 
set(LIBS TKernel
		 TKMath
		 TKG2d
		 TKG3d
		 TKGeomBase
		 TKBRep
		 TKGeomAlgo
		 TKTopAlgo
		 TKPrim
		 TKBO
		 TKShHealing
		 TKBool
		 TKHLR
		 TKMesh
		 TKService
		 TKV3d
		 TKCDF
		 TKLCAF
		 TKCAF
		 TKBinL
		 TKXmlL
		 TKBin
		 TKXml
		 TKVCAF
		 TKXSBase
		 TKSTEPBase
		 TKSTEPAttr
		 TKSTEP209
		 TKSTEP
		 TKIGES
		 TKXCAF
		 TKXDE
		 TKXDEIGES
		 TKXDESTEP
		 TKSTL
		 TKVRML
		 TKRWMesh
		 TKXmlXCAF
		 TKBinXCAF     
		)
list(REVERSE LIBS)  # for linux build

if(CC_BC_WIN)
else()
    find_package (Threads)
    list(APPEND LIBS ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})
endif()

set(ILIBS trimesh2)

__add_common_library(cadcore)

