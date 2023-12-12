# This sets the following variables:
# PYTHON_INCLUDE_DIR Python3_INCLUDE_DIRS
# PYTHON_LIBRARY PYTHON_LIBRARIES
# Python3 target

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
	
if(Python3_FOUND AND Python3_Development_FOUND)
	message(STATUS "Find Python3 ${Python3_VERSION}")
	message(STATUS "INCLUDES : ${Python3_INCLUDE_DIRS}")
	message(STATUS "LIBRARIES : ${Python3_LIBRARIES}")
	message(STATUS "LIBRARY DIRS : ${Python3_LIBRARY_DIRS}")
	
	set(PYTHON_INCLUDE_DIR ${Python3_INCLUDE_DIRS})
	list(GET Python3_LIBRARIES 3 Python3_LIBRARIES_DEBUG)
	list(GET Python3_LIBRARIES 1 Python3_LIBRARIES_RELEASE)
	message(STATUS "Python3 Debug LIBRARIES : ${Python3_LIBRARIES_DEBUG}")
	message(STATUS "Python3 Release LIBRARIES : ${Python3_LIBRARIES_RELEASE}")
	__import_target(Python3 dll)
	set(PYTHON_LIBRARY Python3)
	set(PYTHON_LIBRARIES Python3)
	set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
else()
	message(STATUS "Can't find Python3.")
endif()