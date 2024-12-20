#gtest

set(GTEST_COMPONETS gtest
					gtest_main
					)
					
__conan_import(gtest lib COMPONENT ${GTEST_COMPONETS})