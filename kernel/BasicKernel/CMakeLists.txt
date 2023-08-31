file(GLOB QRC_SRC res/*.qrc)
file(GLOB PLUGIN_SRC res/plugin/*.qrc)
qt5_add_resources(RCC_SRC ${QRC_SRC} ${PLUGIN_SRC}
						  "../../resources/default.qrc"
						  )

__cc_find(RapidJson)
__cc_find(thumbnail)

__files_group(${CMAKE_CURRENT_SOURCE_DIR}/external/ EXTERNAL_SRCS)
__files_group_2(${CMAKE_CURRENT_SOURCE_DIR}/internal/ internal INTERNAL_SRCS)
set(LIBS qhullWrapper cmesh fmesh quazip zlib libqrencode libnest2d clipper3r nestplacer 
		nlopt clipper splitslot OpenMP::OpenMP_CXX AliyunOss ossWrapper sensors_analytics_sdk
		curl cxmdns cpr imageproc rapidjson cxbin
		crcommon crslice Qt5::SerialPort Qt5::Concurrent
		cserialport ssl crypto Qt5::WebSockets thumbnail cxgcode
		qcxffmpeg qcxchart CADCore
		)

set(SRCS ${EXTERNAL_SRCS} ${INTERNAL_SRCS} ${RCC_SRC})
		
if(CC_BC_WIN)
	list(APPEND LIBS dbghelp.lib)
endif()

if( WIN32 AND NOT CYGWIN )
	set(LIBS ${LIBS} ws2_32 crypt32)
endif()

set(ILIBS ${QtQmlQ3dLibs} ${qtuser} qcxutil trimesh2 cxnd mmesh boost_system stringutil cxkernel cxutil)

set(DEFS BASIC_KERNEL_DLL NOMINMAX BOOST_ALL_NO_LIB)

set(INCS ${CMAKE_CURRENT_SOURCE_DIR}/external/)
 
__add_real_target(BasicKernel dll 	SOURCE ${SRCS} 
									LIB ${LIBS}
									ILIB ${ILIBS}
									DEF ${DEFS}
									INC ${INCS}
									INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/external/
									)
if(CC_BC_LINUX)
	target_link_libraries(BasicKernel PRIVATE stdc++ stdc++fs curl zlib pthread)
endif()
