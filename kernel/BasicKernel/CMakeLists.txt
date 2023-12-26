file(GLOB QRC_SRC res/*.qrc)
file(GLOB PLUGIN_SRC res/plugin/*.qrc)
set(QRCS ${QRC_SRC} ${PLUGIN_SRC} "../../resources/default.qrc")

__files_group(${CMAKE_CURRENT_SOURCE_DIR}/external/ EXTERNAL_SRCS)
__files_group_2(${CMAKE_CURRENT_SOURCE_DIR}/internal/ internal INTERNAL_SRCS)

set(PAINT_OPERATE_MODE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/external/operation/paintoperatemode)
__files_group(${PAINT_OPERATE_MODE_PATH} PAINT_OPERATE_MODE_SRCS)

set(SRCS ${EXTERNAL_SRCS} ${INTERNAL_SRCS} ${PAINT_OPERATE_MODE_SRCS})

set(LIBS Qt5::Concurrent Qt5::WebSockets )

__cc_find(boost_static)
__assert_target(boost_nowide)
__assert_target(boost_filesystem)

set(ILIBS qtuser_3d cxkernel shader_entity boost_nowide boost_filesystem)

__cc_find(cadcore)
__assert_target(cadcore)
list(APPEND LIBS cadcore)

__assert_target(crslice)
list(APPEND LIBS crslice)

__cc_find(boost_header)
__assert_target(boost_header)
list(APPEND ILIBS boost_header)

__cc_find(topomesh)
__assert_target(topomesh)

__cc_find(spread)
__assert_target(spread)
__cc_find(expat)
__assert_target(expat)

list(APPEND LIBS rapidjson
		cpr_static expat cxgcode
		qcxffmpeg qcxchart spread topomesh
		)

if(CC_BC_WIN)
	list(APPEND LIBS dbghelp.lib iphlpapi ws2_32 crypt32)
elseif(CC_BC_LINUX)
	list(APPEND LIBS stdc++ stdc++fs zlib pthread)
endif()

set(INCS ${CMAKE_CURRENT_SOURCE_DIR}/external/
		 ${CMAKE_CURRENT_SOURCE_DIR}
		 )
set(INTERFACES ${CMAKE_CURRENT_SOURCE_DIR}/external/)
 
__add_common_library(BasicKernel QTQRC ${QRCS})
