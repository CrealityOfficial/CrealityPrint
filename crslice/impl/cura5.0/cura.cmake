# Copyright (c) 2022 Ultimaker B.V.
# CuraEngine is released under the terms of the AGPLv3 or higher.
set(PREFIX5.2 impl/cura5.0/)
__files_group(${CMAKE_CURRENT_LIST_DIR}/ engine2)

list(APPEND SRCS ${engine2})
			
if(CC_BC_WIN)
	list(APPEND LIBS Bcrypt Ws2_32.lib)
endif()

list(APPEND INCS ${CMAKE_CURRENT_LIST_DIR})
		 
set(CVERSION "CURA_ENGINE_VERSION=\"5.2.0\"")
list(APPEND DEFS ${CVERSION} NOMINMAX ASSERT_INSANE_OUTPUT USE_CPU_TIME)

								  



