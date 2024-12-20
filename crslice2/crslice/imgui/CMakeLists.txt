cmake_minimum_required(VERSION 2.8.12)
project(imgui)

__cc_find(boost_header)

__add_real_target(imgui lib SOURCE
    imconfig.h
    imgui.cpp
    imgui.h
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_internal.h
    imgui_stdlib.cpp
    imgui_stdlib.h
    imgui_tables.cpp
    imgui_widgets.cpp
    # imgui STB
    imstb_rectpack.h
    imstb_textedit.h
    imstb_truetype.h
	LIB boost_header
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../
	FOLDER slice
)
