cmake_minimum_required(VERSION 3.1)

project(auto_doxygen)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../")
include(ConfigureAllModules) 
		   
if(NOT CMAKE_DOXYGEN_NAME)
	set(CMAKE_DOXYGEN_NAME dummy)
endif()
if(NOT CMAKE_DOXYGEN_BRANCH)
	set(CMAKE_DOXYGEN_BRANCH master)
endif()

set(rep_prefix https://github.com/CrealityOfficial/)
set(branch master)

file(REMOVE_RECURSE ${CMAKE_SOURCE_DIR}/../../root/)
__execute(git clone -b ${CMAKE_DOXYGEN_BRANCH} ${rep_prefix}${CMAKE_DOXYGEN_NAME} root)
	
set(DOXYGEN_OUT_PATH ${CMAKE_BINARY_DIR}/doxygen)
file(MAKE_DIRECTORY ${DOXYGEN_OUT_PATH})

set(CMAKE_DOXYGEN_INPUT ${CMAKE_BINARY_DIR}/../root/)
set(CMAKE_DOXYGEN_OUTPUT_DIRECTORY ${DOXYGEN_OUT_PATH})

configure_file(${CMAKE_SOURCE_DIR}/DoxyFile.in
           ${DOXYGEN_OUT_PATH}/DoxyFile)
		   
__execute(doxygen ${DOXYGEN_OUT_PATH}/DoxyFile)

set(USER_NAME cxsw@172.16.33.10)
set(HTML_NET /vagrant_data/www/shared/build/doxygen/)

__execute(ssh ${USER_NAME} "rm -rf ${HTML_NET}${CMAKE_DOXYGEN_NAME}/${CMAKE_DOXYGEN_BRANCH}")
__execute(ssh ${USER_NAME} "mkdir -p ${HTML_NET}${CMAKE_DOXYGEN_NAME}/${CMAKE_DOXYGEN_BRANCH}")
__execute(scp -r ${DOXYGEN_OUT_PATH}/html/ ${USER_NAME}:${HTML_NET}${CMAKE_DOXYGEN_NAME}/${CMAKE_DOXYGEN_BRANCH}/html/)