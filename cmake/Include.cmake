set(HEAD_ONLY_INCLUDE ${CMAKE_SOURCE_DIR}/headonly/)

set(QTUSER_LIBS qtuser_core qtuser_3d qtuser_qml)
set(QTUSER_INCS ${CMAKE_SOURCE_DIR}/../qtuser/core/
				${CMAKE_SOURCE_DIR}/../qtuser/3d/
				${CMAKE_SOURCE_DIR}/../qtuser/qml/)
				
set(SLICE_INCLUDE ${CMAKE_SOURCE_DIR}/../slice/)

set(GOOGLE_TEST_INCLUDE ${CMAKE_SOURCE_DIR}/../google-test/google-test/googletest/include/
						${CMAKE_SOURCE_DIR}/../google-test/google-test/googlemock/include/)
						
set(CURA_INCLUDE ${CMAKE_SOURCE_DIR}/../cura/engine/)