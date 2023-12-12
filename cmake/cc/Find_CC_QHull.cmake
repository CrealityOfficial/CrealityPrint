# This sets the following variables:
# qhullcpp_INCLUDE_DIRS
# qhullcpp_LIBRARIES_DEBUG qhullcpp_LIBRARIES_RELEASE
# qhullcpp target
# qhullstatic_r_INCLUDE_DIRS
# qhullstatic_r_LIBRARIES_DEBUG qhullstatic_r_LIBRARIES_RELEASE
# qhullstatic_r target

if(THIRD0_INSTALL_ROOT)
elseif(CXQHULL_INSTALL_ROOT)
	set(qhullcpp_INCLUDE_ROOT ${CXQHULL_INSTALL_ROOT}/include/qhull/)
	set(qhullcpp_LIB_ROOT ${CXQHULL_INSTALL_ROOT}/lib/)

							   
	set(qhullstatic_r_INCLUDE_ROOT ${CXQHULL_INSTALL_ROOT}/include/qhull/)
	set(qhullstatic_r_LIB_ROOT ${CXQHULL_INSTALL_ROOT}/lib/)
endif()

__search_target_components(qhullcpp
						   INC libqhullcpp/Qhull.h
						   DLIB qhullcpp
						   LIB qhullcpp
						   PRE qhull
						   )

__search_target_components(qhullstatic_r
						   INC libqhullcpp/Qhull.h
						   DLIB qhullstatic_r
						   LIB qhullstatic_r
						   PRE qhull
						   )

__test_import(qhullcpp lib)
__test_import(qhullstatic_r lib)
set(qhull qhullstatic_r qhullcpp)