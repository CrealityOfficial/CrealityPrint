# wrapper_base target

__cc_find(cpython)
__cc_find(boost_header)

__assert_target(cpython)
__assert_target(boost_header)

__conan_import(wrapper_base dll ILIB boost_header cpython
								INTERFACE_DEF BOOST_DEBUG_PYTHON
								)
