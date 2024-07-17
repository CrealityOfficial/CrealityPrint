file(GLOB QRC_SRC res/*.qrc)
file(GLOB PLUGIN_SRC res/plugin/*.qrc)
set(QRCS ${QRC_SRC} ${PLUGIN_SRC})

__files_group(${CMAKE_CURRENT_SOURCE_DIR}/external/ EXTERNAL_SRCS)
__files_group_2(${CMAKE_CURRENT_SOURCE_DIR}/internal/ internal INTERNAL_SRCS)

set(PAINT_OPERATE_MODE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/external/operation/paintoperatemode)
__files_group(${PAINT_OPERATE_MODE_PATH} PAINT_OPERATE_MODE_SRCS)

set(SRCS ${EXTERNAL_SRCS} ${INTERNAL_SRCS} ${PAINT_OPERATE_MODE_SRCS})

set(LIBS Qt5::Concurrent Qt5::WebSockets Qt5::Xml)
__cc_find(curl_static)
__cc_find(boost_static)
__assert_target(boost_nowide)
__assert_target(boost_filesystem)

set(ILIBS qtuser_3d cxkernel shader_entity boost_nowide boost_filesystem)

__cc_find(cadcore)
__assert_target(cadcore)
list(APPEND LIBS cadcore)

__assert_target(crslice)
list(APPEND LIBS crslice)
__assert_target(crslice2)
list(APPEND LIBS crslice2)

__cc_find(boost_header)
__assert_target(boost_header)
list(APPEND ILIBS boost_header)

__cc_find(topomesh)
__assert_target(topomesh)

__cc_find(spread)
__assert_target(spread)

__cc_find(expat)
__assert_target(expat)

list(APPEND LIBS rapidjson quazip_static
		cpr_static expat cxgcode curl_static
		qcxffmpeg qcxchart spread topomesh nestplacer common_3mf
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

# 模块内测试代码
if(BASICKERNEL_USE_UTEST)
	file(GLOB_RECURSE TEST_FILES test/*.cpp)
	__cc_find(gtest)
	__assert_target(gtest)
	set(GTESTLIB gtest)
	__add_real_target(BasicKernelSelfTest exe SOURCE  ${TEST_FILES}
											LIB BasicKernel ${GTESTLIB})

endif()
