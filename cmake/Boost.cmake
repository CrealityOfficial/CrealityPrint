macro(__add_boost_target module)
	set(btarget boost_${module})
	string(TOUPPER ${btarget} UpperName)
	
	if(${UpperName}_SOURCE)
		set(SRC ${${UpperName}_SOURCE})
	else()
		__source_recurse(${CMAKE_CURRENT_SOURCE_DIR} SRC)
	endif()
	__source_recurse(${boost_includes}boost/${module}/ HEADER)
			
	set(INCS ${CMAKE_CURRENT_SOURCE_DIR}/../../)
	set(INTERFACES ${CMAKE_CURRENT_SOURCE_DIR}/../../)
	set(SRCS ${SRC} ${HEADER})
	if(CC_GLOBAL_FORCE_STATIC OR ${UpperName}_STATIC)
		__add_real_target(${btarget} lib SOURCE ${SRCS} 
										LIB ${LIBS}
										INC ${INCS}
										DEF ${DEFS}
										INTERFACE ${INTERFACES}
										)
		set_property(TARGET ${btarget} PROPERTY INTERFACE_COMPILE_DEFINITIONS BOOST_ALL_NO_LIB)
	else()
		list(APPEND DEFS BOOST_ALL_DYN_LINK)
		__add_real_target(${btarget} dll SOURCE ${SRCS} 
										LIB ${LIBS}
										INC ${INCS}
										DEF ${DEFS}
										INTERFACE ${INTERFACES}
										)
		set_property(TARGET ${btarget} PROPERTY INTERFACE_COMPILE_DEFINITIONS BOOST_ALL_DYN_LINK BOOST_ALL_NO_LIB)
	endif()
endmacro()

macro(__find_boost_root)
	find_path(boost_INCLUDE_DIR boost/core/typeinfo.hpp
		HINTS "$ENV{CX_THIRDPARTY_ROOT}/include/"
		PATHS "/usr/include/")
endmacro()

macro(__include_boost)
	__cc_find(Boost)
	__assert_parameter(BOOST_INCLUDE_DIRS)
	include_directories(${BOOST_INCLUDE_DIRS})
endmacro()