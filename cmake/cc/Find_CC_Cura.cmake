# This sets the following variables:
# cura_INCLUDE_DIRS
# cura_LIBRARIES_DEBUG cura_LIBRARIES_RELEASE
# cura target

if(CXCURA_INSTALL_ROOT)
	set(cura_INCLUDE_ROOT ${CXCURA_INSTALL_ROOT}/include/cura/)
	set(cura_LIB_ROOT ${CXCURA_INSTALL_ROOT}/lib/)
endif()

__search_target_components(cura
						   INC FffGcodeWriter.h
						   DLIB cura
						   LIB cura
						   PRE cura
						   )

__test_import(cura lib)