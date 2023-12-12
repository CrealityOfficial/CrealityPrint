# cpython

if(NOT TARGET cpython)
	__use_python()
	
	set(cpython_INCLUDE_DIRS ${Python3_INCLUDE_DIRS})
	set(cpython_LIBRARIES_RELEASE ${Python3_LIBRARIES_RELEASE})
	set(cpython_LIBRARIES_DEBUG ${Python3_LIBRARIES_DEBUG})
	
	__import_target(cpython dll INTERFACE_DEF USE_CPYTHON)
endif()