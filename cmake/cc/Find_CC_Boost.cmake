# This sets the following variables:
# BOOST_INCLUDE_DIRS
# BOOST_INCLUDE_FOUND

if(NOT BOOST_INCLUDE_DIRS)
	if(BOOST_INSTALL_ROOT)
		message(STATUS "Boost Specified BOOST_INSTALL_ROOT : ${BOOST_INSTALL_ROOT}")
		set(BOOST_INCLUDE_DIRS ${BOOST_INSTALL_ROOT}/include/)
		
		set(boost_file_system_INCLUDE_ROOT ${BOOST_INSTALL_ROOT}/include/)
		set(boost_file_system_LIB_ROOT ${BOOST_INSTALL_ROOT}/lib/)
	
		set(boost_thread_INCLUDE_ROOT ${BOOST_INSTALL_ROOT}/include/)
		set(boost_thread_LIB_ROOT ${BOOST_INSTALL_ROOT}/lib/)
								
		set(boost_program_options_INCLUDE_ROOT ${BOOST_INSTALL_ROOT}/include/)
		set(boost_program_options_LIB_ROOT ${BOOST_INSTALL_ROOT}/lib/)
	
		set(boost_regex_INCLUDE_ROOT ${BOOST_INSTALL_ROOT}/include/)
		set(boost_regex_LIB_ROOT ${BOOST_INSTALL_ROOT}/lib/)
	
		set(boost_system_INCLUDE_ROOT ${BOOST_INSTALL_ROOT}/include/)
		set(boost_system_LIB_ROOT ${BOOST_INSTALL_ROOT}/lib/)
	
		set(boost_python_INCLUDE_ROOT ${BOOST_INSTALL_ROOT}/include/)
		set(boost_python_LIB_ROOT ${BOOST_INSTALL_ROOT}/lib/)
	
		set(boost_serialization_INCLUDE_ROOT ${BOOST_INSTALL_ROOT}/include/)
		set(boost_serialization_LIB_ROOT ${BOOST_INSTALL_ROOT}/lib/)
	
		set(boost_nowide_INCLUDE_ROOT ${BOOST_INSTALL_ROOT}/include/)
		set(boost_nowide_LIB_ROOT ${BOOST_INSTALL_ROOT}/lib/)
	
		set(boost_log_INCLUDE_ROOT ${BOOST_INSTALL_ROOT}/include/)
		set(boost_log_LIB_ROOT ${BOOST_INSTALL_ROOT}/lib/)
	
		set(boost_locale_INCLUDE_ROOT ${BOOST_INSTALL_ROOT}/include/)
		set(boost_locale_LIB_ROOT ${BOOST_INSTALL_ROOT}/lib/)		
		
		set(boost_iostreams_INCLUDE_ROOT ${BOOST_INSTALL_ROOT}/include/)
		set(boost_iostreams_LIB_ROOT ${BOOST_INSTALL_ROOT}/lib/)
	else()
		find_path(BOOST_INCLUDE_DIRS
				NAMES boost/filesystem.hpp
				HINTS "${BOOST_INSTALL_ROOT}"
				PATHS "$ENV{USR_INSTALL_ROOT}/include/boost" "$ENV{USR_INSTALL_ROOT}/include/"
					"/usr/include/" "/usr/include/boost/"
					"/usr/local/include/" "/usr/local/include/boost/"
				NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
				)
	endif()
endif()

if(NOT TARGET boost_iostreams)	
	__search_target_components(boost_iostreams
							INC boost/io_fwd.hpp
							DLIB boost_iostreams
							LIB boost_iostreams
							PRE boost
							)
	__test_import(boost_iostreams dll)
endif()

if(NOT TARGET boost_file_system)	
	__search_target_components(boost_file_system
							INC boost/filesystem.hpp
							DLIB boost_file_system
							LIB boost_file_system
							PRE boost
							)
	__test_import(boost_file_system dll)
endif()
	
if(NOT TARGET boost_serialization)	
	__search_target_components(boost_serialization
							INC boost/serialization/access.hpp
							DLIB boost_serialization
							LIB boost_serialization
							PRE boost
							) 
	__test_import(boost_serialization dll)
endif()
						   
if(NOT TARGET boost_python)
	__search_target_components(boost_python
							INC boost/python.hpp
							DLIB boost_python
							LIB boost_python
							PRE boost
							)
	__test_import(boost_python dll)
endif()
						   
if(NOT TARGET boost_system)
	__search_target_components(boost_system
							INC boost/system/error_code.hpp
							DLIB boost_system
							LIB boost_system
							PRE boost
							)	
	__test_import(boost_system dll)
endif()
						   
if(NOT TARGET boost_regex)
	__search_target_components(boost_regex
							INC boost/regex.hpp
							DLIB boost_regex
							LIB boost_regex
							PRE boost
							)
	__test_import(boost_regex dll)
endif()

if(NOT TARGET boost_program_options)
	__search_target_components(boost_program_options
							INC boost/program_options.hpp
							DLIB boost_program_options
							LIB boost_program_options
							PRE boost
							)
	__test_import(boost_program_options dll)
endif()

if(NOT TARGET boost_thread)						   
	__search_target_components(boost_thread
							INC boost/thread.hpp
							DLIB boost_thread
							LIB boost_thread
							PRE boost
							)
	__test_import(boost_thread dll)
endif()

if(NOT TARGET boost_nowide)						   
	__search_target_components(boost_nowide
							INC boost/thread.hpp
							DLIB boost_nowide
							LIB boost_nowide
							PRE boost
							)
	__test_import(boost_nowide dll)
endif()
	
if(NOT TARGET boost_log)	
	__search_target_components(boost_log
							INC boost/thread.hpp
							DLIB boost_log
							LIB boost_log
							PRE boost
							)
	__test_import(boost_log dll)
endif()
	
if(NOT TARGET boost_locale)	
	__search_target_components(boost_locale
							INC boost/thread.hpp
							DLIB boost_locale
							LIB boost_locale
							PRE boost
							)
	__test_import(boost_locale dll)
endif()
						   
add_definitions(-DBOOST_DYN_LINK)
add_definitions(-DBOOST_ALL_NO_LIB)

if(BOOST_INCLUDE_DIRS)
	set(BOOST_INCLUDE_FOUND 1)
	set(Boost_INCLUDE_DIRS ${BOOST_INCLUDE_DIRS})
	message(STATUS "BOOST_INCLUDE_DIRS : ${BOOST_INCLUDE_DIRS}")
else()
	message(STATUS "Find Boost include Failed")
endif()