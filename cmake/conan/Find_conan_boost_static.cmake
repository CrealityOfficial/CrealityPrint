# boost_static target

set(BOOST_COMPONETS boost_filesystem
					boost_iostreams
					boost_program_options
					boost_nowide
					boost_regex
					boost_system
					boost_thread
					boost_chrono
					)
					
__conan_import(boost_static lib COMPONENT ${BOOST_COMPONETS})

foreach(_component ${BOOST_COMPONETS} )
	__add_target_interface(${_component} DEF BOOST_ALL_NO_LIB)
endforeach()