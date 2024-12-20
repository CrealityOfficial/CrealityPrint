# This sets the following variables:
# openvdb target

if(OPENVDB_INSTALL_ROOT)
	message(STATUS "Specified OPENVDB_INSTALL_ROOT : ${OPENVDB_INSTALL_ROOT}")
	set(openvdb_INCLUDE_ROOT ${OPENVDB_INSTALL_ROOT}/include/)
	set(openvdb_LIB_ROOT ${OPENVDB_INSTALL_ROOT}/lib/)
	__search_target_components(openvdb
							   INC openvdb/Exceptions.h
							   DLIB openvdb
							   LIB openvdb
							   )
else()
endif()

__test_import(openvdb dll)