set(CMAKE_CXX_STANDARD 14)

__cc_find(trimesh2)
__assert_target(trimesh2)

__cc_find(cxbin_whole)
__cc_find(QhullWrapper)
__assert_target(cxbin)
__assert_target(qhullWrapper)

__conan_import(boost dll COMPONENT boost_nowide)
__assert_target(boost_nowide)

__cc_find(openssl)
__assert_target(ssl)
__assert_target(crypto)

__cc_find(libtess2)
__assert_target(libtess2)

__cc_find(enchase)
__assert_target(enchase)

__files_group(${CMAKE_CURRENT_SOURCE_DIR}/cxkernel/ SRCS)
set(QRCS res/cxkernel.qrc res/images.qrc)

set(LIBS cxbin ssl crypto qhullWrapper libtess2 enchase)

__assert_target(msbase)
set(ILIBS qtuser_core trimesh2 msbase)

if(TARGET cxcloud)
	list(APPEND ILIBS cxcloud)
endif()

__cc_find(topomesh)
if(TARGET topomesh)
	list(APPEND LIBS topomesh)
endif()

__cc_find(nestplacer)
if(TARGET nestplacer)
	list(APPEND LIBS nestplacer)
endif()

__cc_find(sensors_analytics_sdk)
if(TARGET sensors_analytics_sdk)
	list(APPEND LIBS sensors_analytics_sdk)
endif()

#frameless 
if(CC_BC_WIN)
	list(APPEND SRCS ${CMAKE_CURRENT_SOURCE_DIR}/frameless/FrameLessView.h ${CMAKE_CURRENT_SOURCE_DIR}/frameless/FrameLessView_win.cpp)
else()
	list(APPEND SRCS ${CMAKE_CURRENT_SOURCE_DIR}/frameless/FrameLessView.h ${CMAKE_CURRENT_SOURCE_DIR}/frameless/FrameLessView_unix.cpp)
endif()

if(CC_BC_WIN)
	list(APPEND LIBS Dbghelp.lib crypt32 ws2_32)
elseif(CC_BC_MAC)
	# add_compile_options(-x objective-c++)
	list(APPEND SRCS ${CMAKE_CURRENT_SOURCE_DIR}/macwindow/macwindow.h ${CMAKE_CURRENT_SOURCE_DIR}/macwindow/macwindow.mm 
	${CMAKE_CURRENT_SOURCE_DIR}/macwindow/mycppobject.h ${CMAKE_CURRENT_SOURCE_DIR}/macwindow/mycppobject.cpp)
	find_library(APP_KIT Appkit Foundation)
	list(APPEND ILIBS ${APP_KIT})
endif()

set(INCS ${CMAKE_CURRENT_SOURCE_DIR})
set(DEFS CXKERNEL_DLL NOMINMAX)

__add_real_target(cxkernel dll SOURCE ${SRCS}
								LIB ${LIBS}
								ILIB ${ILIBS}
								INC ${INCS}
								DEF ${DEFS}
								INTERFACE ${INCS}
								QTQRC ${QRCS}
								)
