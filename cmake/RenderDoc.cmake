## find renderdoc

if(DEFINED ENV{RENDER_DOC_ROOT})
	set(RENDER_DOC_PATH $ENV{RENDER_DOC_ROOT})
	set(RENDER_DOC_PATH_OUT ${RENDER_DOC_PATH})
	string(REPLACE "\\" "/" RENDER_DOC_PATH_OUT ${RENDER_DOC_PATH})
	message(STATUS "RENDER_DOC_PATH : ${RENDER_DOC_PATH_OUT}")
	
	if(EXISTS ${RENDER_DOC_PATH_OUT}/renderdoc.dll)
		set(RENDERDOC_DLL "${RENDER_DOC_PATH_OUT}/renderdoc.dll")
		set(RENDER_DOC_ENABLED 1)
	else()
		set(RENDER_DOC_ENABLED 0)
	endif()
else()
	set(RENDER_DOC_ENABLED 0)
endif()

if(RENDER_DOC_ENABLED)
	if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/RenderDoc.in)
		configure_file(${CMAKE_CURRENT_LIST_DIR}/RenderDoc.in
					${CMAKE_BINARY_DIR}/RenderDoc.h)
	endif()
endif()