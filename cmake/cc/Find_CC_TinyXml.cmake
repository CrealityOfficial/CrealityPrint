# This sets the following variables:
# tinyxml target

if(NOT TARGET tinyxml)
	if(THIRD1_INSTALL_ROOT)
		message(STATUS "Specified THIRD1_INSTALL_ROOT : ${THIRD1_INSTALL_ROOT}")
		set(trimesh2_INCLUDE_ROOT ${THIRD1_INSTALL_ROOT}/include/)
		set(trimesh2_LIB_ROOT ${THIRD1_INSTALL_ROOT}/lib/)
	elseif(CXTINYXML_INSTALL_ROOT)
		message(STATUS "Specified CXTINYXML_INSTALL_ROOT : ${CXTINYXML_INSTALL_ROOT}")
		set(tinyxml_INCLUDE_ROOT ${CXTINYXML_INSTALL_ROOT}/include/)
		set(tinyxml_LIB_ROOT ${CXTINYXML_INSTALL_ROOT}/lib/)
	endif()
	
	__search_target_components(tinyxml
							INC tinyxml/tinyxml.h
							DLIB tinyxml
							LIB tinyxml
							PRE tinyxml
							)
	
	__test_import(tinyxml lib)
endif()