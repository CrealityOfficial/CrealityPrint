# This sets the following variables:
# OCC target
# OCC_INCLUDE_DIRS

set(OCC_LIBS TKVCAF
			 TKVrml
			 TKSTL
			 TKBRep
			 TKIGES
			 TKShHealing
			 TKSTEP
			 TKXSBase
			 TKBool
			 TKCAF
			 TKCDF
			 TKernel
			 TKFeat
			 TKFillet
			 TKG2d
			 TKG3d
			 TKGeomAlgo
			 TKGeomBase
			 TKHLR
			 TKMath
			 TKMesh
			 TKOffset
			 TKPrim
			 TKService
			 TKTopAlgo
			 TKV3d
			 TKOpenGl
			 TKLCAF
			 TKBO
			 TKSTEPBase
			 TKSTEPAttr
			 TKSTEP209
			 TKBin
			 TKBinL
			 TKXCAF
			 TKXDESTEP
			 TKXDEIGES
			 TKMeshVS
			 )
			 
if(OCC_INSTALL_ROOT)
	message(STATUS "Specified OCC_INSTALL_ROOT : ${OCC_INSTALL_ROOT}")
	
	foreach(item ${OCC_LIBS})
		set(${item}_INCLUDE_ROOT ${OCC_INSTALL_ROOT}/include/occ/)
		set(${item}_LIB_ROOT ${OCC_INSTALL_ROOT}/lib/)
		__search_target_components(${item}
								INC AIS.hxx
								DLIB ${item}
								LIB ${item}
								)
	endforeach()
	
	set(OCC_INCLUDE_DIRS ${OCC_INSTALL_ROOT}/include/occ/)
else()
	foreach(item ${OCC_LIBS})
		__search_target_components(${item}
								INC AIS.hxx
								DLIB ${item}
								LIB ${item}
								PRE occ
								)
	endforeach()
	
	if(TARGET TKernel)
		set(OCC_INCLUDE_DIRS ${TKernel_INCLUDE_DIRS})
	endif()
endif()

foreach(item ${OCC_LIBS})
	__test_import(${item} dll)
endforeach()
message(STATUS "OpenCascade directory :${OCC_INCLUDE_DIRS}")

set(OCC_BASE_LIBS TKernel TKMath)
set(OCC_IMPORT_LIBS ${OCC_BASE_LIBS} TKIGES TKXSBase)