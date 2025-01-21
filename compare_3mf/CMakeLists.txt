get_filename_component(_APP_NAME ${CMAKE_CURRENT_LIST_DIR} NAME)

file(GLOB SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/*.h")

add_executable(${_APP_NAME} ${SOURCES})

target_link_libraries(${_APP_NAME} libslic3r OpenSSL::SSL OpenSSL::Crypto Bcrypt)

if (WIN32)
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        crealityprint_copy_dlls(COPY_DLLS "Debug" "d" "")
    else()
        crealityprint_copy_dlls(COPY_DLLS "Release" "" "")
    endif()
endif()
