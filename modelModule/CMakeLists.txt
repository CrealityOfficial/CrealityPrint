cmake_minimum_required(VERSION 3.12)
project(modelModule)
set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCE_FILES 
src/modelStructure/*.cpp
src/modelTool/*.cpp
src/modelBusiness/*.cpp)

# 外部调用的库
set(LIBS)
# modelStructure层
__cc_find(trimesh2)
list(APPEND LIBS trimesh2)
__add_real_target(modelModule lib SOURCE ${SOURCE_FILES}
										 LIB ${LIBS})
target_include_directories(modelModule PUBLIC 
include/modelStructure
include/modelTool
include/modelBusiness
)

if (NOT DEFINED MODELMODULE_USE_UTEST)
	set(MODELMODULE_USE_UTEST ON)
endif()

# 模块内测试代码
if(MODELMODULE_USE_UTEST)
	file(GLOB_RECURSE TEST_FILES test/*.cpp)
	__cc_find(gtest)
	set(GTESTLIB gtest)
	__add_real_target(modelModuleSelfTest exe SOURCE ${SOURCE_FILES} ${TEST_FILES}
											LIB ${LIBS} ${GTESTLIB})
	
	target_include_directories(modelModuleSelfTest PUBLIC 
	include/modelStructure
	include/modelTool
	include/modelBusiness
	)
endif()
