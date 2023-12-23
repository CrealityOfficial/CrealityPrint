file(GLOB QRC_SRC res/*.qrc)
file(GLOB PLUGIN_SRC res/plugin/*.qrc)
set(QRCS ${QRC_SRC} ${PLUGIN_SRC} "../../resources/default.qrc")

__files_group(${CMAKE_CURRENT_SOURCE_DIR}/external/ EXTERNAL_SRCS)
__files_group_2(${CMAKE_CURRENT_SOURCE_DIR}/internal/ internal INTERNAL_SRCS)
set(SRCS ${EXTERNAL_SRCS} ${INTERNAL_SRCS})

set(LIBS Qt5::Concurrent Qt5::WebSockets )
set(ILIBS qtuser_3d cxkernel shader_entity)

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

list(APPEND LIBS rapidjson topomesh
		curl cpr cxgcode
		qcxffmpeg qcxchart
		)

if(CC_BC_WIN)
	list(APPEND LIBS dbghelp.lib iphlpapi ws2_32 crypt32)
elseif(CC_BC_LINUX)
	list(APPEND LIBS stdc++ stdc++fs zlib_static pthread)
endif()

set(INCS ${CMAKE_CURRENT_SOURCE_DIR}/external/
		 ${CMAKE_CURRENT_SOURCE_DIR}
		 )
set(INTERFACES ${CMAKE_CURRENT_SOURCE_DIR}/external/)
 
__add_common_library(BasicKernel QTQRC ${QRCS})
