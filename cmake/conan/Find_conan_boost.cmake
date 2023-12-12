# boost target
if(CONAN_BOOST_ROOT_RELEASE)
	set(BOOST_INCLUDE_DIRS ${CONAN_BOOST_ROOT_RELEASE}/include/)
endif()

set(BOOST_COMPONETS boost_filesystem
					boost_iostreams
					boost_program_options
					boost_serialization
					boost_nowide
					boost_regex
					boost_system
					boost_thread
					boost_chrono
					)
					
if(CC_BC_WIN)
	list(APPEND BOOST_COMPONETS boost_log boost_locale)
endif()

if(BOOST_STATIC)
	__conan_import(boost lib COMPONENT ${BOOST_COMPONETS})
else()
	__conan_import(boost dll COMPONENT ${BOOST_COMPONETS})
endif()

__conan_import(boost lib COMPONENT boost_test)

foreach(_component ${BOOST_COMPONETS} )
	__add_target_interface(${_component} DEF BOOST_ALL_NO_LIB BOOST_ALL_DYN_LINK)
endforeach()

__add_target_interface(boost_test DEF BOOST_ALL_NO_LIB)