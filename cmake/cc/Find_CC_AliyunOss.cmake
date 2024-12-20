# This sets the following variables:
# AliyunOss_INCLUDE_DIRS
# AliyunOss_LIBRARIES_DEBUG AliyunOss_LIBRARIES_RELEASE
# AliyunOss target

if(THIRD0_INSTALL_ROOT)
	message(STATUS "Specified THIRD0_INSTALL_ROOT : ${THIRD0_INSTALL_ROOT}")
	set(zlib_INCLUDE_ROOT ${THIRD0_INSTALL_ROOT}/include/zlib/)
	set(zlib_LIB_ROOT ${THIRD0_INSTALL_ROOT}/lib/)
else(CXALIYUNOSS_INSTALL_ROOT)
	set(AliyunOss_INCLUDE_ROOT ${CXALIYUNOSS_INSTALL_ROOT}/include/oss/)
	set(AliyunOss_LIB_ROOT ${CXALIYUNOSS_INSTALL_ROOT}/lib/)
endif()

__search_target_components(AliyunOss
						   INC alibabacloud/oss/Const.h
						   DLIB AliyunOss
						   LIB AliyunOss
						   PRE oss
						   )

__test_import(AliyunOss dll)
add_library(aliyunoss ALIAS AliyunOss)