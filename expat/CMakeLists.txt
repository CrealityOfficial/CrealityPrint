
__add_real_target(expat lib SOURCE xmlparse.c xmlrole.c xmltok.c
							INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
							)
							
if(CC_INSTALL_EXPAT)
	__install_directory_specif(${CMAKE_CURRENT_SOURCE_DIR}/ expat)
endif()