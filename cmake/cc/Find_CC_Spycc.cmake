# This sets the following variables:
# spycc target

if(ANALYSIS_INSTALL_ROOT)
	message(STATUS "Specified ANALYSIS_INSTALL_ROOT : ${ANALYSIS_INSTALL_ROOT}")
	set(spycc_INCLUDE_ROOT ${ANALYSIS_INSTALL_ROOT}/include/spycc/)
	set(spycc_LIB_ROOT ${ANALYSIS_INSTALL_ROOT}/lib/)
	__search_target_components(spycc
							   INC spycc/spy.h
							   DLIB spycc
							   LIB spycc
							   )
else()
endif()

__test_import(spycc lib)