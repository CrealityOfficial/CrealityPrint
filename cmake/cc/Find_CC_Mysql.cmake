# This sets the following variables:
# MYSQL_INCLUDE_DIRS
# MYSQL_INCLUDE_FOUND


if(NOT MYSQL_INSTALL_ROOT)
	set(MYSQL_INSTALL_ROOT $ENV{USR_INSTALL_ROOT}/mysqllib)
	message("===MYSQL_INSTALL_ROOT ${MYSQL_INSTALL_ROOT}")
endif()

	message(STATUS "Mysql Specified MYSQL_INSTALL_ROOT : ${MYSQL_INSTALL_ROOT}")
	
	  # find_path(mysqlclient_INCLUDE_DIRS
			  # NAMES mysql.h
			  # HINTS "${MYSQL_INSTALL_ROOT}"
			  # PATHS "/usr/include/" "/usr/include/mysql/mysql.h"
					  # "/usr/local/include/" "/usr/local/include/mysql/mysql.h"
					  # "$ENV{MYSQL_INSTALL_ROOT}/include/" "$ENV{MYSQL_INSTALL_ROOT}/include/mysql"
			  # NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
			  # 
	 set(libmysql_INCLUDE_DIRS "${MYSQL_INSTALL_ROOT}/include")
	 find_library(libmysql_LIBRARIES_DEBUG
				  NAMES libmysql
				  PATHS "${MYSQL_INSTALL_ROOT}/lib/")	
	#set(mysqlclient_LIBRARIES_DEBUG	${MYSQL_INSTALL_ROOT}/lib/mysqlclient.lib)	
	find_library(libmysql_LIBRARIES_RELEASE
				NAMES libmysql
				PATHS "${MYSQL_INSTALL_ROOT}/lib/")
	

	message("libmysql_LIBRARIES_DEBUG  ${mysqlclient_LIBRARIES_DEBUG}")
	message("libmysql_LIBRARIES_RELEASE  ${mysqlclient_LIBRARIES_RELEASE}")
	message("libmysql_INCLUDE_DIRS    ${mysqlclient_INCLUDE_DIRS}")
# __search_target_components_signle(mysqlclient
						   # INC mysql.h
						   # DLIB mysqlclient
						   # LIB mysqlclient
						   # PRE mysql
						   # )
						   

__test_import(libmysql dll)


if(libmysql_INCLUDE_DIRS)
	set(MYSQL_INCLUDE_FOUND 1)
	set(MYSQL_INCLUDE_DIRS ${mysqlclient_INCLUDE_DIRS})
	message(STATUS "MYSQL_INCLUDE_DIRS : ${MYSQL_INCLUDE_DIRS}")
else()
	message(STATUS "Find Mysql include Failed maybe not set MYSQL_INSTALL_ROOT")
endif()