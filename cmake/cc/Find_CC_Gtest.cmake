# This sets the following variables:
# gtest target
# gtest main

if(NOT TARGET gtest)
	__search_target_components(gtest
							   INC gtest/gtest.h
							   DLIB gtest
							   LIB gtest
							   PRE gtest
							   )
	
	__test_import(gtest lib)
endif()

if(NOT TARGET gtest_main)
	__search_target_components(gtest_main
							   INC gtest/gtest.h
							   DLIB gtest_main
							   LIB gtest_main
							   PRE gtest
							   )
	
	__test_import(gtest_main lib)
endif()