add_library(libslic3r_cli STATIC
    CLI.cpp
    CLI.hpp
    CLIError.cpp
    CLIError.hpp
    SliceCommand.cpp
    SliceCommand.hpp
)

target_include_directories(libslic3r_cli
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

target_link_libraries(libslic3r_cli
    PRIVATE
        libslic3r
        libslic3r_gui
        boost_headeronly
)

target_compile_features(libslic3r_cli PUBLIC cxx_std_17)
