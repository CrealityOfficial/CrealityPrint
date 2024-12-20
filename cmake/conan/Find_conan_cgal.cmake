__cc_find(boost_header)

if(CONAN_CGAL_ROOT_RELEASE)
	set(CGAL_INCLUDE_DIRS ${CONAN_CGAL_ROOT_RELEASE}/include/ ${CONAN_CGAL_ROOT_RELEASE}/include/CGAL/Root/)
	__add_include_interface(cgal INTERFACE ${CGAL_INCLUDE_DIRS})	
endif()
