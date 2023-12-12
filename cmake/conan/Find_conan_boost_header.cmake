# boost_header target
if(CONAN_BOOST_ROOT_RELEASE)
	set(CONAN_INCLUDE_DIRS_BOOST_HEADER_RELEASE ${CONAN_BOOST_ROOT_RELEASE}/include/)	
	__conan_set_include(boost_header INTERFACE_DEF BOOST_ALL_NO_LIB)
endif()
